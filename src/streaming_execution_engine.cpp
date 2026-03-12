#include "gcode/streaming_execution_engine.h"

#include <memory>
#include <utility>

#include "execution_command_builder.h"
#include "gcode/gcode_parser.h"

namespace gcode {
namespace {

Diagnostic makeFaultDiagnostic(int line, const std::string &message) {
  Diagnostic diag;
  diag.severity = Diagnostic::Severity::Error;
  diag.location = {line, 1};
  diag.message = message;
  return diag;
}

void remapInstructionSourceLines(std::vector<AilInstruction> *instructions,
                                 int line) {
  for (auto &instruction : *instructions) {
    std::visit([line](auto &inst) { inst.source.line = line; }, instruction);
  }
}

class RuntimeOnlyExecutionRuntime final : public IExecutionRuntime {
public:
  explicit RuntimeOnlyExecutionRuntime(IRuntime &runtime) : runtime_(runtime) {}

  ConditionResolution resolve(const Condition &,
                              const SourceInfo &source) const override {
    ConditionResolution resolution;
    resolution.kind = ConditionResolutionKind::Error;
    resolution.error_message = "condition resolution is unavailable for "
                               "runtime-only streaming execution";
    return resolution;
  }

  RuntimeResult<WaitToken>
  submitLinearMove(const LinearMoveCommand &cmd) override {
    return runtime_.submitLinearMove(cmd);
  }

  RuntimeResult<WaitToken> submitArcMove(const ArcMoveCommand &cmd) override {
    return runtime_.submitArcMove(cmd);
  }

  RuntimeResult<WaitToken> submitDwell(const DwellCommand &cmd) override {
    return runtime_.submitDwell(cmd);
  }

  RuntimeResult<double> readSystemVariable(std::string_view name) override {
    return runtime_.readSystemVariable(name);
  }

  RuntimeResult<WaitToken> cancelWait(const WaitToken &token) override {
    return runtime_.cancelWait(token);
  }

private:
  IRuntime &runtime_;
};

} // namespace

StreamingExecutionEngine::StreamingExecutionEngine(IExecutionSink &sink,
                                                   IRuntime &runtime,
                                                   ICancellation &cancellation,
                                                   const LowerOptions &options)
    : sink_(sink), runtime_(runtime), cancellation_(cancellation),
      options_(options) {}

StreamingExecutionEngine::StreamingExecutionEngine(IExecutionSink &sink,
                                                   IExecutionRuntime &runtime,
                                                   ICancellation &cancellation,
                                                   const LowerOptions &options)
    : StreamingExecutionEngine(sink, static_cast<IRuntime &>(runtime),
                               cancellation, options) {
  execution_runtime_ = &runtime;
}

bool StreamingExecutionEngine::pushChunk(std::string_view chunk) {
  if (state_ == EngineState::Cancelled || state_ == EngineState::Completed ||
      state_ == EngineState::Faulted) {
    return false;
  }
  input_buffer_.append(chunk.data(), chunk.size());
  return enqueueCompleteLines();
}

StepResult StreamingExecutionEngine::pump() {
  if (state_ == EngineState::Faulted) {
    StepResult result;
    result.status = StepStatus::Faulted;
    return result;
  }
  if (state_ == EngineState::Cancelled) {
    StepResult result;
    result.status = StepStatus::Cancelled;
    return result;
  }
  if (state_ == EngineState::Completed) {
    StepResult result;
    result.status = StepStatus::Completed;
    return result;
  }
  if (state_ == EngineState::Blocked) {
    StepResult result;
    result.status = StepStatus::Blocked;
    result.blocked = blocked_;
    return result;
  }
  if (active_executor_ != nullptr) {
    return advanceActiveExecutor();
  }
  if (cancellation_.isCancelled()) {
    cancel();
    StepResult result;
    result.status = StepStatus::Cancelled;
    return result;
  }
  if (!pending_lines_.empty()) {
    return executeNextLine();
  }
  if (input_finished_) {
    state_ = EngineState::Completed;
    StepResult result;
    result.status = StepStatus::Completed;
    return result;
  }
  state_ = EngineState::ReadyToExecute;
  StepResult result;
  result.status = StepStatus::Progress;
  return result;
}

StepResult StreamingExecutionEngine::finish() {
  input_finished_ = true;
  if (!input_buffer_.empty()) {
    pending_lines_.push_back({next_line_number_++, input_buffer_});
    input_buffer_.clear();
  }
  return pump();
}

StepResult StreamingExecutionEngine::resume(const WaitToken &token) {
  if (state_ != EngineState::Blocked || !blocked_.has_value()) {
    Diagnostic diag = makeFaultDiagnostic(0, "resume called while not blocked");
    return faultWithDiagnostic(diag);
  }
  if (!(blocked_->token == token)) {
    Diagnostic diag =
        makeFaultDiagnostic(blocked_->line, "resume token does not match");
    return faultWithDiagnostic(diag);
  }
  if (active_executor_ != nullptr) {
    active_executor_->notifyEvent(token);
  }
  blocked_.reset();
  state_ =
      active_executor_ == nullptr && pending_lines_.empty() && input_finished_
          ? EngineState::Completed
          : EngineState::ReadyToExecute;
  if (state_ == EngineState::Completed) {
    StepResult result;
    result.status = StepStatus::Completed;
    return result;
  }
  StepResult result;
  result.status = StepStatus::Progress;
  return result;
}

void StreamingExecutionEngine::cancel() {
  if (state_ == EngineState::Blocked && blocked_.has_value()) {
    (void)runtime_.cancelWait(blocked_->token);
    blocked_.reset();
  }
  active_executor_.reset();
  state_ = EngineState::Cancelled;
}

bool StreamingExecutionEngine::enqueueCompleteLines() {
  size_t start = 0;
  while (start < input_buffer_.size()) {
    const size_t newline_pos = input_buffer_.find('\n', start);
    if (newline_pos == std::string::npos) {
      break;
    }
    std::string line = input_buffer_.substr(start, newline_pos - start);
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    pending_lines_.push_back({next_line_number_++, std::move(line)});
    start = newline_pos + 1;
  }
  input_buffer_.erase(0, start);
  return true;
}

StepResult StreamingExecutionEngine::executeNextLine() {
  if (pending_lines_.empty()) {
    StepResult result;
    result.status = StepStatus::Progress;
    return result;
  }
  const PendingLine line = std::move(pending_lines_.front());
  pending_lines_.pop_front();

  std::string line_text = line.text;
  line_text.push_back('\n');
  AilResult line_result = parseAndLowerAil(line_text, options_);
  remapDiagnostics(&line_result.diagnostics, line.line);
  remapRejectedLines(&line_result.rejected_lines, line.line);
  emitDiagnostics(line_result.diagnostics);
  if (!line_result.rejected_lines.empty()) {
    if (const auto rejected =
            makeRejectedEvent(line_result.rejected_lines.front());
        rejected.has_value()) {
      sink_.onRejectedLine(*rejected);
      if (!rejected->reasons.empty()) {
        return faultWithDiagnostic(rejected->reasons.front());
      }
    }
    Diagnostic diag = makeFaultDiagnostic(line.line, "line rejected");
    return faultWithDiagnostic(diag);
  }
  if (line_result.instructions.empty()) {
    state_ = pending_lines_.empty() && input_finished_
                 ? EngineState::Completed
                 : EngineState::ReadyToExecute;
    if (state_ == EngineState::Completed) {
      StepResult step_result;
      step_result.status = StepStatus::Completed;
      return step_result;
    }
    StepResult step_result;
    step_result.status = StepStatus::Progress;
    return step_result;
  }
  remapInstructionSourceLines(&line_result.instructions, line.line);

  AilExecutorOptions executor_options;
  AilExecutorInitialState initial_state;
  initial_state.motion_code_current = current_motion_code_;
  initial_state.rapid_mode_current = current_rapid_mode_;
  initial_state.tool_radius_comp_current = current_tool_radius_comp_;
  initial_state.working_plane_current = current_working_plane_;
  initial_state.active_tool_selection = current_active_tool_selection_;
  initial_state.pending_tool_selection = current_pending_tool_selection_;
  executor_options.initial_state = std::move(initial_state);
  active_executor_ = std::make_unique<AilExecutor>(
      std::move(line_result.instructions), std::move(executor_options));
  active_executor_line_ = line.line;
  active_executor_emitted_diagnostics_ = 0;
  return advanceActiveExecutor();
}

StepResult StreamingExecutionEngine::advanceActiveExecutor() {
  if (active_executor_ == nullptr) {
    StepResult result;
    result.status = StepStatus::Progress;
    return result;
  }

  RuntimeOnlyExecutionRuntime runtime_adapter(runtime_);
  IExecutionRuntime &execution_runtime =
      execution_runtime_ != nullptr ? *execution_runtime_ : runtime_adapter;

  while (active_executor_ != nullptr) {
    const bool progressed = active_executor_->step(0, sink_, execution_runtime);
    const auto &executor_diagnostics = active_executor_->diagnostics();
    while (active_executor_emitted_diagnostics_ < executor_diagnostics.size()) {
      sink_.onDiagnostic(
          executor_diagnostics[active_executor_emitted_diagnostics_++]);
    }

    const auto &executor_state = active_executor_->state();
    if (executor_state.status == ExecutorStatus::Blocked &&
        executor_state.blocked.has_value() &&
        executor_state.blocked->wait_token.has_value()) {
      return makeBlockedResult(active_executor_line_,
                               *executor_state.blocked->wait_token,
                               "instruction execution in progress");
    }
    if (executor_state.status == ExecutorStatus::Fault) {
      const Diagnostic diag =
          executor_diagnostics.empty()
              ? makeFaultDiagnostic(active_executor_line_,
                                    "executor faulted without diagnostic")
              : executor_diagnostics.back();
      active_executor_.reset();
      return faultWithDiagnostic(diag);
    }
    if (executor_state.status == ExecutorStatus::Completed) {
      current_motion_code_ = executor_state.motion_code_current;
      current_working_plane_ = executor_state.working_plane_current;
      current_rapid_mode_ = executor_state.rapid_mode_current;
      current_tool_radius_comp_ = executor_state.tool_radius_comp_current;
      current_active_tool_selection_ = executor_state.active_tool_selection;
      current_pending_tool_selection_ = executor_state.pending_tool_selection;
      active_executor_.reset();
      state_ = pending_lines_.empty() && input_finished_
                   ? EngineState::Completed
                   : EngineState::ReadyToExecute;
      if (state_ == EngineState::Completed) {
        StepResult result;
        result.status = StepStatus::Completed;
        return result;
      }
      StepResult result;
      result.status = StepStatus::Progress;
      return result;
    }
    if (!progressed) {
      break;
    }
  }

  StepResult result;
  result.status = StepStatus::Progress;
  return result;
}

StepResult StreamingExecutionEngine::makeBlockedResult(int line,
                                                       const WaitToken &token,
                                                       std::string reason) {
  blocked_ = BlockedState{line, token, std::move(reason)};
  state_ = EngineState::Blocked;
  StepResult result;
  result.status = StepStatus::Blocked;
  result.blocked = blocked_;
  return result;
}

StepResult
StreamingExecutionEngine::faultWithDiagnostic(const Diagnostic &diag) {
  sink_.onDiagnostic(diag);
  state_ = EngineState::Faulted;
  StepResult result;
  result.status = StepStatus::Faulted;
  result.fault = diag;
  return result;
}

void StreamingExecutionEngine::emitDiagnostics(
    const std::vector<Diagnostic> &diagnostics) {
  for (const auto &diag : diagnostics) {
    sink_.onDiagnostic(diag);
  }
}

std::optional<RejectedLineEvent> StreamingExecutionEngine::makeRejectedEvent(
    const MessageResult::RejectedLine &rejected) const {
  RejectedLineEvent event;
  event.source = toSourceRef(rejected.source);
  event.reasons = rejected.reasons;
  return event;
}

void StreamingExecutionEngine::remapDiagnostics(
    std::vector<Diagnostic> *diagnostics, int line) const {
  for (auto &diag : *diagnostics) {
    diag.location.line = line;
  }
}

void StreamingExecutionEngine::remapRejectedLines(
    std::vector<MessageResult::RejectedLine> *rejected_lines, int line) const {
  for (auto &rejected : *rejected_lines) {
    rejected.source.line = line;
    for (auto &diag : rejected.reasons) {
      diag.location.line = line;
    }
  }
}

} // namespace gcode
