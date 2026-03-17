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
  ASSERT_EQ(trace.events.size(), 2u);
  EXPECT_EQ(trace.events[0].type, "modal_update");
  ASSERT_TRUE(trace.events[0].source.has_value());
  EXPECT_EQ(trace.events[0].source->line, 1);
  ASSERT_TRUE(trace.events[0].data.contains("changes"));
  EXPECT_EQ(trace.events[0].data["changes"]["working_plane"], "zx");
  EXPECT_EQ(trace.events[1].type, "completed");
}

} // namespace
