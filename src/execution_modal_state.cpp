#include "execution_modal_state.h"

namespace gcode {

void applyExecutionInitialState(
    ExecutorState *state,
    const std::optional<AilExecutorInitialState> &initial_state) {
  if (!initial_state.has_value()) {
    return;
  }

  state->motion_code_current = initial_state->motion_code_current;
  state->rapid_mode_current = initial_state->rapid_mode_current;
  state->tool_radius_comp_current = initial_state->tool_radius_comp_current;
  state->working_plane_current = initial_state->working_plane_current;
  state->active_tool_selection = initial_state->active_tool_selection;
  state->pending_tool_selection = initial_state->pending_tool_selection;
  state->selected_tool_selection = initial_state->selected_tool_selection;
  state->user_variables = initial_state->user_variables;
}

ExecutionModalState makeExecutionModalState(
    std::string motion_code, WorkingPlane working_plane,
    RapidInterpolationMode rapid_mode, ToolRadiusCompMode tool_radius_comp,
    std::optional<ToolSelectionState> active_tool_selection,
    std::optional<ToolSelectionState> pending_tool_selection) {
  ExecutionModalState state;
  state.motion_code = std::move(motion_code);
  state.working_plane = working_plane;
  state.rapid_mode = rapid_mode;
  state.tool_radius_comp = tool_radius_comp;
  state.active_tool_selection = std::move(active_tool_selection);
  state.pending_tool_selection = std::move(pending_tool_selection);
  state.selected_tool_selection = std::nullopt;
  return state;
}

ExecutionModalState
makeExecutionModalState(const ExecutorState &state,
                        std::optional<std::string> motion_code_override) {
  ExecutionModalState snapshot = makeExecutionModalState(
      motion_code_override.value_or(state.motion_code_current),
      state.working_plane_current, state.rapid_mode_current,
      state.tool_radius_comp_current, state.active_tool_selection,
      state.pending_tool_selection);
  snapshot.selected_tool_selection = state.selected_tool_selection;
  return snapshot;
}

bool applyExecutionModalInstruction(const AilInstruction &instruction,
                                    WorkingPlane *working_plane,
                                    RapidInterpolationMode *rapid_mode,
                                    ToolRadiusCompMode *tool_radius_comp) {
  if (std::holds_alternative<AilRapidTraverseModeInstruction>(instruction)) {
    *rapid_mode = std::get<AilRapidTraverseModeInstruction>(instruction).mode;
    return true;
  }
  if (std::holds_alternative<AilToolRadiusCompInstruction>(instruction)) {
    *tool_radius_comp =
        std::get<AilToolRadiusCompInstruction>(instruction).mode;
    return true;
  }
  if (std::holds_alternative<AilWorkingPlaneInstruction>(instruction)) {
    *working_plane = std::get<AilWorkingPlaneInstruction>(instruction).plane;
    return true;
  }
  return false;
}

} // namespace gcode
