#pragma once

#include <optional>
#include <string>

#include "gcode/execution_commands.h"

namespace gcode {

struct ExecutionModalState {
  std::optional<WorkingPlane> working_plane = WorkingPlane::XY;
  std::optional<RapidInterpolationMode> rapid_mode;
  std::optional<ToolRadiusCompMode> tool_radius_comp = ToolRadiusCompMode::Off;
};

SourceRef toSourceRef(const SourceInfo &source);
EffectiveModalState makeEffectiveModalState(const std::string &motion_code,
                                            const ExecutionModalState &state);
LinearMoveCommand buildLinearMoveCommand(const AilLinearMoveInstruction &inst,
                                         int line,
                                         const ExecutionModalState &state);
ArcMoveCommand buildArcMoveCommand(const AilArcMoveInstruction &inst, int line,
                                   const ExecutionModalState &state);
DwellCommand buildDwellCommand(const AilDwellInstruction &inst, int line,
                               const ExecutionModalState &state);

} // namespace gcode
