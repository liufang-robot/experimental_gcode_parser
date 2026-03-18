#include "streaming_execution_engine.h"

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

bool messageStartsWith(const std::string &text, const std::string &prefix) {
  return text.rfind(prefix, 0) == 0;
}

bool shouldWaitForMoreInputOnRejected(const RejectedLine &rejected) {
  for (const auto &reason : rejected.reasons) {
    if (reason.message == "missing ENDIF for IF block") {
      return true;
    }
  }
  return false;
}

bool shouldWaitForMoreInputOnFault(const Diagnostic &diag) {
  return messageStartsWith(diag.message, "unresolved goto target: ") ||
         messageStartsWith(diag.message, "unresolved branch target: ");
}

std::string joinPendingLines(
    const std::deque<StreamingExecutionEngine::PendingLine> &lines) {
  std::string text;
  for (const auto &line : lines) {
    text += line.text;
    text.push_back('\n');
  }
  return text;
}

std::vector<int>
sourceLineMap(const std::deque<StreamingExecutionEngine::PendingLine> &lines) {
  std::vector<int> mapping;
  mapping.reserve(lines.size());
  for (const auto &line : lines) {
    mapping.push_back(line.line);
  }
  return mapping;
}

void remapInstructionSourceLines(std::vector<AilInstruction> *instructions,
                                 const std::vector<int> &line_map) {
  for (auto &instruction : *instructions) {
    std::visit(
        [&line_map](auto &inst) {
          if (inst.source.line > 0 &&
              static_cast<size_t>(inst.source.line) <= line_map.size()) {
            inst.source.line =
                line_map[static_cast<size_t>(inst.source.line - 1)];
          }
        },
        instruction);
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

  RuntimeResult<WaitToken>
  submitToolChange(const ToolChangeCommand &cmd) override {
    return runtime_.submitToolChange(cmd);
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
      state_ == EngineState::Faulted || state_ == EngineState::Rejected) {
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
  if (state_ == EngineState::Rejected) {
    StepResult result;
    result.status = StepStatus::Rejected;
    result.rejected = rejected_;
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
    return executePendingProgram();
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
  rejected_.reset();
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

StepResult StreamingExecutionEngine::executePendingProgram() {
  if (pending_lines_.empty()) {
    StepResult result;
    result.status = StepStatus::Progress;
    return result;
  }
  AilResult line_result =
      parseAndLowerAil(joinPendingLines(pending_lines_), options_);
  const auto line_map = sourceLineMap(pending_lines_);
  remapDiagnostics(&line_result.diagnostics, line_map.front());
  remapRejectedLines(&line_result.rejected_lines, line_map.front());
  remapInstructionSourceLines(&line_result.instructions, line_map);

  if (!line_result.rejected_lines.empty() && !input_finished_ &&
      shouldWaitForMoreInputOnRejected(line_result.rejected_lines.front())) {
    state_ = EngineState::ReadyToExecute;
    StepResult result;
    result.status = StepStatus::Progress;
    return result;
  }

  if (line_result.instructions.empty()) {
    emitDiagnostics(line_result.diagnostics);
    if (!line_result.rejected_lines.empty()) {
      if (const auto rejected =
              makeRejectedEvent(line_result.rejected_lines.front());
          rejected.has_value()) {
        sink_.onRejectedLine(*rejected);
        RejectedState rejected_state;
        rejected_state.source = rejected->source;
        rejected_state.reasons = rejected->reasons;
        return makeRejectedResult(rejected_state);
      }
      RejectedState rejected_state;
      rejected_state.source.line = pending_lines_.front().line;
      rejected_state.reasons.push_back(
          makeFaultDiagnostic(pending_lines_.front().line, "line rejected"));
      return makeRejectedResult(rejected_state);
    }
    pending_lines_.clear();
    state_ =
        input_finished_ ? EngineState::Completed : EngineState::ReadyToExecute;
    StepResult step_result;
    step_result.status = state_ == EngineState::Completed
                             ? StepStatus::Completed
                             : StepStatus::Progress;
    return step_result;
  }

  if (!line_result.rejected_lines.empty()) {
    deferred_rejected_ = RejectedState{};
    deferred_rejected_->source =
        toSourceRef(line_result.rejected_lines.front().source);
    deferred_rejected_->reasons = line_result.rejected_lines.front().reasons;
  } else {
    emitDiagnostics(line_result.diagnostics);
    deferred_rejected_.reset();
  }

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
  active_executor_line_ = pending_lines_.front().line;
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
    const auto &executor_state = active_executor_->state();
    if (executor_state.status == ExecutorStatus::Blocked &&
        executor_state.blocked.has_value() &&
        executor_state.blocked->wait_token.has_value()) {
      while (active_executor_emitted_diagnostics_ <
             executor_diagnostics.size()) {
        sink_.onDiagnostic(
            executor_diagnostics[active_executor_emitted_diagnostics_++]);
      }
      return makeBlockedResult(active_executor_line_,
                               *executor_state.blocked->wait_token,
                               "instruction execution in progress");
    }
    if (executor_state.status == ExecutorStatus::Fault) {
      const bool has_executor_fault_diagnostic =
          active_executor_emitted_diagnostics_ < executor_diagnostics.size() ||
          !executor_diagnostics.empty();
      const Diagnostic diag =
          !executor_diagnostics.empty()
              ? executor_diagnostics.back()
              : makeFaultDiagnostic(active_executor_line_,
                                    "executor faulted without diagnostic");
      if (!input_finished_ && shouldWaitForMoreInputOnFault(diag)) {
        active_executor_.reset();
        active_executor_emitted_diagnostics_ = 0;
        deferred_rejected_.reset();
        state_ = EngineState::ReadyToExecute;
        StepResult result;
        result.status = StepStatus::Progress;
        return result;
      }
      while (active_executor_emitted_diagnostics_ <
             executor_diagnostics.size()) {
        sink_.onDiagnostic(
            executor_diagnostics[active_executor_emitted_diagnostics_++]);
      }
      active_executor_.reset();
      if (!has_executor_fault_diagnostic) {
        return faultWithDiagnostic(diag);
      }
      blocked_.reset();
      rejected_.reset();
      state_ = EngineState::Faulted;
      StepResult result;
      result.status = StepStatus::Faulted;
      result.fault = diag;
      return result;
    }
    if (executor_state.status == ExecutorStatus::Completed) {
      while (active_executor_emitted_diagnostics_ <
             executor_diagnostics.size()) {
        sink_.onDiagnostic(
            executor_diagnostics[active_executor_emitted_diagnostics_++]);
      }
      current_motion_code_ = executor_state.motion_code_current;
      current_working_plane_ = executor_state.working_plane_current;
      current_rapid_mode_ = executor_state.rapid_mode_current;
      current_tool_radius_comp_ = executor_state.tool_radius_comp_current;
      current_active_tool_selection_ = executor_state.active_tool_selection;
      current_pending_tool_selection_ = executor_state.pending_tool_selection;
      current_user_variables_ = executor_state.user_variables;
      active_executor_.reset();
      active_executor_emitted_diagnostics_ = 0;
      if (deferred_rejected_.has_value()) {
        for (const auto &diag : deferred_rejected_->reasons) {
          sink_.onDiagnostic(diag);
        }
        sink_.onRejectedLine(RejectedLineEvent{deferred_rejected_->source,
                                               deferred_rejected_->reasons});
        const auto rejected = *deferred_rejected_;
        deferred_rejected_.reset();
        return makeRejectedResult(rejected);
      }
      pending_lines_.clear();
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
    while (active_executor_emitted_diagnostics_ < executor_diagnostics.size()) {
      sink_.onDiagnostic(
          executor_diagnostics[active_executor_emitted_diagnostics_++]);
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
  rejected_.reset();
  state_ = EngineState::Blocked;
  StepResult result;
  result.status = StepStatus::Blocked;
  result.blocked = blocked_;
  return result;
}

StepResult
StreamingExecutionEngine::makeRejectedResult(const RejectedState &rejected) {
  blocked_.reset();
  rejected_ = rejected;
  state_ = EngineState::Rejected;
  StepResult result;
  result.status = StepStatus::Rejected;
  result.rejected = rejected_;
  return result;
}

StepResult
StreamingExecutionEngine::faultWithDiagnostic(const Diagnostic &diag) {
  blocked_.reset();
  rejected_.reset();
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
    const RejectedLine &rejected) const {
  RejectedLineEvent event;
  event.source = toSourceRef(rejected.source);
  event.reasons = rejected.reasons;
  return event;
}

void StreamingExecutionEngine::remapDiagnostics(
    std::vector<Diagnostic> *diagnostics, int line) const {
  for (auto &diag : *diagnostics) {
    diag.location.line = line + diag.location.line - 1;
  }
}

void StreamingExecutionEngine::remapRejectedLines(
    std::vector<RejectedLine> *rejected_lines, int line) const {
  for (auto &rejected : *rejected_lines) {
    rejected.source.line = line + rejected.source.line - 1;
    for (auto &diag : rejected.reasons) {
      diag.location.line = line + diag.location.line - 1;
    }
  }
}

AilExecutorInitialState StreamingExecutionEngine::exportInitialState() const {
  AilExecutorInitialState initial_state;
  initial_state.motion_code_current = current_motion_code_;
  initial_state.rapid_mode_current = current_rapid_mode_;
  initial_state.tool_radius_comp_current = current_tool_radius_comp_;
  initial_state.working_plane_current = current_working_plane_;
  initial_state.active_tool_selection = current_active_tool_selection_;
  initial_state.pending_tool_selection = current_pending_tool_selection_;
  initial_state.user_variables = current_user_variables_;
  return initial_state;
}

void StreamingExecutionEngine::importInitialState(
    const AilExecutorInitialState &state, int next_line_number) {
  current_motion_code_ = state.motion_code_current;
  current_rapid_mode_ = state.rapid_mode_current;
  current_tool_radius_comp_ = state.tool_radius_comp_current;
  current_working_plane_ = state.working_plane_current;
  current_active_tool_selection_ = state.active_tool_selection;
  current_pending_tool_selection_ = state.pending_tool_selection;
  current_user_variables_ = state.user_variables;
  next_line_number_ = next_line_number;
}

} // namespace gcode
