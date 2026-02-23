#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "gtest/gtest.h"

#include "messages.h"
#include "messages_json.h"

namespace {

std::string readFile(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::in);
  std::stringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

TEST(MessagesJsonTest, RoundTripPreservesResult) {
  const std::string input = "N1 G1 X10 Y20 F100\nN2 G1 G2 X30\n";
  gcode::LowerOptions options;
  options.filename = "roundtrip.ngc";
  const auto result = gcode::parseAndLower(input, options);

  const auto json = gcode::toJsonString(result);
  const auto roundtrip = gcode::fromJsonString(json);
  EXPECT_EQ(gcode::toJsonString(roundtrip), json);
}

TEST(MessagesJsonTest, GoldenMessageOutput) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto testdata = source_dir / "testdata" / "messages";
  const auto input_path = testdata / "lowering_failfast.ngc";
  const auto golden_path = testdata / "lowering_failfast.golden.json";

  gcode::LowerOptions options;
  options.filename = input_path.filename().string();
  const auto result = gcode::parseAndLower(readFile(input_path), options);

  const auto actual = nlohmann::json::parse(gcode::toJsonString(result));
  const auto expected = nlohmann::json::parse(readFile(golden_path));
  EXPECT_EQ(actual, expected);
}

TEST(MessagesJsonTest, InvalidJsonReturnsDiagnostic) {
  const auto result = gcode::fromJsonString("{not-json");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_EQ(result.diagnostics[0].severity, gcode::Diagnostic::Severity::Error);
}

} // namespace
