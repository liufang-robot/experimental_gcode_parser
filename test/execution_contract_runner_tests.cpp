#include <filesystem>
#include <fstream>
#include <string>

#include "gtest/gtest.h"

#include "execution_contract_fixture.h"
#include "execution_contract_runner.h"

namespace {

std::string sourcePath(const std::string &relative_path) {
  return std::string(GCODE_SOURCE_DIR) + "/" + relative_path;
}

std::filesystem::path writeTempFile(const std::string &name,
                                    const std::string &contents) {
  const auto path = std::filesystem::temp_directory_path() / name;
  std::ofstream out(path);
  out << contents;
  out.close();
  return path;
}

TEST(ExecutionContractRunnerTest, Step1FixturesMatchReferenceTraces) {
  const std::vector<std::string> case_names = {
      "modal_update",
      "linear_move_completed",
      "rejected_invalid_line",
      "fault_unresolved_target",
      "goto_skips_line",
      "if_else_branch",
      "if_system_variable_false_branch"};

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

TEST(ExecutionContractRunnerTest,
     RuntimeSystemVariableFixtureDrivesIfFalseBranch) {
  const auto program_path = writeTempFile("if_system_variable_false_branch.ngc",
                                          "IF $P_ACT_X == 1 GOTOF TARGET\n"
                                          "G1 X20\n"
                                          "GOTO END\n"
                                          "TARGET:\n"
                                          "G1 X10\n"
                                          "END:\n");
  const auto fixture_path =
      writeTempFile("if_system_variable_false_branch.events.yaml",
                    R"({
  "name": "if_system_variable_false_branch",
  "description": "System-variable-backed IF/GOTOF resolves through the fixture runtime.",
  "initial_state": {
    "modal": {
      "motion_code": "",
      "working_plane": "xy",
      "rapid_mode": "linear",
      "tool_radius_comp": "off",
      "active_tool_selection": null,
      "pending_tool_selection": null,
      "selected_tool_selection": null
    }
  },
  "runtime": {
    "system_variables": {
      "$P_ACT_X": 12.5
    }
  },
  "expected_events": [
    {
      "type": "linear_move",
      "source": {
        "filename": null,
        "line": 2,
        "line_number": null
      },
      "target": {
        "x": 20.0,
        "y": null,
        "z": null,
        "a": null,
        "b": null,
        "c": null
      },
      "feed": null,
      "effective": {
        "motion_code": "G1",
        "working_plane": "xy",
        "rapid_mode": "linear",
        "tool_radius_comp": "off",
        "active_tool_selection": null,
        "pending_tool_selection": null,
        "selected_tool_selection": null
      }
    },
    {
      "type": "completed"
    }
  ]
})");

  const auto reference =
      gcode::loadExecutionContractTrace(fixture_path.string());

  const auto actual = gcode::runExecutionContractFixture(
      program_path.string(), reference,
      sourcePath("output/execution_contract_review/core/"
                 "if_system_variable_false_branch.actual.yaml"));

  EXPECT_TRUE(
      gcode::executionContractTracesEqual(actual.actual_trace, reference))
      << gcode::serializeExecutionContractTrace(actual.actual_trace);
}

} // namespace
