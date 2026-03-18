#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "gcode/execution_commands.h"

namespace gcode {

struct ExecutionContractInitialState {
  EffectiveModalSnapshot modal;
};

struct ExecutionContractRuntimeInputs {
  std::map<std::string, double> system_variables;
};

struct ExecutionContractEvent {
  std::string type;
  std::optional<SourceRef> source;
  nlohmann::ordered_json data;
};

struct ExecutionContractTrace {
  std::string name;
  std::optional<std::string> description;
  ExecutionContractInitialState initial_state;
  std::optional<ExecutionContractRuntimeInputs> runtime;
  std::vector<ExecutionContractEvent> events;
};

ExecutionContractTrace loadExecutionContractTrace(const std::string &path);
std::string
serializeExecutionContractTrace(const ExecutionContractTrace &trace);
nlohmann::ordered_json
executionContractTraceToJson(const ExecutionContractTrace &trace);

} // namespace gcode
