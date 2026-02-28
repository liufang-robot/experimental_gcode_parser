#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "gtest/gtest.h"

#include "ail.h"
#include "ail_json.h"

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

TEST(AilLoweringGoldenTest, GoldenAilOutput) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto testdata = source_dir / "testdata" / "ail";
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

    const auto result = gcode::parseAndLowerAil(readFile(input_path), options);
    const auto actual = nlohmann::json::parse(gcode::ailToJsonString(result));
    const auto expected = nlohmann::json::parse(readFile(golden_path));
    EXPECT_EQ(actual, expected)
        << "fixture mismatch: " << input_path.filename();
  }
}

} // namespace
