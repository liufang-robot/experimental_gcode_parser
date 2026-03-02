#include <cmath>
#include <string>

#include <nlohmann/json.hpp>

#include "gtest/gtest.h"

#include "ail.h"
#include "ail_json.h"

namespace {

bool closeEnough(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-9; }

TEST(AilTest, LowersMotionSubsetIntoAilInstructions) {
  const std::string input =
      "N10 G1 X1 Y2 F3\nN20 G2 X4 Y5 I6 J7 F8\nN30 G4 S30\n";
  gcode::LowerOptions options;
  options.filename = "sample.ngc";
  const auto result = gcode::parseAndLowerAil(input, options);

  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_TRUE(result.rejected_lines.empty());
  ASSERT_EQ(result.instructions.size(), 3u);

  ASSERT_TRUE(std::holds_alternative<gcode::AilLinearMoveInstruction>(
      result.instructions[0]));
  const auto &l0 =
      std::get<gcode::AilLinearMoveInstruction>(result.instructions[0]);
  EXPECT_EQ(l0.source.line, 1);
  EXPECT_EQ(l0.modal.group, gcode::ModalGroupId::Motion);
  EXPECT_EQ(l0.modal.code, "G1");
  ASSERT_TRUE(l0.target_pose.x.has_value());
  EXPECT_TRUE(closeEnough(*l0.target_pose.x, 1.0));
  ASSERT_TRUE(l0.feed.has_value());
  EXPECT_TRUE(closeEnough(*l0.feed, 3.0));

  ASSERT_TRUE(std::holds_alternative<gcode::AilArcMoveInstruction>(
      result.instructions[1]));
  const auto &l1 =
      std::get<gcode::AilArcMoveInstruction>(result.instructions[1]);
  EXPECT_TRUE(l1.clockwise);
  EXPECT_EQ(l1.modal.code, "G2");
  ASSERT_TRUE(l1.arc.i.has_value());
  EXPECT_TRUE(closeEnough(*l1.arc.i, 6.0));

  ASSERT_TRUE(std::holds_alternative<gcode::AilDwellInstruction>(
      result.instructions[2]));
  const auto &l2 = std::get<gcode::AilDwellInstruction>(result.instructions[2]);
  EXPECT_EQ(l2.modal.group, gcode::ModalGroupId::NonModal);
  EXPECT_EQ(l2.modal.code, "G4");
  EXPECT_EQ(l2.dwell_mode, gcode::DwellMode::Revolutions);
  EXPECT_TRUE(closeEnough(l2.dwell_value, 30.0));
}

TEST(AilTest, FailFastRejectedLineIsPreserved) {
  const auto result = gcode::parseAndLowerAil("G1 X10\nG1 G2 X20\nG1 X30\n");
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_EQ(result.rejected_lines.size(), 1u);
  EXPECT_EQ(result.rejected_lines[0].source.line, 2);
}

TEST(AilTest, JsonContainsInstructionSchemaAndModal) {
  const auto result = gcode::parseAndLowerAil("G1 X10\nG4 F3\n");
  const auto j = nlohmann::json::parse(gcode::ailToJsonString(result));

  EXPECT_EQ(j.value("schema_version", 0), 1);
  ASSERT_TRUE(j.contains("instructions"));
  ASSERT_TRUE(j["instructions"].is_array());
  ASSERT_EQ(j["instructions"].size(), 2u);
  EXPECT_EQ(j["instructions"][0]["kind"], "motion_linear");
  EXPECT_EQ(j["instructions"][0]["modal"]["group"], "GGroup1");
  EXPECT_EQ(j["instructions"][1]["kind"], "dwell");
  EXPECT_EQ(j["instructions"][1]["modal"]["group"], "GGroup2");
}

TEST(AilTest, AssignmentProducesTypedExpressionTree) {
  const auto result = gcode::parseAndLowerAil("R1 = $P_ACT_X + 2*R2\n");
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilAssignInstruction>(
      result.instructions[0]));
  const auto &assign =
      std::get<gcode::AilAssignInstruction>(result.instructions[0]);
  EXPECT_EQ(assign.lhs, "R1");
  ASSERT_TRUE(assign.rhs != nullptr);

  const auto json = nlohmann::json::parse(gcode::ailToJsonString(result));
  EXPECT_EQ(json["instructions"][0]["kind"], "assign");
  EXPECT_EQ(json["instructions"][0]["lhs"], "R1");
  EXPECT_EQ(json["instructions"][0]["rhs"]["kind"], "binary");
}

TEST(AilTest, LowersControlFlowInstructions) {
  const auto result = gcode::parseAndLowerAil(
      "START:\nIF R1 == 1 GOTOF END\nGOTO START\nEND:\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 4u);
  EXPECT_TRUE(std::holds_alternative<gcode::AilLabelInstruction>(
      result.instructions[0]));
  EXPECT_TRUE(std::holds_alternative<gcode::AilBranchIfInstruction>(
      result.instructions[1]));
  EXPECT_TRUE(std::holds_alternative<gcode::AilGotoInstruction>(
      result.instructions[2]));
  EXPECT_TRUE(std::holds_alternative<gcode::AilLabelInstruction>(
      result.instructions[3]));

  const auto json = nlohmann::json::parse(gcode::ailToJsonString(result));
  ASSERT_EQ(json["instructions"].size(), 4u);
  EXPECT_EQ(json["instructions"][0]["kind"], "label");
  EXPECT_EQ(json["instructions"][1]["kind"], "branch_if");
  EXPECT_EQ(json["instructions"][2]["kind"], "goto");
}

} // namespace
