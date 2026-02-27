#include <string>

#include "gtest/gtest.h"

#include "semantic_rules.h"

namespace {

gcode::Word makeWord(const std::string &head, const std::string &value,
                     int line, int column) {
  gcode::Word word;
  word.head = head;
  word.value = value;
  word.location = {line, column};
  return word;
}

gcode::Line makeLine(int line_index, std::initializer_list<gcode::Word> words) {
  gcode::Line line;
  line.line_index = line_index;
  for (const auto &word : words) {
    line.items.push_back(word);
  }
  return line;
}

TEST(SemanticRulesTest, ReportsExclusiveMotionFamilyViolation) {
  gcode::ParseResult result;
  result.program.lines.push_back(
      makeLine(1, {makeWord("G", "1", 1, 1), makeWord("G", "2", 1, 4)}));

  gcode::addSemanticDiagnostics(result);

  ASSERT_EQ(result.diagnostics.size(), 1u);
  EXPECT_EQ(result.diagnostics[0].severity, gcode::Diagnostic::Severity::Error);
  EXPECT_EQ(result.diagnostics[0].location.line, 1);
  EXPECT_NE(result.diagnostics[0].message.find("multiple motion commands"),
            std::string::npos);
}

TEST(SemanticRulesTest, ReportsMixedCoordinateModeForG1) {
  gcode::ParseResult result;
  result.program.lines.push_back(
      makeLine(1, {makeWord("G", "1", 1, 1), makeWord("X", "10", 1, 4),
                   makeWord("AP", "90", 1, 8)}));

  gcode::addSemanticDiagnostics(result);

  ASSERT_EQ(result.diagnostics.size(), 1u);
  EXPECT_EQ(result.diagnostics[0].severity, gcode::Diagnostic::Severity::Error);
  EXPECT_EQ(result.diagnostics[0].location.line, 1);
  EXPECT_NE(result.diagnostics[0].message.find("choose one coordinate mode"),
            std::string::npos);
}

TEST(SemanticRulesTest, ReportsG4NotSeparateBlock) {
  gcode::ParseResult result;
  result.program.lines.push_back(
      makeLine(1, {makeWord("G", "4", 1, 1), makeWord("F", "3", 1, 4),
                   makeWord("X", "10", 1, 7)}));

  gcode::addSemanticDiagnostics(result);

  ASSERT_EQ(result.diagnostics.size(), 1u);
  EXPECT_EQ(result.diagnostics[0].severity, gcode::Diagnostic::Severity::Error);
  EXPECT_EQ(result.diagnostics[0].location.line, 1);
  EXPECT_NE(result.diagnostics[0].message.find("separate block"),
            std::string::npos);
}

TEST(SemanticRulesTest, ReportsG4RequiresExactlyOneModeWord) {
  gcode::ParseResult missing_mode;
  missing_mode.program.lines.push_back(makeLine(1, {makeWord("G", "4", 1, 1)}));
  gcode::addSemanticDiagnostics(missing_mode);
  ASSERT_EQ(missing_mode.diagnostics.size(), 1u);
  EXPECT_NE(missing_mode.diagnostics[0].message.find("requires F"),
            std::string::npos);

  gcode::ParseResult both_modes;
  both_modes.program.lines.push_back(
      makeLine(1, {makeWord("G", "4", 1, 1), makeWord("F", "3", 1, 4),
                   makeWord("S", "30", 1, 7)}));
  gcode::addSemanticDiagnostics(both_modes);
  ASSERT_EQ(both_modes.diagnostics.size(), 1u);
  EXPECT_NE(both_modes.diagnostics[0].message.find("not both"),
            std::string::npos);
}

TEST(SemanticRulesTest, ReportsG4NumericValueRequired) {
  gcode::ParseResult result;
  result.program.lines.push_back(
      makeLine(1, {makeWord("G", "4", 1, 1), makeWord("F", "abc", 1, 4)}));
  gcode::addSemanticDiagnostics(result);

  ASSERT_EQ(result.diagnostics.size(), 1u);
  EXPECT_NE(result.diagnostics[0].message.find("must be numeric"),
            std::string::npos);
}

} // namespace
