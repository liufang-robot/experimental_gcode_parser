#pragma once

#include <string>

#include "execution_modal_state.h"
#include "gcode/execution_commands.h"

namespace gcode {

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
