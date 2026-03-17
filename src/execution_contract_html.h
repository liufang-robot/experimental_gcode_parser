#pragma once

#include <string>
#include <vector>

#include "execution_contract_fixture.h"

namespace gcode {

struct ExecutionContractCaseReport {
  std::string suite_name;
  std::string case_name;
  std::string description;
  std::string program_path;
  std::string reference_path;
  std::string actual_path;
  bool matches_reference = false;
  ExecutionContractTrace reference_trace;
  ExecutionContractTrace actual_trace;
};

void writeExecutionContractHtmlSite(
    const std::vector<ExecutionContractCaseReport> &cases,
    const std::string &output_root);

} // namespace gcode
