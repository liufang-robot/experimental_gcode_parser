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

} // namespace
