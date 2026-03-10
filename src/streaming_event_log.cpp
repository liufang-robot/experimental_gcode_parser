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

nlohmann::json modalToJson(const ModalState &modal) {
  nlohmann::json j;
  j["group"] = modal.group == ModalGroupId::Motion ? "GGroup1" : "GGroup2";
  j["code"] = modal.code;
  j["updates_state"] = modal.updates_state;
  return j;
}

nlohmann::json optionalDoubleToJson(const std::optional<double> &value) {
  return value.has_value() ? nlohmann::json(*value) : nlohmann::json(nullptr);
}

nlohmann::json linearMoveToJson(const LinearMoveCommand &cmd) {
  nlohmann::json j;
  j["source"] = sourceToJson(cmd.source);
  j["opcode"] = cmd.opcode;
  j["x"] = optionalDoubleToJson(cmd.x);
  j["y"] = optionalDoubleToJson(cmd.y);
  j["z"] = optionalDoubleToJson(cmd.z);
  j["a"] = optionalDoubleToJson(cmd.a);
  j["b"] = optionalDoubleToJson(cmd.b);
  j["c"] = optionalDoubleToJson(cmd.c);
  j["feed"] = optionalDoubleToJson(cmd.feed);
  j["modal"] = modalToJson(cmd.modal);
  j["effective"] = {
      {"motion_code", cmd.effective.motion_code},
      {"working_plane",
       cmd.effective.working_plane.has_value()
           ? nlohmann::json(
                 *cmd.effective.working_plane == WorkingPlane::XY
                     ? "xy"
                     : (*cmd.effective.working_plane == WorkingPlane::ZX
                            ? "zx"
                            : "yz"))
           : nlohmann::json(nullptr)},
      {"rapid_mode", cmd.effective.rapid_mode.has_value()
                         ? nlohmann::json(*cmd.effective.rapid_mode ==
                                                  RapidInterpolationMode::Linear
                                              ? "linear"
                                              : "nonlinear")
                         : nlohmann::json(nullptr)},
      {"tool_radius_comp",
       cmd.effective.tool_radius_comp.has_value()
           ? nlohmann::json(*cmd.effective.tool_radius_comp ==
                                    ToolRadiusCompMode::Off
                                ? "off"
                                : (*cmd.effective.tool_radius_comp ==
                                           ToolRadiusCompMode::Left
                                       ? "left"
                                       : "right"))
           : nlohmann::json(nullptr)}};
  return j;
}

nlohmann::json arcMoveToJson(const ArcMoveCommand &cmd) {
  nlohmann::json j;
  j["source"] = sourceToJson(cmd.source);
  j["opcode"] = cmd.opcode;
  j["clockwise"] = cmd.clockwise;
  j["plane_effective"] =
      cmd.plane_effective == WorkingPlane::XY
          ? "xy"
          : (cmd.plane_effective == WorkingPlane::ZX ? "zx" : "yz");
  j["x"] = optionalDoubleToJson(cmd.x);
  j["y"] = optionalDoubleToJson(cmd.y);
  j["z"] = optionalDoubleToJson(cmd.z);
  j["a"] = optionalDoubleToJson(cmd.a);
  j["b"] = optionalDoubleToJson(cmd.b);
  j["c"] = optionalDoubleToJson(cmd.c);
  j["arc"] = {{"i", optionalDoubleToJson(cmd.arc.i)},
              {"j", optionalDoubleToJson(cmd.arc.j)},
              {"k", optionalDoubleToJson(cmd.arc.k)},
              {"r", optionalDoubleToJson(cmd.arc.r)}};
  j["feed"] = optionalDoubleToJson(cmd.feed);
  j["modal"] = modalToJson(cmd.modal);
  j["effective"] = {
      {"motion_code", cmd.effective.motion_code},
      {"working_plane",
       cmd.effective.working_plane.has_value()
           ? nlohmann::json(
                 *cmd.effective.working_plane == WorkingPlane::XY
                     ? "xy"
                     : (*cmd.effective.working_plane == WorkingPlane::ZX
                            ? "zx"
                            : "yz"))
           : nlohmann::json(nullptr)},
      {"rapid_mode", cmd.effective.rapid_mode.has_value()
                         ? nlohmann::json(*cmd.effective.rapid_mode ==
                                                  RapidInterpolationMode::Linear
                                              ? "linear"
                                              : "nonlinear")
                         : nlohmann::json(nullptr)},
      {"tool_radius_comp",
       cmd.effective.tool_radius_comp.has_value()
           ? nlohmann::json(*cmd.effective.tool_radius_comp ==
                                    ToolRadiusCompMode::Off
                                ? "off"
                                : (*cmd.effective.tool_radius_comp ==
                                           ToolRadiusCompMode::Left
                                       ? "left"
                                       : "right"))
           : nlohmann::json(nullptr)}};
  return j;
}

nlohmann::json dwellToJson(const DwellCommand &cmd) {
  nlohmann::json j;
  j["source"] = sourceToJson(cmd.source);
  j["dwell_mode"] =
      cmd.dwell_mode == DwellMode::Revolutions ? "revolutions" : "seconds";
  j["dwell_value"] = cmd.dwell_value;
  j["modal"] = modalToJson(cmd.modal);
  j["effective"] = {
      {"motion_code", cmd.effective.motion_code},
      {"working_plane",
       cmd.effective.working_plane.has_value()
           ? nlohmann::json(
                 *cmd.effective.working_plane == WorkingPlane::XY
                     ? "xy"
                     : (*cmd.effective.working_plane == WorkingPlane::ZX
                            ? "zx"
                            : "yz"))
           : nlohmann::json(nullptr)},
      {"rapid_mode", cmd.effective.rapid_mode.has_value()
                         ? nlohmann::json(*cmd.effective.rapid_mode ==
                                                  RapidInterpolationMode::Linear
                                              ? "linear"
                                              : "nonlinear")
                         : nlohmann::json(nullptr)},
      {"tool_radius_comp",
       cmd.effective.tool_radius_comp.has_value()
           ? nlohmann::json(*cmd.effective.tool_radius_comp ==
                                    ToolRadiusCompMode::Off
                                ? "off"
                                : (*cmd.effective.tool_radius_comp ==
                                           ToolRadiusCompMode::Left
                                       ? "left"
                                       : "right"))
           : nlohmann::json(nullptr)}};
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
          out << " opcode=" << params.value("opcode", "");
          const auto &effective = params["effective"];
          if (!effective["working_plane"].is_null()) {
            out << " plane=" << effective["working_plane"].get<std::string>();
          }
          if (!effective["tool_radius_comp"].is_null()) {
            out << " comp=" << effective["tool_radius_comp"].get<std::string>();
          }
          if (!effective["rapid_mode"].is_null()) {
            out << " rapid=" << effective["rapid_mode"].get<std::string>();
          }
          out << "\n";
          out << "  target:";
          if (!params["x"].is_null()) {
            out << " x=" << params["x"].get<double>();
          }
          if (!params["y"].is_null()) {
            out << " y=" << params["y"].get<double>();
          }
          if (!params["z"].is_null()) {
            out << " z=" << params["z"].get<double>();
          }
          if (!params["feed"].is_null()) {
            out << " feed=" << params["feed"].get<double>();
          }
          out << "\n";
          out << "  runtime: submit_linear_move";
        } else if (event == "sink.arc_move") {
          out << "  emit arc_move:";
          out << " opcode=" << params.value("opcode", "");
          out << " plane=" << params.value("plane_effective", "");
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
ReadyRuntimeRecorder::readVariable(std::string_view name) {
  recorder_.add({{"event", "runtime.read_variable"},
                 {"params", {{"name", std::string(name)}}}});
  RuntimeResult<double> result;
  result.status = RuntimeCallStatus::Error;
  result.error_message = "fake runtime does not resolve user variables";
  return result;
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

RuntimeResult<void> ReadyRuntimeRecorder::writeVariable(std::string_view name,
                                                        double value) {
  recorder_.add({{"event", "runtime.write_variable"},
                 {"params", {{"name", std::string(name)}, {"value", value}}}});
  RuntimeResult<void> result;
  result.status = RuntimeCallStatus::Ready;
  return result;
}

RuntimeResult<WaitToken>
ReadyRuntimeRecorder::cancelWait(const WaitToken &token) {
  recorder_.add({{"event", "runtime.cancel_wait"},
                 {"params", {{"kind", token.kind}, {"id", token.id}}}});
  return readyWaitResult();
}

} // namespace gcode
