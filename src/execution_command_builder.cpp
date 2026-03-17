#include "execution_command_builder.h"

#include <type_traits>

namespace gcode {

SourceRef toSourceRef(const SourceInfo &source) {
  SourceRef ref;
  ref.filename = source.filename;
  ref.line = source.line;
  ref.line_number = source.line_number;
  return ref;
}

LinearMoveCommand buildLinearMoveCommand(const AilLinearMoveInstruction &inst,
                                         int line,
                                         const ExecutionModalState &state) {
  LinearMoveCommand cmd;
  cmd.source = toSourceRef(inst.source);
  cmd.source.line = line;
  cmd.target.x = inst.target_pose.x;
  cmd.target.y = inst.target_pose.y;
  cmd.target.z = inst.target_pose.z;
  cmd.target.a = inst.target_pose.a;
  cmd.target.b = inst.target_pose.b;
  cmd.target.c = inst.target_pose.c;
  cmd.feed = inst.feed;
  cmd.effective = state;
  return cmd;
}

ArcMoveCommand buildArcMoveCommand(const AilArcMoveInstruction &inst, int line,
                                   const ExecutionModalState &state) {
  ArcMoveCommand cmd;
  cmd.source = toSourceRef(inst.source);
  cmd.source.line = line;
  cmd.clockwise = inst.clockwise;
  cmd.target.x = inst.target_pose.x;
  cmd.target.y = inst.target_pose.y;
  cmd.target.z = inst.target_pose.z;
  cmd.target.a = inst.target_pose.a;
  cmd.target.b = inst.target_pose.b;
  cmd.target.c = inst.target_pose.c;
  cmd.arc = inst.arc;
  cmd.feed = inst.feed;
  cmd.effective = state;
  return cmd;
}

DwellCommand buildDwellCommand(const AilDwellInstruction &inst, int line,
                               const ExecutionModalState &state) {
  DwellCommand cmd;
  cmd.source = toSourceRef(inst.source);
  cmd.source.line = line;
  cmd.dwell_mode = inst.dwell_mode;
  cmd.dwell_value = inst.dwell_value;
  cmd.effective = state;
  return cmd;
}

ToolChangeCommand
buildToolChangeCommand(const SourceInfo &source, int line,
                       const ToolSelectionState &target_tool_selection,
                       ExecutionModalState state) {
  ToolChangeCommand cmd;
  cmd.source = toSourceRef(source);
  cmd.source.line = line;
  cmd.target_tool_selection = target_tool_selection;
  state.selected_tool_selection = target_tool_selection;
  cmd.effective = std::move(state);
  return cmd;
}

ModalUpdateEvent buildModalUpdateEvent(const AilInstruction &instruction) {
  ModalUpdateEvent event;
  std::visit(
      [&event](const auto &inst) {
        event.source = toSourceRef(inst.source);
        if constexpr (std::is_same_v<std::decay_t<decltype(inst)>,
                                     AilRapidTraverseModeInstruction>) {
          event.changes.rapid_mode = inst.mode;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(inst)>,
                                            AilToolRadiusCompInstruction>) {
          event.changes.tool_radius_comp = inst.mode;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(inst)>,
                                            AilWorkingPlaneInstruction>) {
          event.changes.working_plane = inst.plane;
        }
      },
      instruction);
  return event;
}

} // namespace gcode
