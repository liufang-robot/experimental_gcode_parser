#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "gcode/ail.h"

namespace gcode {

enum class PacketType { LinearMove, ArcMove, Dwell };

struct MotionLinearPayload {
  Pose6 target_pose;
  AxisSystemVariableRefs target_system_variables;
  std::optional<double> feed;
  std::optional<RapidInterpolationMode> rapid_mode_effective;
};

struct MotionArcPayload {
  bool clockwise = true;
  WorkingPlane plane_effective = WorkingPlane::XY;
  Pose6 target_pose;
  ArcParams arc;
  std::optional<double> feed;
};

struct DwellPayload {
  DwellMode dwell_mode = DwellMode::Seconds;
  double dwell_value = 0.0;
};

using PacketPayload =
    std::variant<MotionLinearPayload, MotionArcPayload, DwellPayload>;

struct MotionPacket {
  uint64_t packet_id = 0;
  PacketType type = PacketType::LinearMove;
  SourceInfo source;
  ModalState modal;
  PacketPayload payload;
};

struct PacketResult {
  std::vector<MotionPacket> packets;
  std::vector<Diagnostic> diagnostics;
  std::vector<RejectedLine> rejected_lines;
};

PacketResult lowerAilToPackets(const AilResult &ail_result);

PacketResult parseLowerAndPacketize(std::string_view input,
                                    const LowerOptions &options = {});

} // namespace gcode
