#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "gtest/gtest.h"

#include "ast_printer.h"
#include "gcode_parser.h"

namespace {

std::string readFile(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::in);
  std::stringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

void expectGolden(const std::filesystem::path &input_path,
                  const std::filesystem::path &golden_path) {
  const auto input = readFile(input_path);
  const auto expected = readFile(golden_path);
  const auto result = gcode::parse(input);
  const auto actual = gcode::format(result);
  EXPECT_EQ(actual, expected) << "golden mismatch: " << input_path.filename();
}

TEST(ParserGoldenTest, G1Samples) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto testdata = source_dir / "testdata";
  expectGolden(testdata / "g1_samples.ngc", testdata / "g1_samples.golden.txt");
}

TEST(ParserGoldenTest, G2G3Samples) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto testdata = source_dir / "testdata";
  expectGolden(testdata / "g2g3_samples.ngc",
               testdata / "g2g3_samples.golden.txt");
}

TEST(ParserGoldenTest, CombinedSamples) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto testdata = source_dir / "testdata";
  expectGolden(testdata / "sample1.ngc", testdata / "sample1.golden.txt");
}

TEST(ParserDiagnosticsTest, ActionableSyntaxAndSemanticMessages) {
  {
    const auto result = gcode::parse("G1 @\n");
    ASSERT_EQ(result.diagnostics.size(), 1u);
    EXPECT_NE(result.diagnostics[0].message.find("syntax error:"),
              std::string::npos);
    EXPECT_NE(result.diagnostics[0].message.find("unsupported characters"),
              std::string::npos);
  }

  {
    const auto result = gcode::parse("G1 X10 AP=90 RP=10\n");
    ASSERT_EQ(result.diagnostics.size(), 1u);
    EXPECT_NE(result.diagnostics[0].message.find("choose one coordinate mode"),
              std::string::npos);
  }

  {
    const auto result = gcode::parse("G4 F3 X10\n");
    ASSERT_EQ(result.diagnostics.size(), 1u);
    EXPECT_NE(result.diagnostics[0].message.find("separate block"),
              std::string::npos);
  }
}

TEST(ParserExpressionTest, ParsesAssignmentExpressionIntoAst) {
  const auto result = gcode::parse("R1 = $P_ACT_X + 2*R2\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_TRUE(result.program.lines[0].assignment.has_value());
  EXPECT_EQ(result.program.lines[0].assignment->lhs, "R1");
}

TEST(ParserExpressionTest, ReportsExpressionSyntaxErrorLocation) {
  const auto result = gcode::parse("R1 = $P_ACT_X + * 2\n");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_NE(result.diagnostics[0].message.find("syntax error"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics[0].location.line, 1);
}

TEST(ParserControlFlowTest, ParsesGotoAndLabelWithUnderscore) {
  const auto result = gcode::parse("GOTO END_CODE\nEND_CODE:\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 2u);

  ASSERT_TRUE(result.program.lines[0].goto_statement.has_value());
  EXPECT_EQ(result.program.lines[0].goto_statement->opcode, "GOTO");
  EXPECT_EQ(result.program.lines[0].goto_statement->target, "END_CODE");
  EXPECT_EQ(result.program.lines[0].goto_statement->target_kind, "label");

  ASSERT_TRUE(result.program.lines[1].label_definition.has_value());
  EXPECT_EQ(result.program.lines[1].label_definition->name, "END_CODE");
}

TEST(ParserControlFlowTest, ParsesIfThenElseGoto) {
  const auto result = gcode::parse("IF R1-2 GOTOF END_CODE ELSE GOTOB RETRY\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_TRUE(result.program.lines[0].if_goto_statement.has_value());

  const auto &if_stmt = *result.program.lines[0].if_goto_statement;
  EXPECT_EQ(if_stmt.then_branch.opcode, "GOTOF");
  EXPECT_EQ(if_stmt.then_branch.target, "END_CODE");
  ASSERT_TRUE(if_stmt.else_branch.has_value());
  EXPECT_EQ(if_stmt.else_branch->opcode, "GOTOB");
  EXPECT_EQ(if_stmt.else_branch->target, "RETRY");
}

TEST(ParserControlFlowTest, ParsesGotoTargetKinds) {
  const auto result = gcode::parse("GOTOC N100\nGOTO 200\nGOTOF $DEST\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 3u);

  ASSERT_TRUE(result.program.lines[0].goto_statement.has_value());
  EXPECT_EQ(result.program.lines[0].goto_statement->target_kind, "line_number");
  EXPECT_EQ(result.program.lines[0].goto_statement->target, "N100");

  ASSERT_TRUE(result.program.lines[1].goto_statement.has_value());
  EXPECT_EQ(result.program.lines[1].goto_statement->target_kind, "number");
  EXPECT_EQ(result.program.lines[1].goto_statement->target, "200");

  ASSERT_TRUE(result.program.lines[2].goto_statement.has_value());
  EXPECT_EQ(result.program.lines[2].goto_statement->target_kind,
            "system_variable");
  EXPECT_EQ(result.program.lines[2].goto_statement->target, "$DEST");
}

TEST(ParserControlFlowTest, ParsesLineNumberAtBlockStart) {
  const auto result = gcode::parse("N100 G1 X1\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_TRUE(result.program.lines[0].line_number.has_value());
  EXPECT_EQ(result.program.lines[0].line_number->value, 100);
}

TEST(ParserControlFlowTest, ReportsMisplacedOrInvalidNAddressWord) {
  {
    const auto result = gcode::parse("G1 N100 X1\n");
    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_NE(result.diagnostics[0].message.find("block start"),
              std::string::npos);
  }
  {
    const auto result = gcode::parse("G1 N-1 X1\n");
    ASSERT_FALSE(result.diagnostics.empty());
    EXPECT_NE(result.diagnostics[0].message.find("syntax error"),
              std::string::npos);
  }
}

TEST(ParserControlFlowTest, WarnsOnDuplicateLineNumbers) {
  const auto result = gcode::parse("N100 G1 X1\nN100 G1 X2\nGOTO N100\n");
  ASSERT_EQ(result.diagnostics.size(), 1u);
  EXPECT_EQ(result.diagnostics[0].severity,
            gcode::Diagnostic::Severity::Warning);
  EXPECT_NE(result.diagnostics[0].message.find("duplicate N-address"),
            std::string::npos);
}

TEST(ParserControlFlowTest, ParsesStructuredControlFlowStatements) {
  const auto result = gcode::parse(
      "IF R1 == 1\nELSE\nENDIF\nWHILE R1 < 10\nENDWHILE\nFOR R2 = 0 TO 3\n"
      "ENDFOR\nREPEAT\nUNTIL R2 >= 3\nLOOP\nENDLOOP\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 11u);

  EXPECT_TRUE(result.program.lines[0].if_block_start_statement.has_value());
  EXPECT_TRUE(result.program.lines[1].else_statement.has_value());
  EXPECT_TRUE(result.program.lines[2].endif_statement.has_value());
  EXPECT_TRUE(result.program.lines[3].while_statement.has_value());
  EXPECT_TRUE(result.program.lines[4].endwhile_statement.has_value());
  EXPECT_TRUE(result.program.lines[5].for_statement.has_value());
  EXPECT_TRUE(result.program.lines[6].endfor_statement.has_value());
  EXPECT_TRUE(result.program.lines[7].repeat_statement.has_value());
  EXPECT_TRUE(result.program.lines[8].until_statement.has_value());
  EXPECT_TRUE(result.program.lines[9].loop_statement.has_value());
  EXPECT_TRUE(result.program.lines[10].endloop_statement.has_value());
}

TEST(ParserControlFlowTest, ParsesStructuredIfWithAndParenthesizedConditions) {
  const auto result = gcode::parse(
      "IF (R1 == 1) AND (R2 > 10)\nG1 Z-5 F10\nELSE\nG0 X0 Y5\nENDIF\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 5u);
  ASSERT_TRUE(result.program.lines[0].if_block_start_statement.has_value());

  const auto &condition =
      result.program.lines[0].if_block_start_statement->condition;
  EXPECT_TRUE(condition.has_logical_and);
  ASSERT_EQ(condition.and_terms_raw.size(), 2u);
  EXPECT_EQ(condition.and_terms_raw[0], "(R1 == 1)");
  EXPECT_EQ(condition.and_terms_raw[1], "(R2 > 10)");
  EXPECT_EQ(condition.raw_text, "(R1 == 1)AND(R2 > 10)");
}

TEST(ParserSyntaxBaselineTest, ParsesBlockDeleteWithSkipLevel) {
  const auto result = gcode::parse("/1 N100 G1 X1\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);

  const auto &line = result.program.lines[0];
  EXPECT_TRUE(line.block_delete);
  ASSERT_TRUE(line.block_delete_level.has_value());
  EXPECT_EQ(*line.block_delete_level, 1);
  ASSERT_TRUE(line.line_number.has_value());
  EXPECT_EQ(line.line_number->value, 100);
}

TEST(ParserSyntaxBaselineTest, ReportsInvalidSkipLevelRange) {
  const auto result = gcode::parse("/10 N20 G1 X1\n");
  ASSERT_EQ(result.diagnostics.size(), 1u);
  EXPECT_EQ(result.diagnostics[0].severity, gcode::Diagnostic::Severity::Error);
  EXPECT_NE(result.diagnostics[0].message.find("skip level"),
            std::string::npos);
}

TEST(ParserSyntaxBaselineTest, ReportsBlockLengthOverflow) {
  std::string input = "N10 ";
  input.append(510, 'X');
  input.push_back('\n');

  const auto result = gcode::parse(input);
  ASSERT_EQ(result.diagnostics.size(), 1u);
  EXPECT_EQ(result.diagnostics[0].severity, gcode::Diagnostic::Severity::Error);
  EXPECT_NE(result.diagnostics[0].message.find("block length exceeds 512"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics[0].location.line, 1);
}

TEST(ParserSyntaxBaselineTest, RequiresEqualForMultiLetterAddressValues) {
  const auto result = gcode::parse("G1 AP90 RP=10\n");
  ASSERT_EQ(result.diagnostics.size(), 1u);
  EXPECT_EQ(result.diagnostics[0].severity, gcode::Diagnostic::Severity::Error);
  EXPECT_NE(result.diagnostics[0].message.find("use '='"), std::string::npos);
}

TEST(ParserSyntaxBaselineTest, AcceptsNumericExtensionWithEqual) {
  const auto result = gcode::parse("X1=10\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 1u);
  ASSERT_TRUE(
      std::holds_alternative<gcode::Word>(result.program.lines[0].items[0]));
  const auto &word = std::get<gcode::Word>(result.program.lines[0].items[0]);
  EXPECT_EQ(word.head, "X1");
  ASSERT_TRUE(word.value.has_value());
  EXPECT_EQ(*word.value, "10");
  EXPECT_TRUE(word.has_equal);
}

TEST(ParserSyntaxBaselineTest, ParsesMultiLineBlockComment) {
  const auto result = gcode::parse("(* line1\nline2 *)\nG1 X10\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 2u);
  ASSERT_EQ(result.program.lines[0].items.size(), 1u);
  ASSERT_TRUE(
      std::holds_alternative<gcode::Comment>(result.program.lines[0].items[0]));
  const auto &comment =
      std::get<gcode::Comment>(result.program.lines[0].items[0]);
  EXPECT_EQ(comment.text, "(* line1\nline2 *)");
}

TEST(ParserSyntaxBaselineTest, ReportsUnclosedBlockComment) {
  const auto result = gcode::parse("(* line1\nline2\nG1 X10\n");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_NE(result.diagnostics[0].message.find("malformed comments"),
            std::string::npos);
}

TEST(ParserSyntaxBaselineTest, ReportsDoubleSlashCommentWhenModeDisabled) {
  const auto result = gcode::parse("G1 X10 // trailing\n");
  ASSERT_EQ(result.diagnostics.size(), 1u);
  EXPECT_NE(result.diagnostics[0].message.find("double-slash comments require"),
            std::string::npos);
}

TEST(ParserSyntaxBaselineTest, AcceptsDoubleSlashCommentWhenModeEnabled) {
  gcode::ParseOptions options;
  options.enable_double_slash_comments = true;
  const auto result = gcode::parse("G1 X10 // trailing\n", options);
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 3u);
  ASSERT_TRUE(
      std::holds_alternative<gcode::Comment>(result.program.lines[0].items[2]));
  const auto &comment =
      std::get<gcode::Comment>(result.program.lines[0].items[2]);
  EXPECT_EQ(comment.text, "// trailing");
}

TEST(ParserSyntaxBaselineTest, ParsesMCodeBaselineForms) {
  const auto result = gcode::parse("M3\nM2=3\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 2u);
}

TEST(ParserSyntaxBaselineTest, ParsesSubprogramCallAndReturnSurfaceSyntax) {
  const auto result = gcode::parse("L1000\nL2000 P3\nP=2 L3000\nRET\nM17\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 5u);
}

TEST(ParserSyntaxBaselineTest, ParsesQuotedSubprogramCallTarget) {
  const auto result = gcode::parse("\"DIR/SPF1000\"\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 1u);
  ASSERT_TRUE(
      std::holds_alternative<gcode::Word>(result.program.lines[0].items[0]));
  const auto &word = std::get<gcode::Word>(result.program.lines[0].items[0]);
  EXPECT_TRUE(word.quoted);
  EXPECT_EQ(word.text, "DIR/SPF1000");
}

TEST(ParserSyntaxBaselineTest, ParsesQuotedSubprogramCallWithTrailingRepeat) {
  const auto result = gcode::parse("\"DIR/SPF1000\" P3\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 2u);
}

TEST(ParserSyntaxBaselineTest, ParsesQuotedSubprogramCallWithLeadingRepeat) {
  const auto result = gcode::parse("P=2 \"DIR/SPF1000\"\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 2u);
}

TEST(ParserSyntaxBaselineTest, ParsesAlphabeticSubprogramCallTarget) {
  const auto result = gcode::parse("ALIAS\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 1u);
  ASSERT_TRUE(
      std::holds_alternative<gcode::Word>(result.program.lines[0].items[0]));
  const auto &word = std::get<gcode::Word>(result.program.lines[0].items[0]);
  EXPECT_FALSE(word.quoted);
  EXPECT_EQ(word.text, "ALIAS");
}

TEST(ParserSyntaxBaselineTest, ParsesLowercaseAlphabeticSubprogramCallTarget) {
  const auto result = gcode::parse("main\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 1u);
  ASSERT_TRUE(
      std::holds_alternative<gcode::Word>(result.program.lines[0].items[0]));
  const auto &word = std::get<gcode::Word>(result.program.lines[0].items[0]);
  EXPECT_FALSE(word.quoted);
  EXPECT_EQ(word.text, "main");
}

TEST(ParserSyntaxBaselineTest, ParsesMixedCaseAlphabeticSubprogramCallTarget) {
  const auto result = gcode::parse("mAiN\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 1u);
  ASSERT_TRUE(
      std::holds_alternative<gcode::Word>(result.program.lines[0].items[0]));
  const auto &word = std::get<gcode::Word>(result.program.lines[0].items[0]);
  EXPECT_FALSE(word.quoted);
  EXPECT_EQ(word.text, "mAiN");
}

TEST(ParserSyntaxBaselineTest,
     ParsesLowercaseAlphabeticSubprogramCallWithTrailingRepeat) {
  const auto result = gcode::parse("main P3\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 2u);
}

TEST(ParserSyntaxBaselineTest,
     ParsesMixedCaseAlphabeticSubprogramCallWithTrailingRepeat) {
  const auto result = gcode::parse("mAiN P3\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 2u);
}

TEST(ParserSyntaxBaselineTest,
     ParsesLowercaseAlphabeticSubprogramCallWithLeadingRepeat) {
  const auto result = gcode::parse("P=2 main\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 2u);
}

TEST(ParserSyntaxBaselineTest,
     ParsesMixedCaseAlphabeticSubprogramCallWithLeadingRepeat) {
  const auto result = gcode::parse("P=2 mAiN\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 2u);
}

TEST(ParserSyntaxBaselineTest, ParsesProcDeclarationSurfaceSyntax) {
  const auto result = gcode::parse("PROC MAIN\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 2u);
  ASSERT_TRUE(
      std::holds_alternative<gcode::Word>(result.program.lines[0].items[0]));
  ASSERT_TRUE(
      std::holds_alternative<gcode::Word>(result.program.lines[0].items[1]));
  const auto &kw = std::get<gcode::Word>(result.program.lines[0].items[0]);
  const auto &name = std::get<gcode::Word>(result.program.lines[0].items[1]);
  EXPECT_EQ(kw.head, "PROC");
  EXPECT_EQ(name.text, "MAIN");
}

TEST(ParserSyntaxBaselineTest, ParsesQuotedProcDeclarationSurfaceSyntax) {
  const auto result = gcode::parse("PROC \"DIR/SPF1000\"\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 2u);
  ASSERT_TRUE(
      std::holds_alternative<gcode::Word>(result.program.lines[0].items[0]));
  ASSERT_TRUE(
      std::holds_alternative<gcode::Word>(result.program.lines[0].items[1]));
  const auto &kw = std::get<gcode::Word>(result.program.lines[0].items[0]);
  const auto &name = std::get<gcode::Word>(result.program.lines[0].items[1]);
  EXPECT_EQ(kw.head, "PROC");
  EXPECT_TRUE(name.quoted);
  EXPECT_EQ(name.text, "DIR/SPF1000");
}

TEST(ParserSyntaxBaselineTest, ParsesLowercaseProcDeclarationSurfaceSyntax) {
  const auto result = gcode::parse("proc main\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 2u);
  ASSERT_TRUE(
      std::holds_alternative<gcode::Word>(result.program.lines[0].items[0]));
  ASSERT_TRUE(
      std::holds_alternative<gcode::Word>(result.program.lines[0].items[1]));
  const auto &kw = std::get<gcode::Word>(result.program.lines[0].items[0]);
  const auto &name = std::get<gcode::Word>(result.program.lines[0].items[1]);
  EXPECT_EQ(kw.head, "PROC");
  EXPECT_EQ(name.text, "main");
}

TEST(ParserSyntaxBaselineTest, ParsesMixedCaseProcDeclarationSurfaceSyntax) {
  const auto result = gcode::parse("PrOc Mix\n");
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.program.lines.size(), 1u);
  ASSERT_EQ(result.program.lines[0].items.size(), 2u);
  ASSERT_TRUE(
      std::holds_alternative<gcode::Word>(result.program.lines[0].items[0]));
  ASSERT_TRUE(
      std::holds_alternative<gcode::Word>(result.program.lines[0].items[1]));
  const auto &kw = std::get<gcode::Word>(result.program.lines[0].items[0]);
  const auto &name = std::get<gcode::Word>(result.program.lines[0].items[1]);
  EXPECT_EQ(kw.head, "PROC");
  EXPECT_EQ(name.text, "Mix");
}

TEST(ParserSyntaxBaselineTest, ReportsMalformedProcEqualForm) {
  const auto result = gcode::parse("PROC=MAIN\n");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_NE(result.diagnostics.back().message.find("expected PROC <name>"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.line, 1);
  EXPECT_EQ(result.diagnostics.back().location.column, 1);
}

TEST(ParserSyntaxBaselineTest, ReportsMalformedProcMissingTarget) {
  const auto result = gcode::parse("PROC\n");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_NE(result.diagnostics.back().message.find("expected PROC <name>"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.line, 1);
  EXPECT_EQ(result.diagnostics.back().location.column, 1);
}

TEST(ParserSyntaxBaselineTest, ReportsMalformedProcInvalidTargetWord) {
  const auto result = gcode::parse("PROC G1\n");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_NE(result.diagnostics.back().message.find("expected PROC <name>"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.line, 1);
  EXPECT_EQ(result.diagnostics.back().location.column, 6);
}

TEST(ParserSyntaxBaselineTest, ReportsMalformedProcWithExtraWord) {
  const auto result = gcode::parse("PROC MAIN P2\n");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_NE(result.diagnostics.back().message.find("expected PROC <name>"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.line, 1);
  EXPECT_EQ(result.diagnostics.back().location.column, 11);
}

TEST(ParserSyntaxBaselineTest, ReportsMalformedLowercaseProcWithExtraWord) {
  const auto result = gcode::parse("proc main p2\n");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_NE(result.diagnostics.back().message.find("expected PROC <name>"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.line, 1);
  EXPECT_EQ(result.diagnostics.back().location.column, 11);
}

TEST(ParserSyntaxBaselineTest, ReportsMalformedLowercaseProcEqualForm) {
  const auto result = gcode::parse("proc=main\n");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_NE(result.diagnostics.back().message.find("expected PROC <name>"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.line, 1);
  EXPECT_EQ(result.diagnostics.back().location.column, 1);
}

TEST(ParserSyntaxBaselineTest, ReportsMalformedMixedCaseProcEqualForm) {
  const auto result = gcode::parse("PrOc=main\n");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_NE(result.diagnostics.back().message.find("expected PROC <name>"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.line, 1);
  EXPECT_EQ(result.diagnostics.back().location.column, 1);
}

TEST(ParserSyntaxBaselineTest, ReportsSyntaxErrorForProcQuotedEqualForm) {
  const auto result = gcode::parse("PROC=\"DIR/SPF1000\"\n");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_NE(result.diagnostics.back().message.find("syntax error"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.line, 1);
  EXPECT_EQ(result.diagnostics.back().location.column, 6);
}

TEST(ParserSyntaxBaselineTest,
     ReportsSyntaxErrorForLowercaseProcQuotedEqualForm) {
  const auto result = gcode::parse("proc=\"DIR/SPF1000\"\n");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_NE(result.diagnostics.back().message.find("syntax error"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.line, 1);
  EXPECT_EQ(result.diagnostics.back().location.column, 6);
}

TEST(ParserSyntaxBaselineTest,
     ReportsSyntaxErrorForMixedCaseProcQuotedEqualForm) {
  const auto result = gcode::parse("PrOc=\"DIR/SPF1000\"\n");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_NE(result.diagnostics.back().message.find("syntax error"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics.back().location.line, 1);
  EXPECT_EQ(result.diagnostics.back().location.column, 6);
}

TEST(ParserSyntaxBaselineTest, M98CallRequiresIsoCompatibilityMode) {
  {
    const auto result = gcode::parse("M98 P1000\n");
    ASSERT_EQ(result.diagnostics.size(), 1u);
    EXPECT_NE(result.diagnostics[0].message.find("ISO compatibility"),
              std::string::npos);
  }
  {
    gcode::ParseOptions options;
    options.enable_iso_m98_calls = true;
    const auto result = gcode::parse("M98 P1000\n", options);
    EXPECT_TRUE(result.diagnostics.empty());
  }
}

TEST(ParserSyntaxBaselineTest, ReportsMCodeValueOutOfRange) {
  const auto result = gcode::parse("M2147483648\n");
  ASSERT_EQ(result.diagnostics.size(), 1u);
  EXPECT_NE(result.diagnostics[0].message.find("out of range"),
            std::string::npos);
}

TEST(ParserSyntaxBaselineTest, ReportsExtendedMAddressForProhibitedFunctions) {
  const auto result = gcode::parse("M2=30\n");
  ASSERT_EQ(result.diagnostics.size(), 1u);
  EXPECT_NE(result.diagnostics[0].message.find("not allowed"),
            std::string::npos);
}

TEST(ParserToolSelectorTest, AcceptsNumericFormsWhenToolManagementDisabled) {
  gcode::ParseOptions options;
  options.tool_management = false;
  const auto result = gcode::parse("T12\nT=7\nT1=99\n", options);
  EXPECT_TRUE(result.diagnostics.empty());
}

TEST(ParserToolSelectorTest,
     RejectsNonNumericOrMissingEqualFormsWhenToolManagementDisabled) {
  {
    gcode::ParseOptions options;
    options.tool_management = false;
    const auto result = gcode::parse("T=DRILL\n", options);
    ASSERT_EQ(result.diagnostics.size(), 1u);
    EXPECT_NE(result.diagnostics[0].message.find("numeric forms"),
              std::string::npos);
  }
  {
    gcode::ParseOptions options;
    options.tool_management = false;
    const auto result = gcode::parse("T1=DRILL\n", options);
    ASSERT_EQ(result.diagnostics.size(), 1u);
    EXPECT_NE(result.diagnostics[0].message.find("numeric forms"),
              std::string::npos);
  }
}

TEST(ParserToolSelectorTest,
     AcceptsEqualFormsWhenToolManagementEnabledIncludingIndexed) {
  gcode::ParseOptions options;
  options.tool_management = true;
  const auto result = gcode::parse("T=DRILL_10\nT1=LOC_A5\n", options);
  EXPECT_TRUE(result.diagnostics.empty());
}

TEST(ParserToolSelectorTest,
     RejectsLegacyNumericShortcutWhenToolManagementEnabled) {
  gcode::ParseOptions options;
  options.tool_management = true;
  const auto result = gcode::parse("T12\n", options);
  ASSERT_EQ(result.diagnostics.size(), 1u);
  EXPECT_NE(result.diagnostics[0].message.find("requires '=' forms"),
            std::string::npos);
}

} // namespace
