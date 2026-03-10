#include "gcode/streaming_execution_engine.h"

#include <memory>
#include <utility>

#include "execution_command_builder.h"
#include "gcode/gcode_parser.h"
#include "runtime_expression_eval.h"

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

  ConditionResolution resolve(const Condition &condition,
                              const SourceInfo &) const override {
    return resolveRuntimeCondition(condition, runtime_);
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

  RuntimeResult<double> readVariable(std::string_view name) override {
    return runtime_.readVariable(name);
  }

  RuntimeResult<double> readSystemVariable(std::string_view name) override {
    return runtime_.readSystemVariable(name);
  }

  RuntimeResult<void> writeVariable(std::string_view name,
                                    double value) override {
    return runtime_.writeVariable(name, value);
  }

  RuntimeResult<WaitToken> cancelWait(const WaitToken &token) override {
    return runtime_.cancelWait(token);
  }

  RuntimeResult<double>
  evaluateExpression(const ExprNode &expression) override {
    return evaluateRuntimeExpression(expression, runtime_);
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
  while (true) {
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
    if (state_ == EngineState::WaitingForInput) {
      if (!pending_lines_.empty()) {
        state_ = EngineState::ReadyToExecute;
        continue;
      }
      if (input_finished_ && executor_ != nullptr) {
        executor_->setInputFinished(true);
        state_ = EngineState::ReadyToExecute;
        continue;
      }
      StepResult result;
      result.status = StepStatus::WaitingForInput;
      result.waiting = WaitingForInputState{
          next_line_number_ - 1, "streaming control flow needs more input"};
      return result;
    }
    if (cancellation_.isCancelled()) {
      cancel();
      StepResult result;
      result.status = StepStatus::Cancelled;
      return result;
    }
    if (!pending_lines_.empty()) {
      const auto step = executeNextLine();
      if (step.status != StepStatus::Progress) {
        return step;
      }
      continue;
    }
    if (executor_ != nullptr) {
      const auto step = advanceExecutor();
      if (step.status != StepStatus::Progress) {
        return step;
      }
      if (state_ == EngineState::ReadyToExecute) {
        if (pending_lines_.empty()) {
          break;
        }
        continue;
      }
      continue;
    }
    if (input_finished_) {
      state_ = EngineState::Completed;
      StepResult result;
      result.status = StepStatus::Completed;
      return result;
    }
    state_ = EngineState::ReadyToExecute;
    break;
  }
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
  if (executor_ != nullptr) {
    executor_->notifyEvent(token);
  }
  blocked_.reset();
  state_ = executor_ == nullptr && pending_lines_.empty() && input_finished_
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
  executor_.reset();
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
    state_ = EngineState::ReadyToExecute;
    StepResult step_result;
    step_result.status = StepStatus::Progress;
    return step_result;
  }
  remapInstructionSourceLines(&line_result.instructions, line.line);
  for (const auto &instruction : line_result.instructions) {
    buffered_instructions_.push_back(instruction);
  }

  if (executor_ == nullptr) {
    AilExecutorOptions executor_options;
    executor_options.defer_unresolved_targets_until_input_finished = true;
    executor_options.initial_state = AilExecutorInitialState{
        std::nullopt, ToolRadiusCompMode::Off, WorkingPlane::XY};
    executor_ = std::make_unique<AilExecutor>(
        std::move(line_result.instructions), std::move(executor_options));
    executor_emitted_diagnostics_ = 0;
  } else {
    executor_->appendInstructions(std::move(line_result.instructions));
  }

  state_ = EngineState::ReadyToExecute;
  StepResult step_result;
  step_result.status = StepStatus::Progress;
  return step_result;
}

StepResult StreamingExecutionEngine::advanceExecutor() {
  if (executor_ == nullptr) {
    StepResult result;
    result.status = StepStatus::Progress;
    return result;
  }

  RuntimeOnlyExecutionRuntime runtime_adapter(runtime_);
  IExecutionRuntime &execution_runtime =
      execution_runtime_ != nullptr ? *execution_runtime_ : runtime_adapter;

  while (executor_ != nullptr) {
    const bool progressed = executor_->step(0, sink_, execution_runtime);
    const auto &executor_diagnostics = executor_->diagnostics();
    while (executor_emitted_diagnostics_ < executor_diagnostics.size()) {
      sink_.onDiagnostic(executor_diagnostics[executor_emitted_diagnostics_++]);
    }

    const auto &executor_state = executor_->state();
    if (executor_state.status == ExecutorStatus::Blocked &&
        executor_state.blocked.has_value() &&
        executor_state.blocked->wait_token.has_value()) {
      const size_t blocked_index =
          executor_state.blocked->instruction_index > 0
              ? executor_state.blocked->instruction_index - 1
              : executor_state.pc;
      const int blocked_line =
          blocked_index < buffered_instructions_.size()
              ? std::visit([](const auto &inst) { return inst.source.line; },
                           buffered_instructions_[blocked_index])
              : 0;
      return makeBlockedResult(blocked_line,
                               *executor_state.blocked->wait_token,
                               "instruction execution in progress");
    }
    if (executor_state.status == ExecutorStatus::Blocked &&
        executor_state.blocked.has_value() &&
        !executor_state.blocked->wait_token.has_value() &&
        !executor_state.blocked->retry_at_ms.has_value()) {
      const size_t blocked_index = executor_state.blocked->instruction_index;
      const int blocked_line =
          blocked_index < buffered_instructions_.size()
              ? std::visit([](const auto &inst) { return inst.source.line; },
                           buffered_instructions_[blocked_index])
              : 0;
      if (input_finished_) {
        executor_->setInputFinished(true);
        continue;
      }
      return makeWaitingForInputResult(
          blocked_line,
          "unresolved control-flow target may resolve from future input");
    }
    if (executor_state.status == ExecutorStatus::Fault) {
      const Diagnostic diag =
          executor_diagnostics.empty()
              ? makeFaultDiagnostic(0, "executor faulted without diagnostic")
              : executor_diagnostics.back();
      executor_.reset();
      return faultWithDiagnostic(diag);
    }
    if (executor_state.status == ExecutorStatus::Completed) {
      if (input_finished_ && pending_lines_.empty() &&
          executor_state.pc >= buffered_instructions_.size()) {
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
StreamingExecutionEngine::makeWaitingForInputResult(int line,
                                                    std::string reason) {
  blocked_.reset();
  state_ = EngineState::WaitingForInput;
  StepResult result;
  result.status = StepStatus::WaitingForInput;
  result.waiting = WaitingForInputState{line, std::move(reason)};
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
