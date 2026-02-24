#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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

std::vector<std::filesystem::path>
collectFixtureInputs(const std::filesystem::path &dir) {
  std::vector<std::filesystem::path> inputs;
  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    if (entry.path().extension() == ".ngc") {
      inputs.push_back(entry.path());
    }
  }
  std::sort(inputs.begin(), inputs.end());
  return inputs;
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
  // Fixture naming convention:
  // - input:  <name>.ngc
  // - golden: <name>.golden.json
  // Special case:
  // - names prefixed with "nofilename_" run with filename unset.
  const auto inputs = collectFixtureInputs(testdata);
  ASSERT_FALSE(inputs.empty());
  for (const auto &input_path : inputs) {
    const auto stem = input_path.stem().string();
    const auto golden_path = testdata / (stem + ".golden.json");
    ASSERT_TRUE(std::filesystem::exists(golden_path))
        << "missing golden for " << input_path.filename();

    gcode::LowerOptions options;
    if (stem.rfind("nofilename_", 0) != 0) {
      options.filename = input_path.filename().string();
    }
    const auto result = gcode::parseAndLower(readFile(input_path), options);

    const auto actual = nlohmann::json::parse(gcode::toJsonString(result));
    const auto expected = nlohmann::json::parse(readFile(golden_path));
    EXPECT_EQ(actual, expected)
        << "fixture mismatch: " << input_path.filename();
  }
}

TEST(MessagesJsonTest, InvalidJsonReturnsDiagnostic) {
  const auto result = gcode::fromJsonString("{not-json");
  ASSERT_FALSE(result.diagnostics.empty());
  EXPECT_EQ(result.diagnostics[0].severity, gcode::Diagnostic::Severity::Error);
}

TEST(MessagesJsonTest, RoundTripWithG2G3PreservesResult) {
  const std::string input = "N1 G2 X10 Y20 I1 J2 K3 R4 F5\nN2 G3 X30 Y40 I6 J7 "
                            "K8 CR=9 F10\n";
  gcode::LowerOptions options;
  options.filename = "arc_roundtrip.ngc";
  const auto result = gcode::parseAndLower(input, options);

  const auto json = gcode::toJsonString(result);
  const auto roundtrip = gcode::fromJsonString(json);
  EXPECT_EQ(gcode::toJsonString(roundtrip), json);
}

TEST(MessagesJsonTest, RoundTripWithG4PreservesResult) {
  const std::string input = "N1 G4 F3\nN2 G4 S30\n";
  gcode::LowerOptions options;
  options.filename = "dwell_roundtrip.ngc";
  const auto result = gcode::parseAndLower(input, options);

  const auto json = gcode::toJsonString(result);
  const auto roundtrip = gcode::fromJsonString(json);
  EXPECT_EQ(gcode::toJsonString(roundtrip), json);
}

} // namespace
