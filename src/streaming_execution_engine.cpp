#include "gcode/streaming_execution_engine.h"

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
  blocked_.reset();
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

void StreamingExecutionEngine::cancel() {
  if (state_ == EngineState::Blocked && blocked_.has_value()) {
    (void)runtime_.cancelWait(blocked_->token);
    blocked_.reset();
  }
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

  for (const auto &instruction : line_result.instructions) {
    StepResult step_result = executeAilInstruction(instruction, line.line);
    if (step_result.status != StepStatus::Progress) {
      return step_result;
    }
  }

  state_ = pending_lines_.empty() && input_finished_
               ? EngineState::Completed
               : EngineState::ReadyToExecute;
  if (state_ == EngineState::Completed) {
    StepResult result;
    result.status = StepStatus::Completed;
    return result;
  }
  StepResult step_result;
  step_result.status = StepStatus::Progress;
  return step_result;
}

StepResult StreamingExecutionEngine::executeAilInstruction(
    const AilInstruction &instruction, int line) {
  if (std::holds_alternative<AilRapidTraverseModeInstruction>(instruction)) {
    current_rapid_mode_ =
        std::get<AilRapidTraverseModeInstruction>(instruction).mode;
    StepResult result;
    result.status = StepStatus::Progress;
    return result;
  }
  if (std::holds_alternative<AilToolRadiusCompInstruction>(instruction)) {
    current_tool_radius_comp_ =
        std::get<AilToolRadiusCompInstruction>(instruction).mode;
    StepResult result;
    result.status = StepStatus::Progress;
    return result;
  }
  if (std::holds_alternative<AilWorkingPlaneInstruction>(instruction)) {
    current_working_plane_ =
        std::get<AilWorkingPlaneInstruction>(instruction).plane;
    StepResult result;
    result.status = StepStatus::Progress;
    return result;
  }
  if (std::holds_alternative<AilLinearMoveInstruction>(instruction)) {
    const auto &inst = std::get<AilLinearMoveInstruction>(instruction);
    const ExecutionModalState modal_state{
        current_working_plane_, current_rapid_mode_, current_tool_radius_comp_};
    LinearMoveCommand cmd = buildLinearMoveCommand(inst, line, modal_state);
    sink_.onLinearMove(cmd);
    const auto runtime_result = runtime_.submitLinearMove(cmd);
    if (runtime_result.status == RuntimeCallStatus::Pending &&
        runtime_result.wait_token.has_value()) {
      return makeBlockedResult(cmd.source.line, *runtime_result.wait_token,
                               "linear move in progress");
    }
    if (runtime_result.status == RuntimeCallStatus::Error) {
      return faultWithDiagnostic(
          makeFaultDiagnostic(cmd.source.line, runtime_result.error_message));
    }
  } else if (std::holds_alternative<AilArcMoveInstruction>(instruction)) {
    const auto &inst = std::get<AilArcMoveInstruction>(instruction);
    const ExecutionModalState modal_state{
        current_working_plane_, current_rapid_mode_, current_tool_radius_comp_};
    ArcMoveCommand cmd = buildArcMoveCommand(inst, line, modal_state);
    sink_.onArcMove(cmd);
    const auto runtime_result = runtime_.submitArcMove(cmd);
    if (runtime_result.status == RuntimeCallStatus::Pending &&
        runtime_result.wait_token.has_value()) {
      return makeBlockedResult(cmd.source.line, *runtime_result.wait_token,
                               "arc move in progress");
    }
    if (runtime_result.status == RuntimeCallStatus::Error) {
      return faultWithDiagnostic(
          makeFaultDiagnostic(cmd.source.line, runtime_result.error_message));
    }
  } else if (std::holds_alternative<AilDwellInstruction>(instruction)) {
    const auto &inst = std::get<AilDwellInstruction>(instruction);
    const ExecutionModalState modal_state{
        current_working_plane_, current_rapid_mode_, current_tool_radius_comp_};
    DwellCommand cmd = buildDwellCommand(inst, line, modal_state);
    sink_.onDwell(cmd);
    const auto runtime_result = runtime_.submitDwell(cmd);
    if (runtime_result.status == RuntimeCallStatus::Pending &&
        runtime_result.wait_token.has_value()) {
      return makeBlockedResult(cmd.source.line, *runtime_result.wait_token,
                               "dwell in progress");
    }
    if (runtime_result.status == RuntimeCallStatus::Error) {
      return faultWithDiagnostic(
          makeFaultDiagnostic(cmd.source.line, runtime_result.error_message));
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
