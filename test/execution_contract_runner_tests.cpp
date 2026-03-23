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
      "linear_move_blocked",
      "linear_move_cancelled",
      "linear_move_block_resume",
      "linear_move_system_variable_x",
      "linear_move_system_variable_selector_x",
      "motion_then_condition_system_variable_reads",
      "repeated_system_variable_reads",
      "rapid_move_system_variable_z",
      "resumed_system_variable_read",
      "dwell_seconds_completed",
      "tool_change_deferred_m6",
      "rejected_invalid_line",
      "fault_unresolved_target",
      "goto_skips_line",
      "if_else_branch",
      "if_system_variable_false_branch",
      "if_system_variable_selector_false_branch"};

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
  "options": {
    "filename": null,
    "active_skip_levels": [],
    "tool_change_mode": "deferred_m6",
    "enable_iso_m98_calls": false
  },
  "runtime": {
    "system_variables": {
      "$P_ACT_X": 12.5
    }
  },
  "expected_events": [
    {
      "type": "system_variable_read",
      "source": {
        "filename": null,
        "line": 1,
        "line_number": null
      },
      "name": "$P_ACT_X",
      "value": 12.5
    },
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

TEST(ExecutionContractRunnerTest, FixtureOptionsCanDriveDirectToolMode) {
  const auto program_path = writeTempFile("tool_change_direct_t.ngc", "T12\n");
  const auto fixture_path = writeTempFile("tool_change_direct_t.events.yaml",
                                          R"({
  "name": "tool_change_direct_t",
  "description": "Fixture options expose the direct tool-change policy.",
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
  "options": {
    "filename": null,
    "active_skip_levels": [],
    "tool_change_mode": "direct_t",
    "enable_iso_m98_calls": false
  },
  "expected_events": [
    {
      "type": "tool_change",
      "source": {
        "filename": null,
        "line": 1,
        "line_number": null
      },
      "target_tool_selection": {
        "selector_index": null,
        "selector_value": "12"
      },
      "effective": {
        "motion_code": "G1",
        "working_plane": "xy",
        "rapid_mode": "linear",
        "tool_radius_comp": "off",
        "active_tool_selection": null,
        "pending_tool_selection": null,
        "selected_tool_selection": {
          "selector_index": null,
          "selector_value": "12"
        }
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
                 "tool_change_direct_t.actual.yaml"));

  EXPECT_TRUE(
      gcode::executionContractTracesEqual(actual.actual_trace, reference))
      << gcode::serializeExecutionContractTrace(actual.actual_trace);
}

TEST(ExecutionContractRunnerTest,
     RuntimeReadScriptCanDriveRepeatedSystemVariableReads) {
  const auto program_path = writeTempFile("repeated_system_variable_reads.ngc",
                                          "G1 X=$P_ACT_X\nG1 Y=$P_ACT_X\n");
  const auto fixture_path =
      writeTempFile("repeated_system_variable_reads.events.yaml",
                    R"({
  "name": "repeated_system_variable_reads",
  "description": "Ordered runtime read scripts drive repeated variable reads in trace order.",
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
  "options": {
    "filename": null,
    "active_skip_levels": [],
    "tool_change_mode": "deferred_m6",
    "enable_iso_m98_calls": false
  },
  "runtime": {
    "system_variables": {},
    "system_variable_reads": [
      {
        "name": "$P_ACT_X",
        "value": 10.0
      },
      {
        "name": "$P_ACT_X",
        "value": 20.0
      }
    ]
  },
  "expected_events": [
    {
      "type": "system_variable_read",
      "source": {
        "filename": null,
        "line": 1,
        "line_number": null
      },
      "name": "$P_ACT_X",
      "value": 10.0
    },
    {
      "type": "linear_move",
      "source": {
        "filename": null,
        "line": 1,
        "line_number": null
      },
      "target": {
        "x": 10.0,
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
      "type": "system_variable_read",
      "source": {
        "filename": null,
        "line": 2,
        "line_number": null
      },
      "name": "$P_ACT_X",
      "value": 20.0
    },
    {
      "type": "linear_move",
      "source": {
        "filename": null,
        "line": 2,
        "line_number": null
      },
      "target": {
        "x": null,
        "y": 20.0,
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
                 "repeated_system_variable_reads.actual.yaml"));

  EXPECT_TRUE(
      gcode::executionContractTracesEqual(actual.actual_trace, reference))
      << gcode::serializeExecutionContractTrace(actual.actual_trace);
}

TEST(ExecutionContractRunnerTest, DriverCanBlockAndResumeLinearMove) {
  const auto program_path =
      writeTempFile("linear_move_block_resume.ngc", "G1 X10\nG1 Y10\n");
  const auto fixture_path =
      writeTempFile("linear_move_block_resume.events.yaml",
                    R"({
  "name": "linear_move_block_resume",
  "description": "First move blocks, then resume continues with the next move.",
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
  "options": {
    "filename": null,
    "active_skip_levels": [],
    "tool_change_mode": "deferred_m6",
    "enable_iso_m98_calls": false
  },
  "driver": [
    {
      "action": "finish"
    },
    {
      "action": "resume_blocked"
    }
  ],
  "runtime": {
    "system_variables": {},
    "linear_move_results": [
      {
        "status": "pending",
        "token": {
          "kind": "motion",
          "id": "move-001"
        }
      },
      {
        "status": "ready"
      }
    ]
  },
  "expected_events": [
    {
      "type": "linear_move",
      "source": {
        "filename": null,
        "line": 1,
        "line_number": null
      },
      "target": {
        "x": 10.0,
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
      "type": "blocked",
      "line": 1,
      "token": {
        "kind": "motion",
        "id": "move-001"
      },
      "reason": "instruction execution in progress"
    },
    {
      "type": "linear_move",
      "source": {
        "filename": null,
        "line": 2,
        "line_number": null
      },
      "target": {
        "x": null,
        "y": 10.0,
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
                 "linear_move_block_resume.actual.yaml"));

  EXPECT_TRUE(
      gcode::executionContractTracesEqual(actual.actual_trace, reference))
      << gcode::serializeExecutionContractTrace(actual.actual_trace);
}

} // namespace
