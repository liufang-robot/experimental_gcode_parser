#include "execution_modal_state.h"

namespace gcode {

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
  return state;
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
