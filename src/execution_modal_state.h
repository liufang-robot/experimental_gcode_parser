#pragma once

#include <optional>

#include "gcode/ail.h"

namespace gcode {

struct ExecutionModalState {
  std::optional<WorkingPlane> working_plane = WorkingPlane::XY;
  std::optional<RapidInterpolationMode> rapid_mode;
  std::optional<ToolRadiusCompMode> tool_radius_comp = ToolRadiusCompMode::Off;
};

ExecutionModalState
makeExecutionModalState(std::optional<WorkingPlane> working_plane,
                        std::optional<RapidInterpolationMode> rapid_mode,
                        std::optional<ToolRadiusCompMode> tool_radius_comp);

bool applyExecutionModalInstruction(
    const AilInstruction &instruction,
    std::optional<WorkingPlane> *working_plane,
    std::optional<RapidInterpolationMode> *rapid_mode,
    std::optional<ToolRadiusCompMode> *tool_radius_comp);

} // namespace gcode
