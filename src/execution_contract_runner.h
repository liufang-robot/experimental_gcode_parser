#pragma once

#include <string>

#include "execution_contract_fixture.h"

namespace gcode {

struct ExecutionContractRunResult {
  ExecutionContractTrace actual_trace;
  std::string actual_output_path;
};

ExecutionContractRunResult
runExecutionContractFixture(const std::string &program_path,
                            const ExecutionContractTrace &reference_trace,
                            const std::string &actual_output_path);

bool executionContractTracesEqual(const ExecutionContractTrace &lhs,
                                  const ExecutionContractTrace &rhs);

} // namespace gcode
