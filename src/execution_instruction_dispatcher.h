#pragma once

#include <optional>
#include <string>

#include "execution_command_builder.h"
#include "gcode/execution_interfaces.h"

namespace gcode {

struct ExecutionDispatchResult {
  enum class Status { NotHandled, Progress, Blocked, Error };

  Status status = Status::NotHandled;
  int line = 0;
  std::optional<WaitToken> wait_token;
  std::string message;
};

ExecutionDispatchResult
dispatchExecutionInstruction(const AilInstruction &instruction, int line,
                             const ExecutionModalState &modal_state,
                             IExecutionSink &sink, IRuntime &runtime);

} // namespace gcode
