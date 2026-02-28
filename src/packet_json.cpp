#include "packet_json.h"

#include <type_traits>

#include <nlohmann/json.hpp>

namespace gcode {
namespace {

std::string severityToString(Diagnostic::Severity severity) {
  return severity == Diagnostic::Severity::Error ? "error" : "warning";
}

std::string packetTypeToString(PacketType type) {
  switch (type) {
  case PacketType::LinearMove:
    return "linear_move";
  case PacketType::ArcMove:
    return "arc_move";
  case PacketType::Dwell:
    return "dwell";
  }
  return "linear_move";
}

std::string modalGroupToString(ModalGroupId group) {
  return group == ModalGroupId::Motion ? "GGroup1" : "GGroup2";
}

std::string dwellModeToString(DwellMode mode) {
  return mode == DwellMode::Revolutions ? "revolutions" : "seconds";
}

nlohmann::json locationToJson(const Location &location) {
  nlohmann::json j;
  j["line"] = location.line;
  j["column"] = location.column;
  return j;
}

nlohmann::json diagnosticToJson(const Diagnostic &diag) {
  nlohmann::json j;
  j["severity"] = severityToString(diag.severity);
  j["message"] = diag.message;
  j["location"] = locationToJson(diag.location);
  return j;
}

nlohmann::json optionalDoubleToJson(const std::optional<double> &value) {
  return value.has_value() ? nlohmann::json(*value) : nlohmann::json(nullptr);
}

nlohmann::json sourceToJson(const SourceInfo &source) {
  nlohmann::json j;
  j["filename"] = source.filename.has_value() ? nlohmann::json(*source.filename)
                                              : nlohmann::json(nullptr);
  j["line"] = source.line;
  j["line_number"] = source.line_number.has_value()
                         ? nlohmann::json(*source.line_number)
                         : nlohmann::json(nullptr);
  return j;
}

nlohmann::json modalToJson(const ModalState &modal) {
  nlohmann::json j;
  j["group"] = modalGroupToString(modal.group);
  j["code"] = modal.code;
  j["updates_state"] = modal.updates_state;
  return j;
}

nlohmann::json poseToJson(const Pose6 &pose) {
  nlohmann::json j;
  j["x"] = optionalDoubleToJson(pose.x);
  j["y"] = optionalDoubleToJson(pose.y);
  j["z"] = optionalDoubleToJson(pose.z);
  j["a"] = optionalDoubleToJson(pose.a);
  j["b"] = optionalDoubleToJson(pose.b);
  j["c"] = optionalDoubleToJson(pose.c);
  return j;
}

nlohmann::json arcToJson(const ArcParams &arc) {
  nlohmann::json j;
  j["i"] = optionalDoubleToJson(arc.i);
  j["j"] = optionalDoubleToJson(arc.j);
  j["k"] = optionalDoubleToJson(arc.k);
  j["r"] = optionalDoubleToJson(arc.r);
  return j;
}

nlohmann::json rejectedLineToJson(const MessageResult::RejectedLine &line) {
  nlohmann::json j;
  j["source"] = sourceToJson(line.source);
  j["reasons"] = nlohmann::json::array();
  for (const auto &reason : line.reasons) {
    j["reasons"].push_back(diagnosticToJson(reason));
  }
  return j;
}

nlohmann::json payloadToJson(const PacketPayload &payload) {
  return std::visit(
      [](const auto &p) -> nlohmann::json {
        using T = std::decay_t<decltype(p)>;
        nlohmann::json j;
        if constexpr (std::is_same_v<T, MotionLinearPayload>) {
          j["target_pose"] = poseToJson(p.target_pose);
          j["feed"] = optionalDoubleToJson(p.feed);
        } else if constexpr (std::is_same_v<T, MotionArcPayload>) {
          j["clockwise"] = p.clockwise;
          j["target_pose"] = poseToJson(p.target_pose);
          j["arc"] = arcToJson(p.arc);
          j["feed"] = optionalDoubleToJson(p.feed);
        } else {
          j["dwell_mode"] = dwellModeToString(p.dwell_mode);
          j["dwell_value"] = p.dwell_value;
        }
        return j;
      },
      payload);
}

nlohmann::json packetToJson(const MotionPacket &packet) {
  nlohmann::json j;
  j["packet_id"] = packet.packet_id;
  j["type"] = packetTypeToString(packet.type);
  j["source"] = sourceToJson(packet.source);
  j["modal"] = modalToJson(packet.modal);
  j["payload"] = payloadToJson(packet.payload);
  return j;
}

nlohmann::json toJson(const PacketResult &result) {
  nlohmann::json j;
  j["schema_version"] = 1;
  j["packets"] = nlohmann::json::array();
  for (const auto &packet : result.packets) {
    j["packets"].push_back(packetToJson(packet));
  }

  j["diagnostics"] = nlohmann::json::array();
  for (const auto &diag : result.diagnostics) {
    j["diagnostics"].push_back(diagnosticToJson(diag));
  }

  j["rejected_lines"] = nlohmann::json::array();
  for (const auto &line : result.rejected_lines) {
    j["rejected_lines"].push_back(rejectedLineToJson(line));
  }
  return j;
}

} // namespace

std::string packetToJsonString(const PacketResult &result, bool pretty) {
  const auto j = toJson(result);
  return pretty ? j.dump(2) : j.dump();
}

} // namespace gcode
