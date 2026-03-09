#include "execution_instruction_dispatcher.h"

namespace gcode {

ExecutionDispatchResult
dispatchExecutionInstruction(const AilInstruction &instruction, int line,
                             const ExecutionModalState &modal_state,
                             IExecutionSink &sink, IRuntime &runtime) {
  if (std::holds_alternative<AilLinearMoveInstruction>(instruction)) {
    const auto &inst = std::get<AilLinearMoveInstruction>(instruction);
    LinearMoveCommand cmd = buildLinearMoveCommand(inst, line, modal_state);
    sink.onLinearMove(cmd);
    const auto runtime_result = runtime.submitLinearMove(cmd);
    if (runtime_result.status == RuntimeCallStatus::Pending &&
        runtime_result.wait_token.has_value()) {
      return {ExecutionDispatchResult::Status::Blocked, cmd.source.line,
              runtime_result.wait_token, "linear move in progress"};
    }
    if (runtime_result.status == RuntimeCallStatus::Error) {
      return {ExecutionDispatchResult::Status::Error, cmd.source.line,
              std::nullopt, runtime_result.error_message};
    }
    return {ExecutionDispatchResult::Status::Progress, cmd.source.line,
            std::nullopt, ""};
  }
  if (std::holds_alternative<AilArcMoveInstruction>(instruction)) {
    const auto &inst = std::get<AilArcMoveInstruction>(instruction);
    ArcMoveCommand cmd = buildArcMoveCommand(inst, line, modal_state);
    sink.onArcMove(cmd);
    const auto runtime_result = runtime.submitArcMove(cmd);
    if (runtime_result.status == RuntimeCallStatus::Pending &&
        runtime_result.wait_token.has_value()) {
      return {ExecutionDispatchResult::Status::Blocked, cmd.source.line,
              runtime_result.wait_token, "arc move in progress"};
    }
    if (runtime_result.status == RuntimeCallStatus::Error) {
      return {ExecutionDispatchResult::Status::Error, cmd.source.line,
              std::nullopt, runtime_result.error_message};
    }
    return {ExecutionDispatchResult::Status::Progress, cmd.source.line,
            std::nullopt, ""};
  }
  if (std::holds_alternative<AilDwellInstruction>(instruction)) {
    const auto &inst = std::get<AilDwellInstruction>(instruction);
    DwellCommand cmd = buildDwellCommand(inst, line, modal_state);
    sink.onDwell(cmd);
    const auto runtime_result = runtime.submitDwell(cmd);
    if (runtime_result.status == RuntimeCallStatus::Pending &&
        runtime_result.wait_token.has_value()) {
      return {ExecutionDispatchResult::Status::Blocked, cmd.source.line,
              runtime_result.wait_token, "dwell in progress"};
    }
    if (runtime_result.status == RuntimeCallStatus::Error) {
      return {ExecutionDispatchResult::Status::Error, cmd.source.line,
              std::nullopt, runtime_result.error_message};
    }
    return {ExecutionDispatchResult::Status::Progress, cmd.source.line,
            std::nullopt, ""};
  }

  return {};
}

} // namespace gcode
