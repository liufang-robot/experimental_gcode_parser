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
            packet.payload = MotionLinearPayload{inst.target_pose, inst.feed};
            result.packets.push_back(std::move(packet));
          } else if constexpr (std::is_same_v<T, AilArcMoveInstruction>) {
            MotionPacket packet;
            packet.packet_id = next_packet_id++;
            packet.type = PacketType::ArcMove;
            packet.source = inst.source;
            packet.modal = inst.modal;
            packet.payload = MotionArcPayload{inst.clockwise, inst.target_pose,
                                              inst.arc, inst.feed};
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
