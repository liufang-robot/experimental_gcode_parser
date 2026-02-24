#include "gtest/gtest.h"

#include "gcode_parser.h"
#include "messages.h"

namespace {

// Regression naming convention:
// Regression_<bug_or_issue_id>_<short_behavior>
TEST(RegressionTest, Regression_MixedCartesianAndPolar_G1ReportsError) {
  const std::string input = "N10 G1 X10 AP=90 RP=10\n";
  const auto result = gcode::parse(input);

  ASSERT_EQ(result.diagnostics.size(), 1u);
  const auto &diag = result.diagnostics.front();
  EXPECT_EQ(diag.severity, gcode::Diagnostic::Severity::Error);
  EXPECT_EQ(diag.location.line, 1);
  EXPECT_EQ(diag.location.column, 12);
  EXPECT_EQ(diag.message,
            "mixed cartesian (X/Y/Z/A) and polar (AP/RP) words in G1 line; "
            "choose one coordinate mode");
}

TEST(RegressionTest, Regression_DuplicateSameMotionWord_NoExclusiveError) {
  const std::string input = "G1 G1 X10 F100\n";
  const auto result = gcode::parseAndLower(input);

  EXPECT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.messages.size(), 1u);
  EXPECT_TRUE(std::holds_alternative<gcode::G1Message>(result.messages[0]));
}

TEST(RegressionTest, Regression_UnsupportedChars_HasAccurateLocation) {
  const std::string input = "G1 X10\nG1 @\n";
  const auto result = gcode::parse(input);

  ASSERT_EQ(result.diagnostics.size(), 1u);
  const auto &diag = result.diagnostics.front();
  EXPECT_EQ(diag.severity, gcode::Diagnostic::Severity::Error);
  EXPECT_EQ(diag.location.line, 2);
  EXPECT_EQ(diag.location.column, 4);
  EXPECT_NE(diag.message.find("unsupported characters"), std::string::npos);
}

TEST(RegressionTest, Regression_NonMotionGCode_DoesNotEmitMessage) {
  const std::string input = "G4 P100\n";
  const auto result = gcode::parseAndLower(input);

  EXPECT_TRUE(result.diagnostics.empty());
  EXPECT_TRUE(result.messages.empty());
  EXPECT_TRUE(result.rejected_lines.empty());
}

} // namespace
