#include "execution_contract_runner.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

#include "gcode/execution_session.h"

namespace gcode {
namespace {

std::string readTextFile(const std::string &path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open program file: " + path);
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

nlohmann::ordered_json sourceToJson(const SourceRef &source) {
  nlohmann::ordered_json j;
  j["filename"] = source.filename.has_value()
                      ? nlohmann::ordered_json(*source.filename)
                      : nlohmann::ordered_json(nullptr);
  j["line"] = source.line;
  j["line_number"] = source.line_number.has_value()
                         ? nlohmann::ordered_json(*source.line_number)
                         : nlohmann::ordered_json(nullptr);
  return j;
}

nlohmann::ordered_json
optionalDoubleToJson(const std::optional<double> &value) {
  return value.has_value() ? nlohmann::ordered_json(*value)
                           : nlohmann::ordered_json(nullptr);
}

nlohmann::ordered_json poseTargetToJson(const PoseTarget &target) {
  nlohmann::ordered_json j;
  j["x"] = optionalDoubleToJson(target.x);
  j["y"] = optionalDoubleToJson(target.y);
  j["z"] = optionalDoubleToJson(target.z);
  j["a"] = optionalDoubleToJson(target.a);
  j["b"] = optionalDoubleToJson(target.b);
  j["c"] = optionalDoubleToJson(target.c);
  return j;
}

nlohmann::ordered_json
toolSelectionToJson(const std::optional<ToolSelectionState> &selection) {
  if (!selection.has_value()) {
    return nullptr;
  }

  nlohmann::ordered_json j;
  j["selector_index"] = selection->selector_index.has_value()
                            ? nlohmann::ordered_json(*selection->selector_index)
                            : nlohmann::ordered_json(nullptr);
  j["selector_value"] = selection->selector_value;
  return j;
}

nlohmann::ordered_json
effectiveToJson(const EffectiveModalSnapshot &effective) {
  nlohmann::ordered_json j;
  j["motion_code"] = effective.motion_code;
  j["working_plane"] =
      effective.working_plane == WorkingPlane::XY
          ? "xy"
          : (effective.working_plane == WorkingPlane::ZX ? "zx" : "yz");
  j["rapid_mode"] = effective.rapid_mode == RapidInterpolationMode::Linear
                        ? "linear"
                        : "nonlinear";
  j["tool_radius_comp"] =
      effective.tool_radius_comp == ToolRadiusCompMode::Off
          ? "off"
          : (effective.tool_radius_comp == ToolRadiusCompMode::Left ? "left"
                                                                    : "right");
  j["active_tool_selection"] =
      toolSelectionToJson(effective.active_tool_selection);
  j["pending_tool_selection"] =
      toolSelectionToJson(effective.pending_tool_selection);
  j["selected_tool_selection"] =
      toolSelectionToJson(effective.selected_tool_selection);
  return j;
}

nlohmann::ordered_json diagnosticToJson(const Diagnostic &diag) {
  nlohmann::ordered_json j;
  j["severity"] =
      diag.severity == Diagnostic::Severity::Error ? "error" : "warning";
  j["message"] = diag.message;
  j["location"] = {{"line", diag.location.line},
                   {"column", diag.location.column}};
  return j;
}

class ContractRecordingSink final : public IExecutionSink {
public:
  explicit ContractRecordingSink(std::vector<ExecutionContractEvent> *events)
      : events_(events) {}

  void onDiagnostic(const Diagnostic &diag) override {
    events_->push_back({"diagnostic", std::nullopt, diagnosticToJson(diag)});
  }

  void onRejectedLine(const RejectedLineEvent &event) override {
    nlohmann::ordered_json data;
    data["reasons"] = nlohmann::ordered_json::array();
    for (const auto &reason : event.reasons) {
      data["reasons"].push_back(diagnosticToJson(reason));
    }
    events_->push_back({"rejected", event.source, std::move(data)});
  }

  void onModalUpdate(const ModalUpdateEvent &event) override {
    nlohmann::ordered_json data;
    nlohmann::ordered_json changes;
    if (event.changes.working_plane.has_value()) {
      changes["working_plane"] =
          *event.changes.working_plane == WorkingPlane::XY
              ? "xy"
              : (*event.changes.working_plane == WorkingPlane::ZX ? "zx"
                                                                  : "yz");
    }
    if (event.changes.rapid_mode.has_value()) {
      changes["rapid_mode"] =
          *event.changes.rapid_mode == RapidInterpolationMode::Linear
              ? "linear"
              : "nonlinear";
    }
    if (event.changes.tool_radius_comp.has_value()) {
      changes["tool_radius_comp"] =
          *event.changes.tool_radius_comp == ToolRadiusCompMode::Off
              ? "off"
              : (*event.changes.tool_radius_comp == ToolRadiusCompMode::Left
                     ? "left"
                     : "right");
    }
    data["changes"] = std::move(changes);
    events_->push_back({"modal_update", event.source, std::move(data)});
  }

  void onLinearMove(const LinearMoveCommand &cmd) override {
    nlohmann::ordered_json data;
    data["target"] = poseTargetToJson(cmd.target);
    data["feed"] = optionalDoubleToJson(cmd.feed);
    data["effective"] = effectiveToJson(cmd.effective);
    events_->push_back({"linear_move", cmd.source, std::move(data)});
  }

  void onArcMove(const ArcMoveCommand &cmd) override {
    nlohmann::ordered_json data;
    data["clockwise"] = cmd.clockwise;
    data["target"] = poseTargetToJson(cmd.target);
    data["arc"] = {{"i", optionalDoubleToJson(cmd.arc.i)},
                   {"j", optionalDoubleToJson(cmd.arc.j)},
                   {"k", optionalDoubleToJson(cmd.arc.k)},
                   {"r", optionalDoubleToJson(cmd.arc.r)}};
    data["feed"] = optionalDoubleToJson(cmd.feed);
    data["effective"] = effectiveToJson(cmd.effective);
    events_->push_back({"arc_move", cmd.source, std::move(data)});
  }

  void onDwell(const DwellCommand &cmd) override {
    nlohmann::ordered_json data;
    data["dwell_mode"] =
        cmd.dwell_mode == DwellMode::Revolutions ? "revolutions" : "seconds";
    data["dwell_value"] = cmd.dwell_value;
    data["effective"] = effectiveToJson(cmd.effective);
    events_->push_back({"dwell", cmd.source, std::move(data)});
  }

  void onToolChange(const ToolChangeCommand &cmd) override {
    nlohmann::ordered_json data;
    data["target_tool_selection"] =
        toolSelectionToJson(cmd.target_tool_selection);
    data["effective"] = effectiveToJson(cmd.effective);
    events_->push_back({"tool_change", cmd.source, std::move(data)});
  }

private:
  std::vector<ExecutionContractEvent> *events_;
};

class ReadyRuntime final : public IRuntime {
public:
  RuntimeResult<WaitToken>
  submitLinearMove(const LinearMoveCommand &) override {
    RuntimeResult<WaitToken> result;
    result.status = RuntimeCallStatus::Ready;
    return result;
  }

  RuntimeResult<WaitToken> submitArcMove(const ArcMoveCommand &) override {
    RuntimeResult<WaitToken> result;
    result.status = RuntimeCallStatus::Ready;
    return result;
  }

  RuntimeResult<WaitToken> submitDwell(const DwellCommand &) override {
    RuntimeResult<WaitToken> result;
    result.status = RuntimeCallStatus::Ready;
    return result;
  }

  RuntimeResult<WaitToken>
  submitToolChange(const ToolChangeCommand &) override {
    RuntimeResult<WaitToken> result;
    result.status = RuntimeCallStatus::Ready;
    return result;
  }

  RuntimeResult<double> readSystemVariable(std::string_view) override {
    RuntimeResult<double> result;
    result.status = RuntimeCallStatus::Error;
    result.error_message = "system variable reads are not configured";
    return result;
  }

  RuntimeResult<WaitToken> cancelWait(const WaitToken &) override {
    RuntimeResult<WaitToken> result;
    result.status = RuntimeCallStatus::Ready;
    return result;
  }
};

class NeverCancelled final : public ICancellation {
public:
  bool isCancelled() const override { return false; }
};

void appendTerminalStepEvent(std::vector<ExecutionContractEvent> *events,
                             const StepResult &step) {
  switch (step.status) {
  case StepStatus::Completed:
    events->push_back({"completed", std::nullopt, {}});
    return;
  case StepStatus::Cancelled:
    events->push_back({"cancelled", std::nullopt, {}});
    return;
  case StepStatus::Faulted: {
    if (step.fault.has_value()) {
      events->push_back(
          {"faulted", std::nullopt, diagnosticToJson(*step.fault)});
    } else {
      events->push_back({"faulted", std::nullopt, {}});
    }
    return;
  }
  case StepStatus::Blocked: {
    nlohmann::ordered_json data;
    if (step.blocked.has_value()) {
      data["line"] = step.blocked->line;
      data["token"] = {{"kind", step.blocked->token.kind},
                       {"id", step.blocked->token.id}};
      data["reason"] = step.blocked->reason;
    }
    events->push_back({"blocked", std::nullopt, std::move(data)});
    return;
  }
  case StepStatus::Rejected:
  case StepStatus::Progress:
    return;
  }
}

} // namespace

ExecutionContractRunResult
runExecutionContractFixture(const std::string &program_path,
                            const ExecutionContractTrace &reference_trace,
                            const std::string &actual_output_path) {
  std::vector<ExecutionContractEvent> actual_events;
  ContractRecordingSink sink(&actual_events);
  ReadyRuntime runtime;
  NeverCancelled cancellation;
  LowerOptions options;

  ExecutionSession session(sink, runtime, cancellation, options);
  if (!session.pushChunk(readTextFile(program_path))) {
    throw std::runtime_error("failed to enqueue execution contract input");
  }

  StepResult step = session.finish();
  while (step.status == StepStatus::Progress) {
    step = session.pump();
  }
  appendTerminalStepEvent(&actual_events, step);

  ExecutionContractTrace actual_trace;
  actual_trace.name = reference_trace.name;
  actual_trace.description = reference_trace.description;
  actual_trace.initial_state = reference_trace.initial_state;
  actual_trace.events = std::move(actual_events);

  std::filesystem::path output_path(actual_output_path);
  std::filesystem::create_directories(output_path.parent_path());
  std::ofstream out(output_path);
  out << serializeExecutionContractTrace(actual_trace);
  out.close();

  return {std::move(actual_trace), output_path.string()};
}

bool executionContractTracesEqual(const ExecutionContractTrace &lhs,
                                  const ExecutionContractTrace &rhs) {
  return executionContractTraceToJson(lhs) == executionContractTraceToJson(rhs);
}

} // namespace gcode
