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

} // namespace
