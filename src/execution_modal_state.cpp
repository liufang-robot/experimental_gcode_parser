#include "execution_modal_state.h"

namespace gcode {

ExecutionModalState
makeExecutionModalState(std::optional<WorkingPlane> working_plane,
                        std::optional<RapidInterpolationMode> rapid_mode,
                        std::optional<ToolRadiusCompMode> tool_radius_comp) {
  ExecutionModalState state;
  state.working_plane = working_plane;
  state.rapid_mode = rapid_mode;
  state.tool_radius_comp = tool_radius_comp;
  return state;
}

bool applyExecutionModalInstruction(
    const AilInstruction &instruction,
    std::optional<WorkingPlane> *working_plane,
    std::optional<RapidInterpolationMode> *rapid_mode,
    std::optional<ToolRadiusCompMode> *tool_radius_comp) {
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
