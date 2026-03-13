#pragma once

#include <string>

#include "gcode/execution_commands.h"

namespace gcode {

struct AilExecutorInitialState;
struct ExecutorState;

using ExecutionModalState = EffectiveModalSnapshot;

void applyExecutionInitialState(
    ExecutorState *state,
    const std::optional<AilExecutorInitialState> &initial_state);

ExecutionModalState makeExecutionModalState(
    std::string motion_code, WorkingPlane working_plane,
    RapidInterpolationMode rapid_mode, ToolRadiusCompMode tool_radius_comp,
    std::optional<ToolSelectionState> active_tool_selection,
    std::optional<ToolSelectionState> pending_tool_selection);

ExecutionModalState makeExecutionModalState(
    const ExecutorState &state,
    std::optional<std::string> motion_code_override = std::nullopt);

bool applyExecutionModalInstruction(const AilInstruction &instruction,
                                    WorkingPlane *working_plane,
                                    RapidInterpolationMode *rapid_mode,
                                    ToolRadiusCompMode *tool_radius_comp);

} // namespace gcode
