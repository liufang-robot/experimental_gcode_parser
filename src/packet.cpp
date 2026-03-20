#include "packet.h"

#include <type_traits>

namespace gcode {
namespace {

Diagnostic makeSkippedInstructionWarning(const SourceInfo &source,
                                         const char *kind) {
  Diagnostic diag;
  diag.severity = Diagnostic::Severity::Warning;
  diag.message =
      std::string("packetization skipped non-motion instruction: ") + kind;
  diag.location = {source.line, 1};
  return diag;
}

} // namespace

PacketResult lowerAilToPackets(const AilResult &ail_result) {
  PacketResult result;
  result.diagnostics = ail_result.diagnostics;
  result.rejected_lines = ail_result.rejected_lines;

  uint64_t next_packet_id = 1;
  result.packets.reserve(ail_result.instructions.size());

  for (const auto &instruction : ail_result.instructions) {
    std::visit(
        [&](const auto &inst) {
          using T = std::decay_t<decltype(inst)>;
          if constexpr (std::is_same_v<T, AilLinearMoveInstruction>) {
            MotionPacket packet;
            packet.packet_id = next_packet_id++;
            packet.type = PacketType::LinearMove;
            packet.source = inst.source;
            packet.modal = inst.modal;
            packet.payload = MotionLinearPayload{
                inst.target_pose, inst.target_system_variables, inst.feed,
                inst.rapid_mode_effective};
            result.packets.push_back(std::move(packet));
          } else if constexpr (std::is_same_v<T, AilArcMoveInstruction>) {
            MotionPacket packet;
            packet.packet_id = next_packet_id++;
            packet.type = PacketType::ArcMove;
            packet.source = inst.source;
            packet.modal = inst.modal;
            packet.payload =
                MotionArcPayload{inst.clockwise, inst.plane_effective,
                                 inst.target_pose, inst.arc, inst.feed};
            result.packets.push_back(std::move(packet));
          } else if constexpr (std::is_same_v<T, AilDwellInstruction>) {
            MotionPacket packet;
            packet.packet_id = next_packet_id++;
            packet.type = PacketType::Dwell;
            packet.source = inst.source;
            packet.modal = inst.modal;
            packet.payload = DwellPayload{inst.dwell_mode, inst.dwell_value};
            result.packets.push_back(std::move(packet));
          } else if constexpr (std::is_same_v<T, AilAssignInstruction>) {
            result.diagnostics.push_back(
                makeSkippedInstructionWarning(inst.source, "assign"));
          } else if constexpr (std::is_same_v<T, AilMCodeInstruction>) {
            result.diagnostics.push_back(
                makeSkippedInstructionWarning(inst.source, "m_function"));
          } else if constexpr (std::is_same_v<
                                   T, AilRapidTraverseModeInstruction>) {
            // Rapid-mode instructions influence G0 payload metadata and do not
            // produce standalone packets.
          } else if constexpr (std::is_same_v<T,
                                              AilToolRadiusCompInstruction>) {
            // Tool-radius-compensation instructions are modal state and do not
            // produce standalone packets.
          } else if constexpr (std::is_same_v<T, AilWorkingPlaneInstruction>) {
            // Working-plane instructions are modal state and do not produce
            // standalone packets.
          } else if constexpr (std::is_same_v<T, AilToolSelectInstruction> ||
                               std::is_same_v<T, AilToolChangeInstruction>) {
            // Tool-select/tool-change instructions are non-motion control
            // state and do not produce standalone packets.
          } else if constexpr (std::is_same_v<T,
                                              AilReturnBoundaryInstruction>) {
            // Return-boundary instructions are runtime control flow and do not
            // produce standalone motion packets.
          } else if constexpr (std::is_same_v<T,
                                              AilSubprogramCallInstruction>) {
            // Subprogram-call instructions are runtime control flow and do not
            // produce standalone motion packets.
          } else if constexpr (std::is_same_v<T, AilLabelInstruction>) {
            result.diagnostics.push_back(
                makeSkippedInstructionWarning(inst.source, "label"));
          } else if constexpr (std::is_same_v<T, AilGotoInstruction>) {
            result.diagnostics.push_back(
                makeSkippedInstructionWarning(inst.source, "goto"));
          } else if constexpr (std::is_same_v<T, AilBranchIfInstruction>) {
            result.diagnostics.push_back(
                makeSkippedInstructionWarning(inst.source, "branch_if"));
          } else {
            result.diagnostics.push_back(
                makeSkippedInstructionWarning(inst.source, "sync"));
          }
        },
        instruction);
  }

  return result;
}

PacketResult parseLowerAndPacketize(std::string_view input,
                                    const LowerOptions &options) {
  const auto ail_result = parseAndLowerAil(input, options);
  return lowerAilToPackets(ail_result);
}

} // namespace gcode
