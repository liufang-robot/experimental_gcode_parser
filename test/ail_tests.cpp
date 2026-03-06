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
  EXPECT_EQ(l1.plane_effective, gcode::WorkingPlane::XY);
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

TEST(AilTest, G0LowersToLinearInstructionWithG0Opcode) {
  const auto result = gcode::parseAndLowerAil("G0 X10 Y20 F500\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilLinearMoveInstruction>(
      result.instructions[0]));
  const auto &move =
      std::get<gcode::AilLinearMoveInstruction>(result.instructions[0]);
  EXPECT_EQ(move.opcode, "G0");
  EXPECT_EQ(move.modal.code, "G0");
  ASSERT_TRUE(move.target_pose.x.has_value());
  ASSERT_TRUE(move.target_pose.y.has_value());
  EXPECT_TRUE(closeEnough(*move.target_pose.x, 10.0));
  EXPECT_TRUE(closeEnough(*move.target_pose.y, 20.0));
  ASSERT_TRUE(move.feed.has_value());
  EXPECT_TRUE(closeEnough(*move.feed, 500.0));

  const auto json = nlohmann::json::parse(gcode::ailToJsonString(result));
  EXPECT_EQ(json["instructions"][0]["kind"], "motion_linear");
  EXPECT_EQ(json["instructions"][0]["opcode"], "G0");
}

TEST(AilTest, EmitsMFunctionInstructionsInAilAndJson) {
  const auto result = gcode::parseAndLowerAil("N10 M3\nN20 M2=3 M5\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_TRUE(result.rejected_lines.empty());
  ASSERT_EQ(result.instructions.size(), 3u);

  ASSERT_TRUE(std::holds_alternative<gcode::AilMCodeInstruction>(
      result.instructions[0]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilMCodeInstruction>(
      result.instructions[1]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilMCodeInstruction>(
      result.instructions[2]));

  const auto &m0 = std::get<gcode::AilMCodeInstruction>(result.instructions[0]);
  EXPECT_EQ(m0.source.line, 1);
  ASSERT_TRUE(m0.source.line_number.has_value());
  EXPECT_EQ(*m0.source.line_number, 10);
  EXPECT_FALSE(m0.address_extension.has_value());
  EXPECT_EQ(m0.value, 3);

  const auto &m1 = std::get<gcode::AilMCodeInstruction>(result.instructions[1]);
  EXPECT_EQ(m1.source.line, 2);
  ASSERT_TRUE(m1.source.line_number.has_value());
  EXPECT_EQ(*m1.source.line_number, 20);
  ASSERT_TRUE(m1.address_extension.has_value());
  EXPECT_EQ(*m1.address_extension, 2);
  EXPECT_EQ(m1.value, 3);

  const auto &m2 = std::get<gcode::AilMCodeInstruction>(result.instructions[2]);
  EXPECT_FALSE(m2.address_extension.has_value());
  EXPECT_EQ(m2.value, 5);

  const auto json = nlohmann::json::parse(gcode::ailToJsonString(result));
  ASSERT_EQ(json["instructions"].size(), 3u);
  EXPECT_EQ(json["instructions"][0]["kind"], "m_function");
  EXPECT_EQ(json["instructions"][0]["value"], 3);
  EXPECT_TRUE(json["instructions"][0]["address_extension"].is_null());
  EXPECT_EQ(json["instructions"][1]["address_extension"], 2);
  EXPECT_EQ(json["instructions"][2]["value"], 5);
}

TEST(AilTest, EmitsToolSelectAndToolChangeInstructionsWithDeferredTiming) {
  gcode::LowerOptions options;
  options.tool_change_mode = gcode::ToolChangeMode::DeferredM6;
  const auto result =
      gcode::parseAndLowerAil("N10 T12\nN20 M6\nN30 T1=99\n", options);
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_TRUE(result.rejected_lines.empty());
  ASSERT_EQ(result.instructions.size(), 3u);

  ASSERT_TRUE(std::holds_alternative<gcode::AilToolSelectInstruction>(
      result.instructions[0]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilToolChangeInstruction>(
      result.instructions[1]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilToolSelectInstruction>(
      result.instructions[2]));

  const auto &select0 =
      std::get<gcode::AilToolSelectInstruction>(result.instructions[0]);
  EXPECT_EQ(select0.selector_value, "12");
  EXPECT_FALSE(select0.selector_index.has_value());
  EXPECT_EQ(select0.timing, gcode::ToolActionTiming::DeferredUntilM6);

  const auto &change =
      std::get<gcode::AilToolChangeInstruction>(result.instructions[1]);
  EXPECT_EQ(change.timing, gcode::ToolActionTiming::DeferredUntilM6);

  const auto &select1 =
      std::get<gcode::AilToolSelectInstruction>(result.instructions[2]);
  ASSERT_TRUE(select1.selector_index.has_value());
  EXPECT_EQ(*select1.selector_index, 1);
  EXPECT_EQ(select1.selector_value, "99");
  EXPECT_EQ(select1.timing, gcode::ToolActionTiming::DeferredUntilM6);

  const auto json = nlohmann::json::parse(gcode::ailToJsonString(result));
  EXPECT_EQ(json["instructions"][0]["kind"], "tool_select");
  EXPECT_EQ(json["instructions"][0]["selector_value"], "12");
  EXPECT_EQ(json["instructions"][0]["timing"], "deferred_until_m6");
  EXPECT_EQ(json["instructions"][1]["kind"], "tool_change");
  EXPECT_EQ(json["instructions"][1]["opcode"], "M6");
  EXPECT_EQ(json["instructions"][1]["timing"], "deferred_until_m6");
}

TEST(AilTest, EmitsToolSelectImmediateTimingWhenDirectModeConfigured) {
  gcode::LowerOptions options;
  options.tool_change_mode = gcode::ToolChangeMode::DirectT;
  const auto result = gcode::parseAndLowerAil("T12\n", options);
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilToolSelectInstruction>(
      result.instructions[0]));
  const auto &select =
      std::get<gcode::AilToolSelectInstruction>(result.instructions[0]);
  EXPECT_EQ(select.timing, gcode::ToolActionTiming::Immediate);
}

TEST(AilTest, EmitsReturnBoundaryInstructionsForRetAndM17) {
  const auto result = gcode::parseAndLowerAil("RET\nM17\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 2u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilReturnBoundaryInstruction>(
      result.instructions[0]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilReturnBoundaryInstruction>(
      result.instructions[1]));

  const auto &ret =
      std::get<gcode::AilReturnBoundaryInstruction>(result.instructions[0]);
  const auto &m17 =
      std::get<gcode::AilReturnBoundaryInstruction>(result.instructions[1]);
  EXPECT_EQ(ret.opcode, "RET");
  EXPECT_EQ(m17.opcode, "M17");

  const auto json = nlohmann::json::parse(gcode::ailToJsonString(result));
  EXPECT_EQ(json["instructions"][0]["kind"], "return_boundary");
  EXPECT_EQ(json["instructions"][0]["opcode"], "RET");
  EXPECT_EQ(json["instructions"][1]["kind"], "return_boundary");
  EXPECT_EQ(json["instructions"][1]["opcode"], "M17");
}

TEST(AilTest, EmitsSubprogramCallInstructionsForDirectAndPRepeatForms) {
  const auto result =
      gcode::parseAndLowerAil("L1001\nL1002 P3\nP=2 L1003\nG1 X1\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 4u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilSubprogramCallInstruction>(
      result.instructions[0]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilSubprogramCallInstruction>(
      result.instructions[1]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilSubprogramCallInstruction>(
      result.instructions[2]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilLinearMoveInstruction>(
      result.instructions[3]));

  const auto &call0 =
      std::get<gcode::AilSubprogramCallInstruction>(result.instructions[0]);
  const auto &call1 =
      std::get<gcode::AilSubprogramCallInstruction>(result.instructions[1]);
  const auto &call2 =
      std::get<gcode::AilSubprogramCallInstruction>(result.instructions[2]);
  EXPECT_EQ(call0.target, "L1001");
  EXPECT_FALSE(call0.repeat_count.has_value());
  EXPECT_EQ(call1.target, "L1002");
  ASSERT_TRUE(call1.repeat_count.has_value());
  EXPECT_EQ(*call1.repeat_count, 3);
  EXPECT_EQ(call2.target, "L1003");
  ASSERT_TRUE(call2.repeat_count.has_value());
  EXPECT_EQ(*call2.repeat_count, 2);

  const auto json = nlohmann::json::parse(gcode::ailToJsonString(result));
  EXPECT_EQ(json["instructions"][0]["kind"], "subprogram_call");
  EXPECT_EQ(json["instructions"][0]["target"], "L1001");
  EXPECT_TRUE(json["instructions"][0]["repeat_count"].is_null());
  EXPECT_EQ(json["instructions"][1]["repeat_count"], 3);
  EXPECT_EQ(json["instructions"][2]["repeat_count"], 2);
}

TEST(AilTest, EmitsIsoM98SubprogramCallWhenEnabled) {
  gcode::LowerOptions options;
  options.enable_iso_m98_calls = true;
  const auto result = gcode::parseAndLowerAil("M98 P1000\n", options);
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilSubprogramCallInstruction>(
      result.instructions[0]));
  const auto &call =
      std::get<gcode::AilSubprogramCallInstruction>(result.instructions[0]);
  EXPECT_EQ(call.target, "1000");
  EXPECT_FALSE(call.repeat_count.has_value());
}

TEST(AilTest, EmitsQuotedSubprogramCallInstruction) {
  const auto result = gcode::parseAndLowerAil("\"DIR/SPF1000\"\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilSubprogramCallInstruction>(
      result.instructions[0]));
  const auto &call =
      std::get<gcode::AilSubprogramCallInstruction>(result.instructions[0]);
  EXPECT_EQ(call.target, "DIR/SPF1000");
  EXPECT_FALSE(call.repeat_count.has_value());
}

TEST(AilTest, EmitsUnquotedAlphabeticSubprogramCallInstruction) {
  const auto result = gcode::parseAndLowerAil("ALIAS\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilSubprogramCallInstruction>(
      result.instructions[0]));
  const auto &call =
      std::get<gcode::AilSubprogramCallInstruction>(result.instructions[0]);
  EXPECT_EQ(call.target, "ALIAS");
  EXPECT_FALSE(call.repeat_count.has_value());
}

TEST(AilTest, EmitsProcDeclarationAsLabelInstruction) {
  const auto result = gcode::parseAndLowerAil("PROC MAIN\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilLabelInstruction>(
      result.instructions[0]));
  const auto &decl =
      std::get<gcode::AilLabelInstruction>(result.instructions[0]);
  EXPECT_EQ(decl.name, "MAIN");
}

TEST(AilTest, EmitsQuotedProcDeclarationAsLabelInstruction) {
  const auto result = gcode::parseAndLowerAil("PROC \"DIR/SPF1000\"\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilLabelInstruction>(
      result.instructions[0]));
  const auto &decl =
      std::get<gcode::AilLabelInstruction>(result.instructions[0]);
  EXPECT_EQ(decl.name, "DIR/SPF1000");
}

TEST(AilTest, EmitsLowercaseProcDeclarationAsLabelInstruction) {
  const auto result = gcode::parseAndLowerAil("proc main\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilLabelInstruction>(
      result.instructions[0]));
  const auto &decl =
      std::get<gcode::AilLabelInstruction>(result.instructions[0]);
  EXPECT_EQ(decl.name, "MAIN");
}

TEST(AilTest, EmitsMixedCaseProcDeclarationAsLabelInstruction) {
  const auto result = gcode::parseAndLowerAil("PrOc Mix\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilLabelInstruction>(
      result.instructions[0]));
  const auto &decl =
      std::get<gcode::AilLabelInstruction>(result.instructions[0]);
  EXPECT_EQ(decl.name, "MIX");
}

TEST(AilTest, ReportsErrorForProcWithoutTargetName) {
  const auto result = gcode::parseAndLowerAil("PROC\n");
  EXPECT_TRUE(result.instructions.empty());
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_EQ(result.diagnostics.back().severity,
            gcode::Diagnostic::Severity::Error);
  EXPECT_NE(result.diagnostics.back().message.find("expected PROC <name>"),
            std::string::npos);
}

TEST(AilTest, ReportsErrorForProcHeadWithEqualForm) {
  const auto result = gcode::parseAndLowerAil("PROC=MAIN\n");
  EXPECT_TRUE(result.instructions.empty());
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_EQ(result.diagnostics.back().severity,
            gcode::Diagnostic::Severity::Error);
  EXPECT_NE(result.diagnostics.back().message.find("expected PROC <name>"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.column, 1);
}

TEST(AilTest, ReportsErrorForLowercaseProcHeadWithEqualForm) {
  const auto result = gcode::parseAndLowerAil("proc=main\n");
  EXPECT_TRUE(result.instructions.empty());
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_EQ(result.diagnostics.back().severity,
            gcode::Diagnostic::Severity::Error);
  EXPECT_NE(result.diagnostics.back().message.find("expected PROC <name>"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.column, 1);
}

TEST(AilTest, ReportsErrorForMixedCaseProcHeadWithEqualForm) {
  const auto result = gcode::parseAndLowerAil("PrOc=main\n");
  EXPECT_TRUE(result.instructions.empty());
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_EQ(result.diagnostics.back().severity,
            gcode::Diagnostic::Severity::Error);
  EXPECT_NE(result.diagnostics.back().message.find("expected PROC <name>"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.column, 1);
}

TEST(AilTest, ReportsSyntaxErrorForProcQuotedEqualForm) {
  const auto result = gcode::parseAndLowerAil("PROC=\"DIR/SPF1000\"\n");
  EXPECT_TRUE(result.instructions.empty());
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_EQ(result.diagnostics.back().severity,
            gcode::Diagnostic::Severity::Error);
  EXPECT_NE(result.diagnostics.back().message.find("syntax error"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.column, 6);
}

TEST(AilTest, ReportsErrorForMalformedProcDeclarationShape) {
  const auto result = gcode::parseAndLowerAil("PROC MAIN P2\n");
  EXPECT_TRUE(result.instructions.empty());
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_EQ(result.diagnostics.back().severity,
            gcode::Diagnostic::Severity::Error);
  EXPECT_NE(result.diagnostics.back().message.find("expected PROC <name>"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.column, 11);
}

TEST(AilTest, ReportsErrorForMalformedQuotedProcDeclarationShape) {
  const auto result = gcode::parseAndLowerAil("PROC \"DIR/SPF1000\" P2\n");
  EXPECT_TRUE(result.instructions.empty());
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_EQ(result.diagnostics.back().severity,
            gcode::Diagnostic::Severity::Error);
  EXPECT_NE(result.diagnostics.back().message.find("expected PROC <name>"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.column, 20);
}

TEST(AilTest, ReportsErrorForInvalidProcTargetWordLocation) {
  const auto result = gcode::parseAndLowerAil("PROC G1\n");
  EXPECT_TRUE(result.instructions.empty());
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_EQ(result.diagnostics.back().severity,
            gcode::Diagnostic::Severity::Error);
  EXPECT_NE(result.diagnostics.back().message.find("expected PROC <name>"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.column, 6);
}

TEST(AilTest, AcceptsEmptyInlineProcSignatureSuffixWithoutWarning) {
  const auto result = gcode::parseAndLowerAil("PROC MAIN()\n");
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilLabelInstruction>(
      result.instructions[0]));
  const auto &decl =
      std::get<gcode::AilLabelInstruction>(result.instructions[0]);
  EXPECT_EQ(decl.name, "MAIN");
  EXPECT_TRUE(result.diagnostics.empty());
}

TEST(AilTest, WarnsWhenInlineProcSignatureSuffixHasParameters) {
  const auto result = gcode::parseAndLowerAil("PROC MAIN(R1)\n");
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilLabelInstruction>(
      result.instructions[0]));
  const auto &decl =
      std::get<gcode::AilLabelInstruction>(result.instructions[0]);
  EXPECT_EQ(decl.name, "MAIN");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_EQ(result.diagnostics.back().severity,
            gcode::Diagnostic::Severity::Warning);
  EXPECT_NE(result.diagnostics.back().message.find("PROC signature parameters"),
            std::string::npos);
}

TEST(AilTest, AcceptsEmptyInlineProcSignatureSuffixWithWhitespace) {
  const auto result = gcode::parseAndLowerAil("PROC MAIN ()\n");
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilLabelInstruction>(
      result.instructions[0]));
  const auto &decl =
      std::get<gcode::AilLabelInstruction>(result.instructions[0]);
  EXPECT_EQ(decl.name, "MAIN");
  EXPECT_TRUE(result.diagnostics.empty());
}

TEST(AilTest, WarnsWhenInlineProcSignatureSuffixHasWhitespace) {
  const auto result = gcode::parseAndLowerAil("PROC MAIN (R1)\n");
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilLabelInstruction>(
      result.instructions[0]));
  const auto &decl =
      std::get<gcode::AilLabelInstruction>(result.instructions[0]);
  EXPECT_EQ(decl.name, "MAIN");

  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_EQ(result.diagnostics.back().severity,
            gcode::Diagnostic::Severity::Warning);
  EXPECT_NE(result.diagnostics.back().message.find("PROC signature parameters"),
            std::string::npos);
}

TEST(AilTest, AcceptsEmptyInlineSubprogramCallArgumentsWithoutWarning) {
  const auto result = gcode::parseAndLowerAil("MAIN()\n");
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilSubprogramCallInstruction>(
      result.instructions[0]));
  const auto &call =
      std::get<gcode::AilSubprogramCallInstruction>(result.instructions[0]);
  EXPECT_EQ(call.target, "MAIN");
  EXPECT_TRUE(result.diagnostics.empty());
}

TEST(AilTest, AcceptsEmptyInlineSubprogramCallArgumentsWithWhitespace) {
  const auto result = gcode::parseAndLowerAil("MAIN ()\n");
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilSubprogramCallInstruction>(
      result.instructions[0]));
  const auto &call =
      std::get<gcode::AilSubprogramCallInstruction>(result.instructions[0]);
  EXPECT_EQ(call.target, "MAIN");
  EXPECT_TRUE(result.diagnostics.empty());
}

TEST(AilTest, WarnsWhenInlineSubprogramCallArgumentsAreIgnored) {
  const auto result = gcode::parseAndLowerAil("MAIN(R1)\n");
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilSubprogramCallInstruction>(
      result.instructions[0]));
  const auto &call =
      std::get<gcode::AilSubprogramCallInstruction>(result.instructions[0]);
  EXPECT_EQ(call.target, "MAIN");

  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_EQ(result.diagnostics.back().severity,
            gcode::Diagnostic::Severity::Warning);
  EXPECT_NE(result.diagnostics.back().message.find("call arguments"),
            std::string::npos);
}

TEST(AilTest, WarnsWhenInlineSubprogramCallArgumentsHaveWhitespace) {
  const auto result = gcode::parseAndLowerAil("MAIN (R1)\n");
  ASSERT_EQ(result.instructions.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilSubprogramCallInstruction>(
      result.instructions[0]));
  const auto &call =
      std::get<gcode::AilSubprogramCallInstruction>(result.instructions[0]);
  EXPECT_EQ(call.target, "MAIN");

  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_EQ(result.diagnostics.back().severity,
            gcode::Diagnostic::Severity::Warning);
  EXPECT_NE(result.diagnostics.back().message.find("call arguments"),
            std::string::npos);
}

TEST(AilTest, EmitsRapidTraverseModeInstructionsForRTLIONAndRTLIOF) {
  const auto result = gcode::parseAndLowerAil("RTLION\nG0 X1\nRTLIOF\nG0 X2\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_TRUE(result.rejected_lines.empty());
  ASSERT_EQ(result.instructions.size(), 4u);

  ASSERT_TRUE(std::holds_alternative<gcode::AilRapidTraverseModeInstruction>(
      result.instructions[0]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilLinearMoveInstruction>(
      result.instructions[1]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilRapidTraverseModeInstruction>(
      result.instructions[2]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilLinearMoveInstruction>(
      result.instructions[3]));

  const auto &mode0 =
      std::get<gcode::AilRapidTraverseModeInstruction>(result.instructions[0]);
  EXPECT_EQ(mode0.mode, gcode::RapidInterpolationMode::Linear);

  const auto &mode1 =
      std::get<gcode::AilRapidTraverseModeInstruction>(result.instructions[2]);
  EXPECT_EQ(mode1.mode, gcode::RapidInterpolationMode::NonLinear);

  const auto json = nlohmann::json::parse(gcode::ailToJsonString(result));
  EXPECT_EQ(json["instructions"][0]["kind"], "rapid_mode");
  EXPECT_EQ(json["instructions"][0]["opcode"], "RTLION");
  EXPECT_EQ(json["instructions"][0]["mode"], "linear");
  EXPECT_EQ(json["instructions"][2]["opcode"], "RTLIOF");
  EXPECT_EQ(json["instructions"][2]["mode"], "nonlinear");
}

TEST(AilTest, EmitsToolRadiusCompInstructionsForG40G41G42) {
  const auto result = gcode::parseAndLowerAil("G40\nG41\nG42\nG1 X1\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_TRUE(result.rejected_lines.empty());
  ASSERT_EQ(result.instructions.size(), 4u);

  ASSERT_TRUE(std::holds_alternative<gcode::AilToolRadiusCompInstruction>(
      result.instructions[0]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilToolRadiusCompInstruction>(
      result.instructions[1]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilToolRadiusCompInstruction>(
      result.instructions[2]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilLinearMoveInstruction>(
      result.instructions[3]));

  const auto &comp0 =
      std::get<gcode::AilToolRadiusCompInstruction>(result.instructions[0]);
  EXPECT_EQ(comp0.mode, gcode::ToolRadiusCompMode::Off);
  const auto &comp1 =
      std::get<gcode::AilToolRadiusCompInstruction>(result.instructions[1]);
  EXPECT_EQ(comp1.mode, gcode::ToolRadiusCompMode::Left);
  const auto &comp2 =
      std::get<gcode::AilToolRadiusCompInstruction>(result.instructions[2]);
  EXPECT_EQ(comp2.mode, gcode::ToolRadiusCompMode::Right);

  const auto json = nlohmann::json::parse(gcode::ailToJsonString(result));
  EXPECT_EQ(json["instructions"][0]["kind"], "tool_radius_comp");
  EXPECT_EQ(json["instructions"][0]["opcode"], "G40");
  EXPECT_EQ(json["instructions"][0]["mode"], "off");
  EXPECT_EQ(json["instructions"][1]["opcode"], "G41");
  EXPECT_EQ(json["instructions"][1]["mode"], "left");
  EXPECT_EQ(json["instructions"][2]["opcode"], "G42");
  EXPECT_EQ(json["instructions"][2]["mode"], "right");
}

TEST(AilTest, EmitsWorkingPlaneInstructionsForG17G18G19) {
  const auto result = gcode::parseAndLowerAil("G17\nG18\nG19\nG1 X1\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_TRUE(result.rejected_lines.empty());
  ASSERT_EQ(result.instructions.size(), 4u);

  ASSERT_TRUE(std::holds_alternative<gcode::AilWorkingPlaneInstruction>(
      result.instructions[0]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilWorkingPlaneInstruction>(
      result.instructions[1]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilWorkingPlaneInstruction>(
      result.instructions[2]));

  const auto &p0 =
      std::get<gcode::AilWorkingPlaneInstruction>(result.instructions[0]);
  EXPECT_EQ(p0.plane, gcode::WorkingPlane::XY);
  const auto &p1 =
      std::get<gcode::AilWorkingPlaneInstruction>(result.instructions[1]);
  EXPECT_EQ(p1.plane, gcode::WorkingPlane::ZX);
  const auto &p2 =
      std::get<gcode::AilWorkingPlaneInstruction>(result.instructions[2]);
  EXPECT_EQ(p2.plane, gcode::WorkingPlane::YZ);

  const auto json = nlohmann::json::parse(gcode::ailToJsonString(result));
  EXPECT_EQ(json["instructions"][0]["kind"], "working_plane");
  EXPECT_EQ(json["instructions"][0]["opcode"], "G17");
  EXPECT_EQ(json["instructions"][0]["plane"], "xy");
  EXPECT_EQ(json["instructions"][1]["opcode"], "G18");
  EXPECT_EQ(json["instructions"][1]["plane"], "zx");
  EXPECT_EQ(json["instructions"][2]["opcode"], "G19");
  EXPECT_EQ(json["instructions"][2]["plane"], "yz");
}

TEST(AilTest, AppliesCurrentWorkingPlaneToArcInstructions) {
  const auto result = gcode::parseAndLowerAil("G18\nG2 X10 Z20 I1 K2 F100\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 2u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilArcMoveInstruction>(
      result.instructions[1]));
  const auto &arc =
      std::get<gcode::AilArcMoveInstruction>(result.instructions[1]);
  EXPECT_EQ(arc.plane_effective, gcode::WorkingPlane::ZX);

  const auto json = nlohmann::json::parse(gcode::ailToJsonString(result));
  EXPECT_EQ(json["instructions"][1]["kind"], "motion_arc");
  EXPECT_EQ(json["instructions"][1]["plane_effective"], "zx");
}

TEST(AilTest, AppliesCurrentRapidModeToFollowingG0Instructions) {
  const auto result =
      gcode::parseAndLowerAil("G0 X1\nRTLIOF\nG0 X2\nRTLION\nG0 X3\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 5u);

  ASSERT_TRUE(std::holds_alternative<gcode::AilLinearMoveInstruction>(
      result.instructions[0]));
  const auto &g0_before_mode =
      std::get<gcode::AilLinearMoveInstruction>(result.instructions[0]);
  EXPECT_EQ(g0_before_mode.opcode, "G0");
  EXPECT_FALSE(g0_before_mode.rapid_mode_effective.has_value());

  ASSERT_TRUE(std::holds_alternative<gcode::AilLinearMoveInstruction>(
      result.instructions[2]));
  const auto &g0_nonlinear =
      std::get<gcode::AilLinearMoveInstruction>(result.instructions[2]);
  ASSERT_TRUE(g0_nonlinear.rapid_mode_effective.has_value());
  EXPECT_EQ(*g0_nonlinear.rapid_mode_effective,
            gcode::RapidInterpolationMode::NonLinear);

  ASSERT_TRUE(std::holds_alternative<gcode::AilLinearMoveInstruction>(
      result.instructions[4]));
  const auto &g0_linear =
      std::get<gcode::AilLinearMoveInstruction>(result.instructions[4]);
  ASSERT_TRUE(g0_linear.rapid_mode_effective.has_value());
  EXPECT_EQ(*g0_linear.rapid_mode_effective,
            gcode::RapidInterpolationMode::Linear);

  const auto json = nlohmann::json::parse(gcode::ailToJsonString(result));
  EXPECT_FALSE(json["instructions"][0].contains("rapid_mode_effective"));
  EXPECT_EQ(json["instructions"][2]["rapid_mode_effective"], "nonlinear");
  EXPECT_EQ(json["instructions"][4]["rapid_mode_effective"], "linear");
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

TEST(AilTest, BranchIfJsonIncludesLogicalAndConditionMetadata) {
  const auto result = gcode::parseAndLowerAil(
      "IF (R1 == 1) AND (R2 > 10) GOTOF TARGET\nTARGET:\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 2u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilBranchIfInstruction>(
      result.instructions[0]));

  const auto json = nlohmann::json::parse(gcode::ailToJsonString(result));
  ASSERT_EQ(json["instructions"][0]["kind"], "branch_if");
  EXPECT_TRUE(
      json["instructions"][0]["condition"]["has_logical_and"].get<bool>());
  ASSERT_EQ(json["instructions"][0]["condition"]["and_terms"].size(), 2u);
  EXPECT_EQ(json["instructions"][0]["condition"]["and_terms"][0], "(R1 == 1)");
  EXPECT_EQ(json["instructions"][0]["condition"]["and_terms"][1], "(R2 > 10)");
}

TEST(AilTest, StructuredIfElseLowersToBranchGotoAndLabels) {
  const auto result = gcode::parseAndLowerAil(
      "IF (R1 == 1) AND (R2 > 10)\nG1 Z-5 F10\nELSE\nG1 X0 Y5\nENDIF\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 7u);

  ASSERT_TRUE(std::holds_alternative<gcode::AilBranchIfInstruction>(
      result.instructions[0]));
  const auto &branch =
      std::get<gcode::AilBranchIfInstruction>(result.instructions[0]);
  EXPECT_EQ(branch.then_branch.target_kind, "label");
  ASSERT_TRUE(branch.else_branch.has_value());
  EXPECT_EQ(branch.else_branch->target_kind, "label");
  EXPECT_NE(branch.then_branch.target, branch.else_branch->target);

  EXPECT_TRUE(std::holds_alternative<gcode::AilLabelInstruction>(
      result.instructions[1]));
  EXPECT_TRUE(std::holds_alternative<gcode::AilLinearMoveInstruction>(
      result.instructions[2]));
  EXPECT_TRUE(std::holds_alternative<gcode::AilGotoInstruction>(
      result.instructions[3]));
  EXPECT_TRUE(std::holds_alternative<gcode::AilLabelInstruction>(
      result.instructions[4]));
  EXPECT_TRUE(std::holds_alternative<gcode::AilLinearMoveInstruction>(
      result.instructions[5]));
  EXPECT_TRUE(std::holds_alternative<gcode::AilLabelInstruction>(
      result.instructions[6]));
}

TEST(AilTest, KeepsLineNumberInInstructionSourceAndGotoTargetKind) {
  const auto result = gcode::parseAndLowerAil("N100 G1 X1\nGOTOF N100\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.instructions.size(), 2u);
  ASSERT_TRUE(std::holds_alternative<gcode::AilLinearMoveInstruction>(
      result.instructions[0]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilGotoInstruction>(
      result.instructions[1]));

  const auto &move =
      std::get<gcode::AilLinearMoveInstruction>(result.instructions[0]);
  ASSERT_TRUE(move.source.line_number.has_value());
  EXPECT_EQ(*move.source.line_number, 100);

  const auto &jump =
      std::get<gcode::AilGotoInstruction>(result.instructions[1]);
  EXPECT_EQ(jump.target_kind, "line_number");
  EXPECT_EQ(jump.target, "N100");
}

TEST(AilTest, ActiveSkipLevelRemovesMatchingLinesFromInstructionStream) {
  gcode::LowerOptions options;
  options.active_skip_levels = {1};
  const auto result =
      gcode::parseAndLowerAil("/1 G1 X10\n/2 G1 X20\nG1 X30\n", options);
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_TRUE(result.rejected_lines.empty());
  ASSERT_EQ(result.instructions.size(), 2u);

  ASSERT_TRUE(std::holds_alternative<gcode::AilLinearMoveInstruction>(
      result.instructions[0]));
  ASSERT_TRUE(std::holds_alternative<gcode::AilLinearMoveInstruction>(
      result.instructions[1]));

  const auto &first =
      std::get<gcode::AilLinearMoveInstruction>(result.instructions[0]);
  const auto &second =
      std::get<gcode::AilLinearMoveInstruction>(result.instructions[1]);
  ASSERT_TRUE(first.target_pose.x.has_value());
  ASSERT_TRUE(second.target_pose.x.has_value());
  EXPECT_TRUE(closeEnough(*first.target_pose.x, 20.0));
  EXPECT_TRUE(closeEnough(*second.target_pose.x, 30.0));
}

} // namespace
