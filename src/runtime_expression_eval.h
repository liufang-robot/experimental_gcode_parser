#pragma once

#include "gcode/ast.h"
#include "gcode/condition_runtime.h"
#include "gcode/execution_interfaces.h"

namespace gcode {

RuntimeResult<double> evaluateRuntimeExpression(const ExprNode &node,
                                                IRuntime &runtime);

ConditionResolution resolveRuntimeCondition(const Condition &condition,
                                            IRuntime &runtime);

} // namespace gcode
