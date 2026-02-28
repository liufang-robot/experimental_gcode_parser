#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "gtest/gtest.h"

#include "packet.h"
#include "packet_json.h"

namespace {

bool closeEnough(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-9; }

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

TEST(PacketTest, ConvertsAilMotionInstructionsToPackets) {
  const std::string input =
      "N10 G1 X1 Y2 F3\nN20 G2 X4 Y5 I6 J7 F8\nN30 G4 S30\n";
  gcode::LowerOptions options;
  options.filename = "sample.ngc";
  const auto result = gcode::parseLowerAndPacketize(input, options);

  ASSERT_TRUE(result.rejected_lines.empty());
  ASSERT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.packets.size(), 3u);

  EXPECT_EQ(result.packets[0].packet_id, 1u);
  EXPECT_EQ(result.packets[1].packet_id, 2u);
  EXPECT_EQ(result.packets[2].packet_id, 3u);

  EXPECT_EQ(result.packets[0].type, gcode::PacketType::LinearMove);
  EXPECT_EQ(result.packets[1].type, gcode::PacketType::ArcMove);
  EXPECT_EQ(result.packets[2].type, gcode::PacketType::Dwell);

  ASSERT_TRUE(std::holds_alternative<gcode::MotionLinearPayload>(
      result.packets[0].payload));
  const auto &p0 =
      std::get<gcode::MotionLinearPayload>(result.packets[0].payload);
  ASSERT_TRUE(p0.target_pose.x.has_value());
  EXPECT_TRUE(closeEnough(*p0.target_pose.x, 1.0));
  ASSERT_TRUE(p0.feed.has_value());
  EXPECT_TRUE(closeEnough(*p0.feed, 3.0));

  ASSERT_TRUE(std::holds_alternative<gcode::MotionArcPayload>(
      result.packets[1].payload));
  const auto &p1 = std::get<gcode::MotionArcPayload>(result.packets[1].payload);
  EXPECT_TRUE(p1.clockwise);
  ASSERT_TRUE(p1.arc.i.has_value());
  EXPECT_TRUE(closeEnough(*p1.arc.i, 6.0));

  ASSERT_TRUE(
      std::holds_alternative<gcode::DwellPayload>(result.packets[2].payload));
  const auto &p2 = std::get<gcode::DwellPayload>(result.packets[2].payload);
  EXPECT_EQ(p2.dwell_mode, gcode::DwellMode::Revolutions);
  EXPECT_TRUE(closeEnough(p2.dwell_value, 30.0));
}

TEST(PacketTest, SkipsNonMotionInstructionsWithWarning) {
  const auto result =
      gcode::parseLowerAndPacketize("R1 = $P_ACT_X + 2\nG1 X20\n");

  ASSERT_EQ(result.packets.size(), 1u);
  EXPECT_EQ(result.packets[0].source.line, 2);

  ASSERT_EQ(result.diagnostics.size(), 1u);
  EXPECT_EQ(result.diagnostics[0].severity,
            gcode::Diagnostic::Severity::Warning);
  EXPECT_NE(result.diagnostics[0].message.find("packetization skipped"),
            std::string::npos);
  EXPECT_EQ(result.diagnostics[0].location.line, 1);
}

TEST(PacketTest, JsonContainsStableSchema) {
  const auto result = gcode::parseLowerAndPacketize("G1 X10\nG4 F3\n");
  const auto json = nlohmann::json::parse(gcode::packetToJsonString(result));

  EXPECT_EQ(json.value("schema_version", 0), 1);
  ASSERT_TRUE(json.contains("packets"));
  ASSERT_EQ(json["packets"].size(), 2u);
  EXPECT_EQ(json["packets"][0]["type"], "linear_move");
  EXPECT_EQ(json["packets"][1]["type"], "dwell");
}

TEST(PacketTest, GoldenPacketOutput) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto testdata = source_dir / "testdata" / "packets";
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

    const auto result =
        gcode::parseLowerAndPacketize(readFile(input_path), options);
    const auto actual =
        nlohmann::json::parse(gcode::packetToJsonString(result));
    const auto expected = nlohmann::json::parse(readFile(golden_path));
    EXPECT_EQ(actual, expected)
        << "fixture mismatch: " << input_path.filename();
  }
}

} // namespace
