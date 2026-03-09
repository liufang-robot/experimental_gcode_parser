#include "execution_command_builder.h"

namespace gcode {

SourceRef toSourceRef(const SourceInfo &source) {
  SourceRef ref;
  ref.filename = source.filename;
  ref.line = source.line;
  ref.line_number = source.line_number;
  return ref;
}

EffectiveModalState makeEffectiveModalState(const std::string &motion_code,
                                            const ExecutionModalState &state) {
  EffectiveModalState effective;
  effective.motion_code = motion_code;
  effective.working_plane = state.working_plane;
  effective.rapid_mode = state.rapid_mode;
  effective.tool_radius_comp = state.tool_radius_comp;
  return effective;
}

LinearMoveCommand buildLinearMoveCommand(const AilLinearMoveInstruction &inst,
                                         int line,
                                         const ExecutionModalState &state) {
  LinearMoveCommand cmd;
  cmd.source = toSourceRef(inst.source);
  cmd.source.line = line;
  cmd.opcode = inst.opcode;
  cmd.x = inst.target_pose.x;
  cmd.y = inst.target_pose.y;
  cmd.z = inst.target_pose.z;
  cmd.a = inst.target_pose.a;
  cmd.b = inst.target_pose.b;
  cmd.c = inst.target_pose.c;
  cmd.feed = inst.feed;
  cmd.modal = inst.modal;
  cmd.effective = makeEffectiveModalState(cmd.opcode, state);
  return cmd;
}

ArcMoveCommand buildArcMoveCommand(const AilArcMoveInstruction &inst, int line,
                                   const ExecutionModalState &state) {
  ArcMoveCommand cmd;
  cmd.source = toSourceRef(inst.source);
  cmd.source.line = line;
  cmd.opcode = inst.modal.code;
  cmd.clockwise = inst.clockwise;
  cmd.plane_effective = inst.plane_effective;
  cmd.x = inst.target_pose.x;
  cmd.y = inst.target_pose.y;
  cmd.z = inst.target_pose.z;
  cmd.a = inst.target_pose.a;
  cmd.b = inst.target_pose.b;
  cmd.c = inst.target_pose.c;
  cmd.arc = inst.arc;
  cmd.feed = inst.feed;
  cmd.modal = inst.modal;
  cmd.effective = makeEffectiveModalState(cmd.opcode, state);
  return cmd;
}

DwellCommand buildDwellCommand(const AilDwellInstruction &inst, int line,
                               const ExecutionModalState &state) {
  DwellCommand cmd;
  cmd.source = toSourceRef(inst.source);
  cmd.source.line = line;
  cmd.dwell_mode = inst.dwell_mode;
  cmd.dwell_value = inst.dwell_value;
  cmd.modal = inst.modal;
  cmd.effective = makeEffectiveModalState(cmd.modal.code, state);
  return cmd;
}

} // namespace gcode
