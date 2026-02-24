#include "messages_json.h"

#include <nlohmann/json.hpp>

namespace gcode {
namespace {

std::string severityToString(Diagnostic::Severity severity) {
  return severity == Diagnostic::Severity::Error ? "error" : "warning";
}

Diagnostic::Severity severityFromString(const std::string &severity) {
  if (severity == "warning") {
    return Diagnostic::Severity::Warning;
  }
  return Diagnostic::Severity::Error;
}

nlohmann::json locationToJson(const Location &location) {
  nlohmann::json j;
  j["line"] = location.line;
  j["column"] = location.column;
  return j;
}

Location locationFromJson(const nlohmann::json &j) {
  Location location;
  location.line = j.value("line", 0);
  location.column = j.value("column", 0);
  return location;
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

SourceInfo sourceFromJson(const nlohmann::json &j) {
  SourceInfo source;
  if (j.contains("filename") && !j["filename"].is_null()) {
    source.filename = j["filename"].get<std::string>();
  }
  source.line = j.value("line", 0);
  if (j.contains("line_number") && !j["line_number"].is_null()) {
    source.line_number = j["line_number"].get<int>();
  }
  return source;
}

nlohmann::json optionalDoubleToJson(const std::optional<double> &value) {
  return value.has_value() ? nlohmann::json(*value) : nlohmann::json(nullptr);
}

std::string dwellModeToString(DwellMode mode) {
  return mode == DwellMode::Revolutions ? "revolutions" : "seconds";
}

DwellMode dwellModeFromString(const std::string &mode) {
  if (mode == "revolutions") {
    return DwellMode::Revolutions;
  }
  return DwellMode::Seconds;
}

std::optional<double> optionalDoubleFromJson(const nlohmann::json &j,
                                             const char *key) {
  if (!j.contains(key) || j[key].is_null()) {
    return std::nullopt;
  }
  return j[key].get<double>();
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

Pose6 poseFromJson(const nlohmann::json &j) {
  Pose6 pose;
  pose.x = optionalDoubleFromJson(j, "x");
  pose.y = optionalDoubleFromJson(j, "y");
  pose.z = optionalDoubleFromJson(j, "z");
  pose.a = optionalDoubleFromJson(j, "a");
  pose.b = optionalDoubleFromJson(j, "b");
  pose.c = optionalDoubleFromJson(j, "c");
  return pose;
}

nlohmann::json arcToJson(const ArcParams &arc) {
  nlohmann::json j;
  j["i"] = optionalDoubleToJson(arc.i);
  j["j"] = optionalDoubleToJson(arc.j);
  j["k"] = optionalDoubleToJson(arc.k);
  j["r"] = optionalDoubleToJson(arc.r);
  return j;
}

ArcParams arcFromJson(const nlohmann::json &j) {
  ArcParams arc;
  arc.i = optionalDoubleFromJson(j, "i");
  arc.j = optionalDoubleFromJson(j, "j");
  arc.k = optionalDoubleFromJson(j, "k");
  arc.r = optionalDoubleFromJson(j, "r");
  return arc;
}

nlohmann::json diagnosticToJson(const Diagnostic &diag) {
  nlohmann::json j;
  j["severity"] = severityToString(diag.severity);
  j["message"] = diag.message;
  j["location"] = locationToJson(diag.location);
  return j;
}

Diagnostic diagnosticFromJson(const nlohmann::json &j) {
  Diagnostic diag;
  diag.severity = severityFromString(j.value("severity", "error"));
  diag.message = j.value("message", "");
  if (j.contains("location")) {
    diag.location = locationFromJson(j["location"]);
  }
  return diag;
}

nlohmann::json messageToJson(const ParsedMessage &message) {
  nlohmann::json j;
  if (std::holds_alternative<G1Message>(message)) {
    const auto &g1 = std::get<G1Message>(message);
    j["type"] = "G1";
    j["source"] = sourceToJson(g1.source);
    j["target_pose"] = poseToJson(g1.target_pose);
    j["feed"] = optionalDoubleToJson(g1.feed);
  } else if (std::holds_alternative<G2Message>(message)) {
    const auto &g2 = std::get<G2Message>(message);
    j["type"] = "G2";
    j["source"] = sourceToJson(g2.source);
    j["target_pose"] = poseToJson(g2.target_pose);
    j["arc"] = arcToJson(g2.arc);
    j["feed"] = optionalDoubleToJson(g2.feed);
  } else if (std::holds_alternative<G3Message>(message)) {
    const auto &g3 = std::get<G3Message>(message);
    j["type"] = "G3";
    j["source"] = sourceToJson(g3.source);
    j["target_pose"] = poseToJson(g3.target_pose);
    j["arc"] = arcToJson(g3.arc);
    j["feed"] = optionalDoubleToJson(g3.feed);
  } else if (std::holds_alternative<G4Message>(message)) {
    const auto &g4 = std::get<G4Message>(message);
    j["type"] = "G4";
    j["source"] = sourceToJson(g4.source);
    j["dwell_mode"] = dwellModeToString(g4.dwell_mode);
    j["dwell_value"] = g4.dwell_value;
  }
  return j;
}

ParsedMessage messageFromJson(const nlohmann::json &j) {
  const auto type = j.value("type", "");
  if (type == "G1") {
    G1Message g1;
    if (j.contains("source")) {
      g1.source = sourceFromJson(j["source"]);
    }
    if (j.contains("target_pose")) {
      g1.target_pose = poseFromJson(j["target_pose"]);
    }
    g1.feed = optionalDoubleFromJson(j, "feed");
    return g1;
  } else if (type == "G2") {
    G2Message g2;
    if (j.contains("source")) {
      g2.source = sourceFromJson(j["source"]);
    }
    if (j.contains("target_pose")) {
      g2.target_pose = poseFromJson(j["target_pose"]);
    }
    if (j.contains("arc")) {
      g2.arc = arcFromJson(j["arc"]);
    }
    g2.feed = optionalDoubleFromJson(j, "feed");
    return g2;
  } else if (type == "G3") {
    G3Message g3;
    if (j.contains("source")) {
      g3.source = sourceFromJson(j["source"]);
    }
    if (j.contains("target_pose")) {
      g3.target_pose = poseFromJson(j["target_pose"]);
    }
    if (j.contains("arc")) {
      g3.arc = arcFromJson(j["arc"]);
    }
    g3.feed = optionalDoubleFromJson(j, "feed");
    return g3;
  } else if (type == "G4") {
    G4Message g4;
    if (j.contains("source")) {
      g4.source = sourceFromJson(j["source"]);
    }
    g4.dwell_mode = dwellModeFromString(j.value("dwell_mode", "seconds"));
    g4.dwell_value = j.value("dwell_value", 0.0);
    return g4;
  }
  return G1Message{};
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

MessageResult::RejectedLine rejectedLineFromJson(const nlohmann::json &j) {
  MessageResult::RejectedLine line;
  if (j.contains("source")) {
    line.source = sourceFromJson(j["source"]);
  }
  if (j.contains("reasons")) {
    for (const auto &reason : j["reasons"]) {
      line.reasons.push_back(diagnosticFromJson(reason));
    }
  }
  return line;
}

nlohmann::json toJson(const MessageResult &result) {
  nlohmann::json j;
  j["schema_version"] = 1;
  j["messages"] = nlohmann::json::array();
  for (const auto &message : result.messages) {
    j["messages"].push_back(messageToJson(message));
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

MessageResult fromJson(const nlohmann::json &j) {
  MessageResult result;
  if (j.contains("messages")) {
    for (const auto &message : j["messages"]) {
      result.messages.push_back(messageFromJson(message));
    }
  }
  if (j.contains("diagnostics")) {
    for (const auto &diag : j["diagnostics"]) {
      result.diagnostics.push_back(diagnosticFromJson(diag));
    }
  }
  if (j.contains("rejected_lines")) {
    for (const auto &line : j["rejected_lines"]) {
      result.rejected_lines.push_back(rejectedLineFromJson(line));
    }
  }
  return result;
}

} // namespace

std::string toJsonString(const MessageResult &result, bool pretty) {
  const auto j = toJson(result);
  return pretty ? j.dump(2) : j.dump();
}

MessageResult fromJsonString(const std::string &json_text) {
  try {
    const auto j = nlohmann::json::parse(json_text);
    return fromJson(j);
  } catch (const std::exception &ex) {
    MessageResult result;
    Diagnostic diag;
    diag.severity = Diagnostic::Severity::Error;
    diag.message = std::string("json parse error: ") + ex.what();
    diag.location = {0, 0};
    result.diagnostics.push_back(std::move(diag));
    return result;
  }
}

} // namespace gcode
