#include <filesystem>

#include "gtest/gtest.h"

#include "execution_contract_fixture.h"
#include "execution_contract_runner.h"

namespace {

std::string sourcePath(const std::string &relative_path) {
  return std::string(GCODE_SOURCE_DIR) + "/" + relative_path;
}

TEST(ExecutionContractRunnerTest, Step1FixturesMatchReferenceTraces) {
  const std::vector<std::string> case_names = {
      "modal_update",          "linear_move_completed",
      "rejected_invalid_line", "fault_unresolved_target",
      "goto_skips_line",       "if_else_branch"};

  for (const auto &case_name : case_names) {
    SCOPED_TRACE(case_name);
    const auto reference = gcode::loadExecutionContractTrace(sourcePath(
        "testdata/execution_contract/core/" + case_name + ".events.yaml"));

    const auto actual = gcode::runExecutionContractFixture(
        sourcePath("testdata/execution_contract/core/" + case_name + ".ngc"),
        reference,
        sourcePath("output/execution_contract_review/core/" + case_name +
                   ".actual.yaml"));

    EXPECT_TRUE(
        gcode::executionContractTracesEqual(actual.actual_trace, reference));
    EXPECT_EQ(actual.actual_trace.name, reference.name);
    EXPECT_FALSE(actual.actual_output_path.empty());
    EXPECT_TRUE(std::filesystem::exists(actual.actual_output_path));
  }
}

} // namespace
