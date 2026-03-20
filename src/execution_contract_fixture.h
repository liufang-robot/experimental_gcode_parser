#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "gcode/execution_commands.h"
#include "gcode/lowering_types.h"

namespace gcode {

struct ExecutionContractInitialState {
  EffectiveModalSnapshot modal;
};

struct ExecutionContractOptions {
  std::optional<std::string> filename;
  std::vector<int> active_skip_levels;
  std::optional<ToolChangeMode> tool_change_mode;
  bool enable_iso_m98_calls = false;
};

enum class ExecutionContractDriverAction {
  Finish,
  ResumeBlocked,
  CancelBlocked,
};

struct ExecutionContractDriverStep {
  ExecutionContractDriverAction action = ExecutionContractDriverAction::Finish;
};

enum class ExecutionContractRuntimeWaitStatus {
  Ready,
  Pending,
};

struct ExecutionContractRuntimeWaitResult {
  ExecutionContractRuntimeWaitStatus status =
      ExecutionContractRuntimeWaitStatus::Ready;
  std::optional<WaitToken> token;
};

struct ExecutionContractSystemVariableRead {
  std::string name;
  double value = 0.0;
};

struct ExecutionContractRuntimeInputs {
  std::map<std::string, double> system_variables;
  std::vector<ExecutionContractSystemVariableRead> system_variable_reads;
  std::vector<ExecutionContractRuntimeWaitResult> linear_move_results;
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
  ExecutionContractOptions options;
  std::vector<ExecutionContractDriverStep> driver;
  std::optional<ExecutionContractRuntimeInputs> runtime;
  std::vector<ExecutionContractEvent> events;
};

ExecutionContractTrace loadExecutionContractTrace(const std::string &path);
std::string
serializeExecutionContractTrace(const ExecutionContractTrace &trace);
nlohmann::ordered_json
executionContractTraceToJson(const ExecutionContractTrace &trace);

} // namespace gcode
