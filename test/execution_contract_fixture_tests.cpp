#include <filesystem>
#include <fstream>
#include <string>

#include "gtest/gtest.h"

#include "execution_contract_fixture.h"

namespace {

std::filesystem::path writeFixtureFile(const std::string &name,
                                       const std::string &contents) {
  const auto path = std::filesystem::temp_directory_path() / name;
  std::ofstream out(path);
  out << contents;
  out.close();
  return path;
}

TEST(ExecutionContractFixtureTest, LoadsInitialStateAndModalUpdateChanges) {
  const auto path =
      writeFixtureFile("execution_contract_modal_update.events.yaml",
                       R"({
  "name": "modal_update",
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
  "expected_events": [
    {
      "type": "modal_update",
      "source": {
        "filename": null,
        "line": 1,
        "line_number": null
      },
      "changes": {
        "working_plane": "zx"
      }
    },
    {
      "type": "completed"
    }
  ]
})");

  const auto trace = gcode::loadExecutionContractTrace(path.string());

  EXPECT_EQ(trace.name, "modal_update");
  EXPECT_EQ(trace.initial_state.modal.working_plane, gcode::WorkingPlane::XY);
  EXPECT_FALSE(trace.options.filename.has_value());
  EXPECT_TRUE(trace.options.active_skip_levels.empty());
  ASSERT_TRUE(trace.options.tool_change_mode.has_value());
  EXPECT_EQ(*trace.options.tool_change_mode, gcode::ToolChangeMode::DeferredM6);
  EXPECT_FALSE(trace.options.enable_iso_m98_calls);
  ASSERT_EQ(trace.events.size(), 2u);
  EXPECT_EQ(trace.events[0].type, "modal_update");
  ASSERT_TRUE(trace.events[0].source.has_value());
  EXPECT_EQ(trace.events[0].source->line, 1);
  ASSERT_TRUE(trace.events[0].data.contains("changes"));
  EXPECT_EQ(trace.events[0].data["changes"]["working_plane"], "zx");
  EXPECT_EQ(trace.events[1].type, "completed");
}

TEST(ExecutionContractFixtureTest, LoadsRuntimeSystemVariables) {
  const auto path =
      writeFixtureFile("execution_contract_runtime_inputs.events.yaml",
                       R"({
  "name": "runtime_inputs",
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
    "filename": "runtime_inputs.ngc",
    "active_skip_levels": [1, 3],
    "tool_change_mode": "direct_t",
    "enable_iso_m98_calls": true
  },
  "runtime": {
    "system_variables": {
      "$P_ACT_X": 12.5,
      "$AA_IM[X]": 8.0
    }
  },
  "expected_events": [
    {
      "type": "completed"
    }
  ]
})");

  const auto trace = gcode::loadExecutionContractTrace(path.string());

  ASSERT_TRUE(trace.runtime.has_value());
  ASSERT_TRUE(trace.options.filename.has_value());
  EXPECT_EQ(*trace.options.filename, "runtime_inputs.ngc");
  ASSERT_EQ(trace.options.active_skip_levels.size(), 2u);
  EXPECT_EQ(trace.options.active_skip_levels[0], 1);
  EXPECT_EQ(trace.options.active_skip_levels[1], 3);
  ASSERT_TRUE(trace.options.tool_change_mode.has_value());
  EXPECT_EQ(*trace.options.tool_change_mode, gcode::ToolChangeMode::DirectT);
  EXPECT_TRUE(trace.options.enable_iso_m98_calls);
  ASSERT_EQ(trace.runtime->system_variables.size(), 2u);
  EXPECT_EQ(trace.runtime->system_variables.at("$P_ACT_X"), 12.5);
  EXPECT_EQ(trace.runtime->system_variables.at("$AA_IM[X]"), 8.0);

  const auto roundtrip = gcode::executionContractTraceToJson(trace);
  ASSERT_TRUE(roundtrip.contains("options"));
  EXPECT_EQ(roundtrip["options"]["filename"], "runtime_inputs.ngc");
  ASSERT_EQ(roundtrip["options"]["active_skip_levels"].size(), 2u);
  EXPECT_EQ(roundtrip["options"]["tool_change_mode"], "direct_t");
  EXPECT_EQ(roundtrip["options"]["enable_iso_m98_calls"], true);
  ASSERT_TRUE(roundtrip.contains("runtime"));
  ASSERT_TRUE(roundtrip["runtime"].contains("system_variables"));
  EXPECT_EQ(roundtrip["runtime"]["system_variables"]["$P_ACT_X"], 12.5);
  EXPECT_EQ(roundtrip["runtime"]["system_variables"]["$AA_IM[X]"], 8.0);
}

} // namespace
