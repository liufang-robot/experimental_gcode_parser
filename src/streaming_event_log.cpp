#include "streaming_event_log.h"

#include <iomanip>
#include <sstream>

namespace gcode {
namespace {

nlohmann::json sourceToJson(const SourceRef &source) {
  nlohmann::json j;
  j["filename"] = source.filename.has_value() ? nlohmann::json(*source.filename)
                                              : nlohmann::json(nullptr);
  j["line"] = source.line;
  j["line_number"] = source.line_number.has_value()
                         ? nlohmann::json(*source.line_number)
                         : nlohmann::json(nullptr);
  return j;
}

nlohmann::json optionalDoubleToJson(const std::optional<double> &value) {
  return value.has_value() ? nlohmann::json(*value) : nlohmann::json(nullptr);
}

nlohmann::json poseTargetToJson(const PoseTarget &target) {
  return {{"x", optionalDoubleToJson(target.x)},
          {"y", optionalDoubleToJson(target.y)},
          {"z", optionalDoubleToJson(target.z)},
          {"a", optionalDoubleToJson(target.a)},
          {"b", optionalDoubleToJson(target.b)},
          {"c", optionalDoubleToJson(target.c)}};
}

nlohmann::json
toolSelectionToJson(const std::optional<ToolSelectionState> &selection) {
  if (!selection.has_value()) {
    return nullptr;
  }
  nlohmann::json j;
  j["selector_index"] = selection->selector_index.has_value()
                            ? nlohmann::json(*selection->selector_index)
                            : nlohmann::json(nullptr);
  j["selector_value"] = selection->selector_value;
  return j;
}

nlohmann::json effectiveModalToJson(const EffectiveModalSnapshot &effective) {
  return {{"motion_code", effective.motion_code},
          {"working_plane",
           effective.working_plane == WorkingPlane::XY
               ? "xy"
               : (effective.working_plane == WorkingPlane::ZX ? "zx" : "yz")},
          {"rapid_mode", effective.rapid_mode == RapidInterpolationMode::Linear
                             ? "linear"
                             : "nonlinear"},
          {"tool_radius_comp",
           effective.tool_radius_comp == ToolRadiusCompMode::Off
               ? "off"
               : (effective.tool_radius_comp == ToolRadiusCompMode::Left
                      ? "left"
                      : "right")},
          {"active_tool_selection",
           toolSelectionToJson(effective.active_tool_selection)},
          {"pending_tool_selection",
           toolSelectionToJson(effective.pending_tool_selection)}};
}

nlohmann::json linearMoveToJson(const LinearMoveCommand &cmd) {
  nlohmann::json j;
  j["source"] = sourceToJson(cmd.source);
  j["target"] = poseTargetToJson(cmd.target);
  j["feed"] = optionalDoubleToJson(cmd.feed);
  j["effective"] = effectiveModalToJson(cmd.effective);
  return j;
}

nlohmann::json arcMoveToJson(const ArcMoveCommand &cmd) {
  nlohmann::json j;
  j["source"] = sourceToJson(cmd.source);
  j["clockwise"] = cmd.clockwise;
  j["target"] = poseTargetToJson(cmd.target);
  j["arc"] = {{"i", optionalDoubleToJson(cmd.arc.i)},
              {"j", optionalDoubleToJson(cmd.arc.j)},
              {"k", optionalDoubleToJson(cmd.arc.k)},
              {"r", optionalDoubleToJson(cmd.arc.r)}};
  j["feed"] = optionalDoubleToJson(cmd.feed);
  j["effective"] = effectiveModalToJson(cmd.effective);
  return j;
}

nlohmann::json dwellToJson(const DwellCommand &cmd) {
  nlohmann::json j;
  j["source"] = sourceToJson(cmd.source);
  j["dwell_mode"] =
      cmd.dwell_mode == DwellMode::Revolutions ? "revolutions" : "seconds";
  j["dwell_value"] = cmd.dwell_value;
  j["effective"] = effectiveModalToJson(cmd.effective);
  return j;
}

nlohmann::json diagnosticToJson(const Diagnostic &diag) {
  nlohmann::json j;
  j["severity"] =
      diag.severity == Diagnostic::Severity::Error ? "error" : "warning";
  j["message"] = diag.message;
  j["location"] = {{"line", diag.location.line},
                   {"column", diag.location.column}};
  return j;
}

RuntimeResult<WaitToken> readyWaitResult() {
  RuntimeResult<WaitToken> result;
  result.status = RuntimeCallStatus::Ready;
  return result;
}

} // namespace

void EventLogRecorder::add(nlohmann::json entry) {
  entry["seq"] = entries_.size() + 1;
  entries_.push_back(std::move(entry));
}

std::string EventLogRecorder::toJsonLines() const {
  std::ostringstream out;
  for (const auto &entry : entries_) {
    out << entry.dump() << "\n";
  }
  return out.str();
}

std::string EventLogRecorder::toDebugText() const {
  std::ostringstream out;
  for (size_t i = 0; i < entries_.size(); ++i) {
    const auto &entry = entries_[i];
    const std::string event = entry.value("event", "");
    if ((event == "sink.linear_move" || event == "sink.arc_move" ||
         event == "sink.dwell") &&
        i + 1 < entries_.size()) {
      const auto &next = entries_[i + 1];
      const std::string next_event = next.value("event", "");
      const bool is_matching_runtime =
          (event == "sink.linear_move" &&
           next_event == "runtime.submit_linear_move") ||
          (event == "sink.arc_move" &&
           next_event == "runtime.submit_arc_move") ||
          (event == "sink.dwell" && next_event == "runtime.submit_dwell");
      if (is_matching_runtime) {
        const auto &params = entry["params"];
        const auto &source = params["source"];
        out << "line " << entry["line"].get<int>();
        if (!source["line_number"].is_null()) {
          out << " n=" << source["line_number"].get<int>();
        }
        out << "\n";
        if (event == "sink.linear_move") {
          out << "  emit linear_move:";
          const auto &effective = params["effective"];
          out << " motion=" << effective["motion_code"].get<std::string>();
          out << " plane=" << effective["working_plane"].get<std::string>();
          out << " comp=" << effective["tool_radius_comp"].get<std::string>();
          out << " rapid=" << effective["rapid_mode"].get<std::string>();
          if (!effective["active_tool_selection"].is_null()) {
            out << " active_tool="
                << effective["active_tool_selection"]["selector_value"]
                       .get<std::string>();
          }
          if (!effective["pending_tool_selection"].is_null()) {
            out << " pending_tool="
                << effective["pending_tool_selection"]["selector_value"]
                       .get<std::string>();
          }
          out << "\n";
          out << "  target:";
          const auto &target = params["target"];
          if (!target["x"].is_null()) {
            out << " x=" << target["x"].get<double>();
          }
          if (!target["y"].is_null()) {
            out << " y=" << target["y"].get<double>();
          }
          if (!target["z"].is_null()) {
            out << " z=" << target["z"].get<double>();
          }
          if (!params["feed"].is_null()) {
            out << " feed=" << params["feed"].get<double>();
          }
          out << "\n";
          out << "  runtime: submit_linear_move";
        } else if (event == "sink.arc_move") {
          out << "  emit arc_move:";
          const auto &effective = params["effective"];
          out << " motion=" << effective["motion_code"].get<std::string>();
          out << " plane=" << effective["working_plane"].get<std::string>();
          out << "\n";
          out << "  runtime: submit_arc_move";
        } else {
          out << "  emit dwell:";
          out << " mode=" << params.value("dwell_mode", "");
          out << " value=" << params.value("dwell_value", 0.0);
          out << "\n";
          out << "  runtime: submit_dwell";
        }
        out << "\n";
        ++i;
        continue;
      }
    }

    out << entry.value("seq", 0) << " " << event;
    if (entry.contains("line")) {
      out << " line=" << entry["line"].get<int>();
    }
    if (event == "runtime.submit_linear_move" ||
        event == "runtime.submit_arc_move" || event == "runtime.submit_dwell") {
      out << "\n";
      out << "  paired_runtime_event_without_sink";
    } else if (event == "diagnostic") {
      out << "\n";
      out << "  diagnostic: msg=\"" << entry["message"].get<std::string>()
          << "\"";
    } else if (event == "rejected_line") {
      const auto &source = entry["params"]["source"];
      out << "\n";
      out << "  source:";
      if (!source["filename"].is_null()) {
        out << " file=" << source["filename"].get<std::string>();
      }
      if (!source["line_number"].is_null()) {
        out << " n=" << source["line_number"].get<int>();
      }
      const auto &reasons = entry["params"]["reasons"];
      out << "\n";
      out << "  rejected: errors=" << reasons.size();
      if (!reasons.empty()) {
        out << " first=\"" << reasons[0]["message"].get<std::string>() << "\"";
      }
    } else if (entry.contains("message")) {
      out << "\n";
      out << "  msg=\"" << entry["message"].get<std::string>() << "\"";
    } else if (entry.contains("params")) {
      const auto &params = entry["params"];
      out << "\n";
      out << "  params=" << params.dump();
    }
    out << "\n";
  }
  return out.str();
}

void RecordingExecutionSink::onDiagnostic(const Diagnostic &diag) {
  recorder_.add({{"event", "diagnostic"},
                 {"line", diag.location.line},
                 {"message", diag.message},
                 {"params", diagnosticToJson(diag)}});
}

void RecordingExecutionSink::onRejectedLine(const RejectedLineEvent &event) {
  nlohmann::json reasons = nlohmann::json::array();
  for (const auto &diag : event.reasons) {
    reasons.push_back(diagnosticToJson(diag));
  }
  recorder_.add(
      {{"event", "rejected_line"},
       {"line", event.source.line},
       {"params",
        {{"source", sourceToJson(event.source)}, {"reasons", reasons}}}});
}

void RecordingExecutionSink::onLinearMove(const LinearMoveCommand &cmd) {
  recorder_.add({{"event", "sink.linear_move"},
                 {"line", cmd.source.line},
                 {"params", linearMoveToJson(cmd)}});
}

void RecordingExecutionSink::onArcMove(const ArcMoveCommand &cmd) {
  recorder_.add({{"event", "sink.arc_move"},
                 {"line", cmd.source.line},
                 {"params", arcMoveToJson(cmd)}});
}

void RecordingExecutionSink::onDwell(const DwellCommand &cmd) {
  recorder_.add({{"event", "sink.dwell"},
                 {"line", cmd.source.line},
                 {"params", dwellToJson(cmd)}});
}

RuntimeResult<WaitToken>
ReadyRuntimeRecorder::submitLinearMove(const LinearMoveCommand &cmd) {
  recorder_.add({{"event", "runtime.submit_linear_move"},
                 {"line", cmd.source.line},
                 {"params", linearMoveToJson(cmd)}});
  return readyWaitResult();
}

RuntimeResult<WaitToken>
ReadyRuntimeRecorder::submitArcMove(const ArcMoveCommand &cmd) {
  recorder_.add({{"event", "runtime.submit_arc_move"},
                 {"line", cmd.source.line},
                 {"params", arcMoveToJson(cmd)}});
  return readyWaitResult();
}

RuntimeResult<WaitToken>
ReadyRuntimeRecorder::submitDwell(const DwellCommand &cmd) {
  recorder_.add({{"event", "runtime.submit_dwell"},
                 {"line", cmd.source.line},
                 {"params", dwellToJson(cmd)}});
  return readyWaitResult();
}

RuntimeResult<double>
ReadyRuntimeRecorder::readSystemVariable(std::string_view name) {
  recorder_.add({{"event", "runtime.read_system_variable"},
                 {"params", {{"name", std::string(name)}}}});
  RuntimeResult<double> result;
  result.status = RuntimeCallStatus::Error;
  result.error_message = "fake runtime does not resolve system variables";
  return result;
}

RuntimeResult<WaitToken>
ReadyRuntimeRecorder::cancelWait(const WaitToken &token) {
  recorder_.add({{"event", "runtime.cancel_wait"},
                 {"params", {{"kind", token.kind}, {"id", token.id}}}});
  return readyWaitResult();
}

} // namespace gcode
