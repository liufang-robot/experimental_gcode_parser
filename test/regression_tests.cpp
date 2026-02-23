#include "gtest/gtest.h"

#include "gcode_parser.h"

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

} // namespace
