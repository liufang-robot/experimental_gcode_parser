#include "execution_contract_fixture.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace gcode {
namespace {

std::optional<SourceRef> sourceFromJson(const nlohmann::ordered_json &j) {
  if (j.is_null()) {
    return std::nullopt;
  }

  SourceRef source;
  if (j.contains("filename") && !j["filename"].is_null()) {
    source.filename = j["filename"].get<std::string>();
  }
  source.line = j.value("line", 0);
  if (j.contains("line_number") && !j["line_number"].is_null()) {
    source.line_number = j["line_number"].get<int>();
  }
  return source;
}

nlohmann::ordered_json sourceToJson(const std::optional<SourceRef> &source) {
  if (!source.has_value()) {
    return nullptr;
  }

  nlohmann::ordered_json j;
  j["filename"] = source->filename.has_value()
                      ? nlohmann::ordered_json(*source->filename)
                      : nlohmann::ordered_json(nullptr);
  j["line"] = source->line;
  j["line_number"] = source->line_number.has_value()
                         ? nlohmann::ordered_json(*source->line_number)
                         : nlohmann::ordered_json(nullptr);
  return j;
}

WorkingPlane workingPlaneFromString(const std::string &value) {
  if (value == "xy") {
    return WorkingPlane::XY;
  }
  if (value == "zx") {
    return WorkingPlane::ZX;
  }
  if (value == "yz") {
    return WorkingPlane::YZ;
  }
  throw std::runtime_error("unsupported working_plane value: " + value);
}

std::string workingPlaneToString(WorkingPlane plane) {
  switch (plane) {
  case WorkingPlane::XY:
    return "xy";
  case WorkingPlane::ZX:
    return "zx";
  case WorkingPlane::YZ:
    return "yz";
  }
  return "xy";
}

RapidInterpolationMode rapidModeFromString(const std::string &value) {
  if (value == "linear") {
    return RapidInterpolationMode::Linear;
  }
  if (value == "nonlinear") {
    return RapidInterpolationMode::NonLinear;
  }
  throw std::runtime_error("unsupported rapid_mode value: " + value);
}

std::string rapidModeToString(RapidInterpolationMode mode) {
  return mode == RapidInterpolationMode::Linear ? "linear" : "nonlinear";
}

ToolRadiusCompMode toolRadiusCompFromString(const std::string &value) {
  if (value == "off") {
    return ToolRadiusCompMode::Off;
  }
  if (value == "left") {
    return ToolRadiusCompMode::Left;
  }
  if (value == "right") {
    return ToolRadiusCompMode::Right;
  }
  throw std::runtime_error("unsupported tool_radius_comp value: " + value);
}

std::string toolRadiusCompToString(ToolRadiusCompMode mode) {
  switch (mode) {
  case ToolRadiusCompMode::Off:
    return "off";
  case ToolRadiusCompMode::Left:
    return "left";
  case ToolRadiusCompMode::Right:
    return "right";
  }
  return "off";
}

std::optional<ToolSelectionState>
toolSelectionFromJson(const nlohmann::ordered_json &j) {
  if (j.is_null()) {
    return std::nullopt;
  }

  ToolSelectionState selection;
  if (j.contains("selector_index") && !j["selector_index"].is_null()) {
    selection.selector_index = j["selector_index"].get<int>();
  }
  selection.selector_value = j.value("selector_value", "");
  return selection;
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

EffectiveModalSnapshot
effectiveModalSnapshotFromJson(const nlohmann::ordered_json &j) {
  EffectiveModalSnapshot snapshot;
  snapshot.motion_code = j.value("motion_code", "");
  snapshot.working_plane =
      workingPlaneFromString(j.value("working_plane", std::string("xy")));
  snapshot.rapid_mode =
      rapidModeFromString(j.value("rapid_mode", std::string("linear")));
  snapshot.tool_radius_comp =
      toolRadiusCompFromString(j.value("tool_radius_comp", std::string("off")));
  snapshot.active_tool_selection = toolSelectionFromJson(
      j.value("active_tool_selection", nlohmann::ordered_json(nullptr)));
  snapshot.pending_tool_selection = toolSelectionFromJson(
      j.value("pending_tool_selection", nlohmann::ordered_json(nullptr)));
  snapshot.selected_tool_selection = toolSelectionFromJson(
      j.value("selected_tool_selection", nlohmann::ordered_json(nullptr)));
  return snapshot;
}

nlohmann::ordered_json
effectiveModalSnapshotToJson(const EffectiveModalSnapshot &snapshot) {
  nlohmann::ordered_json j;
  j["motion_code"] = snapshot.motion_code;
  j["working_plane"] = workingPlaneToString(snapshot.working_plane);
  j["rapid_mode"] = rapidModeToString(snapshot.rapid_mode);
  j["tool_radius_comp"] = toolRadiusCompToString(snapshot.tool_radius_comp);
  j["active_tool_selection"] =
      toolSelectionToJson(snapshot.active_tool_selection);
  j["pending_tool_selection"] =
      toolSelectionToJson(snapshot.pending_tool_selection);
  j["selected_tool_selection"] =
      toolSelectionToJson(snapshot.selected_tool_selection);
  return j;
}

ToolChangeMode toolChangeModeFromString(const std::string &value) {
  if (value == "direct_t") {
    return ToolChangeMode::DirectT;
  }
  if (value == "deferred_m6") {
    return ToolChangeMode::DeferredM6;
  }
  throw std::runtime_error("unsupported tool_change_mode value: " + value);
}

std::string toolChangeModeToString(ToolChangeMode mode) {
  switch (mode) {
  case ToolChangeMode::DirectT:
    return "direct_t";
  case ToolChangeMode::DeferredM6:
    return "deferred_m6";
  }
  return "deferred_m6";
}

ExecutionContractDriverAction driverActionFromString(const std::string &value) {
  if (value == "finish") {
    return ExecutionContractDriverAction::Finish;
  }
  if (value == "resume_blocked") {
    return ExecutionContractDriverAction::ResumeBlocked;
  }
  throw std::runtime_error("unsupported driver action: " + value);
}

std::string driverActionToString(ExecutionContractDriverAction action) {
  switch (action) {
  case ExecutionContractDriverAction::Finish:
    return "finish";
  case ExecutionContractDriverAction::ResumeBlocked:
    return "resume_blocked";
  }
  return "finish";
}

ExecutionContractDriverStep
driverStepFromJson(const nlohmann::ordered_json &j) {
  ExecutionContractDriverStep step;
  step.action = driverActionFromString(j.at("action").get<std::string>());
  return step;
}

nlohmann::ordered_json
driverStepToJson(const ExecutionContractDriverStep &step) {
  nlohmann::ordered_json j;
  j["action"] = driverActionToString(step.action);
  return j;
}

ExecutionContractRuntimeWaitStatus
runtimeWaitStatusFromString(const std::string &value) {
  if (value == "ready") {
    return ExecutionContractRuntimeWaitStatus::Ready;
  }
  if (value == "pending") {
    return ExecutionContractRuntimeWaitStatus::Pending;
  }
  throw std::runtime_error("unsupported runtime wait status: " + value);
}

std::string
runtimeWaitStatusToString(ExecutionContractRuntimeWaitStatus status) {
  switch (status) {
  case ExecutionContractRuntimeWaitStatus::Ready:
    return "ready";
  case ExecutionContractRuntimeWaitStatus::Pending:
    return "pending";
  }
  return "ready";
}

ExecutionContractRuntimeWaitResult
runtimeWaitResultFromJson(const nlohmann::ordered_json &j) {
  ExecutionContractRuntimeWaitResult result;
  result.status =
      runtimeWaitStatusFromString(j.at("status").get<std::string>());
  if (j.contains("token") && !j["token"].is_null()) {
    WaitToken token;
    token.kind = j["token"].value("kind", "");
    token.id = j["token"].value("id", "");
    result.token = std::move(token);
  }
  if (result.status == ExecutionContractRuntimeWaitStatus::Pending &&
      !result.token.has_value()) {
    throw std::runtime_error("pending runtime wait result requires token");
  }
  return result;
}

nlohmann::ordered_json
runtimeWaitResultToJson(const ExecutionContractRuntimeWaitResult &result) {
  nlohmann::ordered_json j;
  j["status"] = runtimeWaitStatusToString(result.status);
  if (result.token.has_value()) {
    j["token"] = {{"kind", result.token->kind}, {"id", result.token->id}};
  }
  return j;
}

ExecutionContractOptions optionsFromJson(const nlohmann::ordered_json &j) {
  ExecutionContractOptions options;
  if (j.contains("filename") && !j["filename"].is_null()) {
    options.filename = j["filename"].get<std::string>();
  }
  if (j.contains("active_skip_levels") && j["active_skip_levels"].is_array()) {
    options.active_skip_levels =
        j["active_skip_levels"].get<std::vector<int>>();
  }
  if (j.contains("tool_change_mode") && !j["tool_change_mode"].is_null()) {
    options.tool_change_mode =
        toolChangeModeFromString(j["tool_change_mode"].get<std::string>());
  }
  options.enable_iso_m98_calls = j.value("enable_iso_m98_calls", false);
  return options;
}

nlohmann::ordered_json optionsToJson(const ExecutionContractOptions &options) {
  nlohmann::ordered_json j;
  j["filename"] = options.filename.has_value()
                      ? nlohmann::ordered_json(*options.filename)
                      : nlohmann::ordered_json(nullptr);
  j["active_skip_levels"] = options.active_skip_levels;
  j["tool_change_mode"] = options.tool_change_mode.has_value()
                              ? nlohmann::ordered_json(toolChangeModeToString(
                                    *options.tool_change_mode))
                              : nlohmann::ordered_json(nullptr);
  j["enable_iso_m98_calls"] = options.enable_iso_m98_calls;
  return j;
}

ExecutionContractRuntimeInputs
runtimeInputsFromJson(const nlohmann::ordered_json &j) {
  ExecutionContractRuntimeInputs runtime;
  if (!j.contains("system_variables") || j["system_variables"].is_null()) {
    return runtime;
  }

  for (auto it = j["system_variables"].begin();
       it != j["system_variables"].end(); ++it) {
    runtime.system_variables[it.key()] = it.value().get<double>();
  }
  if (j.contains("linear_move_results") &&
      j["linear_move_results"].is_array()) {
    for (const auto &result_json : j["linear_move_results"]) {
      runtime.linear_move_results.push_back(
          runtimeWaitResultFromJson(result_json));
    }
  }
  return runtime;
}

nlohmann::ordered_json
runtimeInputsToJson(const ExecutionContractRuntimeInputs &runtime) {
  nlohmann::ordered_json j;
  j["system_variables"] = nlohmann::ordered_json::object();
  for (const auto &entry : runtime.system_variables) {
    j["system_variables"][entry.first] = entry.second;
  }
  j["linear_move_results"] = nlohmann::ordered_json::array();
  for (const auto &result : runtime.linear_move_results) {
    j["linear_move_results"].push_back(runtimeWaitResultToJson(result));
  }
  return j;
}

ExecutionContractEvent eventFromJson(const nlohmann::ordered_json &j) {
  ExecutionContractEvent event;
  event.type = j.at("type").get<std::string>();
  if (j.contains("source")) {
    event.source = sourceFromJson(j["source"]);
  }
  event.data = j;
  event.data.erase("type");
  event.data.erase("source");
  return event;
}

nlohmann::ordered_json eventToJson(const ExecutionContractEvent &event) {
  nlohmann::ordered_json j;
  j["type"] = event.type;
  if (event.source.has_value()) {
    j["source"] = sourceToJson(event.source);
  }
  for (auto it = event.data.begin(); it != event.data.end(); ++it) {
    j[it.key()] = it.value();
  }
  return j;
}

std::string readFile(const std::string &path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open fixture file: " + path);
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

} // namespace

ExecutionContractTrace loadExecutionContractTrace(const std::string &path) {
  const auto raw = readFile(path);
  const auto root = nlohmann::ordered_json::parse(raw);

  ExecutionContractTrace trace;
  trace.name = root.at("name").get<std::string>();
  if (root.contains("description") && !root["description"].is_null()) {
    trace.description = root["description"].get<std::string>();
  }
  trace.initial_state.modal =
      effectiveModalSnapshotFromJson(root.at("initial_state").at("modal"));
  if (root.contains("options") && !root["options"].is_null()) {
    trace.options = optionsFromJson(root["options"]);
  }
  if (root.contains("driver") && root["driver"].is_array()) {
    for (const auto &driver_json : root["driver"]) {
      trace.driver.push_back(driverStepFromJson(driver_json));
    }
  }
  if (root.contains("runtime") && !root["runtime"].is_null()) {
    trace.runtime = runtimeInputsFromJson(root["runtime"]);
  }

  for (const auto &event_json : root.at("expected_events")) {
    trace.events.push_back(eventFromJson(event_json));
  }

  return trace;
}

nlohmann::ordered_json
executionContractTraceToJson(const ExecutionContractTrace &trace) {
  nlohmann::ordered_json root;
  root["name"] = trace.name;
  if (trace.description.has_value()) {
    root["description"] = *trace.description;
  }
  root["initial_state"] = {
      {"modal", effectiveModalSnapshotToJson(trace.initial_state.modal)}};
  root["options"] = optionsToJson(trace.options);
  if (!trace.driver.empty()) {
    root["driver"] = nlohmann::ordered_json::array();
    for (const auto &step : trace.driver) {
      root["driver"].push_back(driverStepToJson(step));
    }
  }
  if (trace.runtime.has_value()) {
    root["runtime"] = runtimeInputsToJson(*trace.runtime);
  }
  root["expected_events"] = nlohmann::ordered_json::array();
  for (const auto &event : trace.events) {
    root["expected_events"].push_back(eventToJson(event));
  }
  return root;
}

std::string
serializeExecutionContractTrace(const ExecutionContractTrace &trace) {
  return executionContractTraceToJson(trace).dump(2) + "\n";
}

} // namespace gcode
