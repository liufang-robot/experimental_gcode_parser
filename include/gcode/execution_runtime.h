#pragma once

#include "gcode/condition_runtime.h"
#include "gcode/execution_interfaces.h"

namespace gcode {

class IExecutionRuntime : public IRuntime, public IConditionResolver {
public:
  ~IExecutionRuntime() override = default;
};

} // namespace gcode
