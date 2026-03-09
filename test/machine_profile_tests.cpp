#include "gtest/gtest.h"

#include "machine_profile.h"

namespace {

TEST(MachineProfileTest, Siemens840DBaselineDefaults) {
  const auto profile = gcode::MachineProfile::siemens840dBaseline();

  EXPECT_EQ(profile.controller, gcode::ControllerKind::Siemens840D);
  EXPECT_EQ(profile.tool_change_mode, gcode::ToolChangeMode::DeferredM6);
  EXPECT_FALSE(profile.tool_management);
  EXPECT_EQ(profile.unknown_mcode_policy, gcode::ErrorPolicy::Error);
  EXPECT_EQ(profile.modal_conflict_policy, gcode::ErrorPolicy::Error);
}

TEST(MachineProfileTest, Siemens840DWorkOffsetRangeSet) {
  const auto profile = gcode::MachineProfile::siemens840dBaseline();

  EXPECT_TRUE(profile.supported_work_offsets.contains(54));
  EXPECT_TRUE(profile.supported_work_offsets.contains(57));
  EXPECT_TRUE(profile.supported_work_offsets.contains(505));
  EXPECT_TRUE(profile.supported_work_offsets.contains(599));
  EXPECT_FALSE(profile.supported_work_offsets.contains(53));
  EXPECT_FALSE(profile.supported_work_offsets.contains(600));
}

TEST(MachineProfileTest, RangeSetRejectsInvalidRange) {
  gcode::RangeSet ranges;
  ranges.add(10, 5);

  EXPECT_TRUE(ranges.empty());
  EXPECT_FALSE(ranges.contains(10));
}

} // namespace
