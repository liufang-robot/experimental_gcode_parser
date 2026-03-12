#pragma once

#include <string>

#include "gcode/execution_commands.h"

namespace gcode {

using ExecutionModalState = EffectiveModalSnapshot;

ExecutionModalState makeExecutionModalState(
    std::string motion_code, WorkingPlane working_plane,
    RapidInterpolationMode rapid_mode, ToolRadiusCompMode tool_radius_comp,
    std::optional<ToolSelectionState> active_tool_selection,
    std::optional<ToolSelectionState> pending_tool_selection);

bool applyExecutionModalInstruction(const AilInstruction &instruction,
                                    WorkingPlane *working_plane,
                                    RapidInterpolationMode *rapid_mode,
                                    ToolRadiusCompMode *tool_radius_comp);

} // namespace gcode
