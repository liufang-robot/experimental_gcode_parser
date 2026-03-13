#include "execution_instruction_dispatcher.h"

namespace gcode {
namespace {

ExecutionDispatchResult makeProgressResult(int line) {
  return {ExecutionDispatchResult::Status::Progress, line, std::nullopt, ""};
}

ExecutionDispatchResult
makeRuntimeDispatchResult(int line,
                          const RuntimeResult<WaitToken> &runtime_result,
                          const std::string &pending_message) {
  if (runtime_result.status == RuntimeCallStatus::Pending &&
      runtime_result.wait_token.has_value()) {
    return {ExecutionDispatchResult::Status::Blocked, line,
            runtime_result.wait_token, pending_message};
  }
  if (runtime_result.status == RuntimeCallStatus::Error) {
    return {ExecutionDispatchResult::Status::Error, line, std::nullopt,
            runtime_result.error_message};
  }
  return makeProgressResult(line);
}

template <typename Command, typename EmitFn, typename SubmitFn>
ExecutionDispatchResult
dispatchBuiltCommand(const Command &cmd, IExecutionSink &sink,
                     IRuntime &runtime, EmitFn emit, SubmitFn submit,
                     const std::string &pending_msg) {
  emit(sink, cmd);
  return makeRuntimeDispatchResult(cmd.source.line, submit(runtime, cmd),
                                   pending_msg);
}

} // namespace

ExecutionDispatchResult
dispatchExecutionInstruction(const AilInstruction &instruction, int line,
                             const ExecutionModalState &modal_state,
                             IExecutionSink &sink, IRuntime &runtime) {
  if (std::holds_alternative<AilLinearMoveInstruction>(instruction)) {
    const auto &inst = std::get<AilLinearMoveInstruction>(instruction);
    LinearMoveCommand cmd = buildLinearMoveCommand(inst, line, modal_state);
    return dispatchBuiltCommand(
        cmd, sink, runtime,
        [](IExecutionSink &event_sink, const LinearMoveCommand &command) {
          event_sink.onLinearMove(command);
        },
        [](IRuntime &event_runtime, const LinearMoveCommand &command) {
          return event_runtime.submitLinearMove(command);
        },
        "linear move in progress");
  }
  if (std::holds_alternative<AilArcMoveInstruction>(instruction)) {
    const auto &inst = std::get<AilArcMoveInstruction>(instruction);
    ArcMoveCommand cmd = buildArcMoveCommand(inst, line, modal_state);
    return dispatchBuiltCommand(
        cmd, sink, runtime,
        [](IExecutionSink &event_sink, const ArcMoveCommand &command) {
          event_sink.onArcMove(command);
        },
        [](IRuntime &event_runtime, const ArcMoveCommand &command) {
          return event_runtime.submitArcMove(command);
        },
        "arc move in progress");
  }
  if (std::holds_alternative<AilDwellInstruction>(instruction)) {
    const auto &inst = std::get<AilDwellInstruction>(instruction);
    DwellCommand cmd = buildDwellCommand(inst, line, modal_state);
    return dispatchBuiltCommand(
        cmd, sink, runtime,
        [](IExecutionSink &event_sink, const DwellCommand &command) {
          event_sink.onDwell(command);
        },
        [](IRuntime &event_runtime, const DwellCommand &command) {
          return event_runtime.submitDwell(command);
        },
        "dwell in progress");
  }
  return {};
}

} // namespace gcode
