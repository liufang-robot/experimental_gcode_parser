#pragma once

#include <string>

#include "execution_modal_state.h"
#include "gcode/execution_commands.h"

namespace gcode {

SourceRef toSourceRef(const SourceInfo &source);
LinearMoveCommand buildLinearMoveCommand(const AilLinearMoveInstruction &inst,
                                         int line,
                                         const ExecutionModalState &state);
ArcMoveCommand buildArcMoveCommand(const AilArcMoveInstruction &inst, int line,
                                   const ExecutionModalState &state);
DwellCommand buildDwellCommand(const AilDwellInstruction &inst, int line,
                               const ExecutionModalState &state);
ToolChangeCommand
buildToolChangeCommand(const SourceInfo &source, int line,
                       const ToolSelectionState &target_tool_selection,
                       ExecutionModalState state);
ModalUpdateEvent buildModalUpdateEvent(const AilInstruction &instruction);

} // namespace gcode
