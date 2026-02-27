#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "messages.h"

namespace {

bool closeEnough(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-9; }

std::string makeProgram(size_t line_count) {
  std::string text;
  text.reserve(line_count * 18);
  for (size_t i = 0; i < line_count; ++i) {
    text += "G1 X1 F2\n";
  }
  return text;
}

TEST(StreamingTest, StreamsTenThousandLinesWithoutAccumulatingMessageVector) {
  const std::string input = makeProgram(10000);
  size_t message_count = 0;
  size_t diagnostic_count = 0;

  gcode::StreamCallbacks callbacks;
  callbacks.on_message = [&](const gcode::ParsedMessage &) { ++message_count; };
  callbacks.on_diagnostic = [&](const gcode::Diagnostic &) {
    ++diagnostic_count;
  };

  const bool completed = gcode::parseAndLowerStream(input, {}, callbacks);
  EXPECT_TRUE(completed);
  EXPECT_EQ(message_count, 10000u);
  EXPECT_EQ(diagnostic_count, 0u);
}

TEST(StreamingTest, CancelHookStopsEarly) {
  const std::string input = makeProgram(10000);
  size_t message_count = 0;

  gcode::StreamCallbacks callbacks;
  callbacks.on_message = [&](const gcode::ParsedMessage &) { ++message_count; };

  gcode::StreamOptions options;
  options.should_cancel = [&]() { return message_count >= 10; };

  const bool completed =
      gcode::parseAndLowerStream(input, {}, callbacks, options);
  EXPECT_FALSE(completed);
  EXPECT_GE(message_count, 10u);
  EXPECT_LT(message_count, 10000u);
}

TEST(StreamingTest, StreamsRejectedLineOnError) {
  size_t rejected_count = 0;
  size_t message_count = 0;
  const std::string input = "G1 X10\nG4 F3 X20\nG1 X30\n";

  gcode::StreamCallbacks callbacks;
  callbacks.on_message = [&](const gcode::ParsedMessage &) { ++message_count; };
  callbacks.on_rejected_line =
      [&](const gcode::MessageResult::RejectedLine &rejected) {
        ++rejected_count;
        EXPECT_EQ(rejected.source.line, 2);
      };

  const bool completed = gcode::parseAndLowerStream(input, {}, callbacks);
  EXPECT_FALSE(completed);
  EXPECT_EQ(rejected_count, 1u);
  EXPECT_EQ(message_count, 1u);
}

TEST(StreamingTest, ParseAndLowerFileStreamWorks) {
  const auto temp_file =
      std::filesystem::temp_directory_path() / "gcode_stream_test.ngc";
  {
    std::ofstream out(temp_file, std::ios::out | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out << "N1 G4 F3\nN2 G4 S30\n";
  }

  size_t message_count = 0;
  gcode::StreamCallbacks callbacks;
  callbacks.on_message = [&](const gcode::ParsedMessage &) { ++message_count; };

  const bool completed =
      gcode::parseAndLowerFileStream(temp_file.string(), {}, callbacks);

  std::error_code ec;
  std::filesystem::remove(temp_file, ec);

  EXPECT_TRUE(completed);
  EXPECT_EQ(message_count, 2u);
}

TEST(StreamingTest, CallbackCanCaptureAndValidateDetailedMessages) {
  const std::string input = "N1 G4 F3\nN2 G1 X10 Y20 F30\nN3 G4 S40\n";
  std::vector<gcode::ParsedMessage> messages;

  gcode::StreamCallbacks callbacks;
  callbacks.on_message = [&](const gcode::ParsedMessage &message) {
    messages.push_back(message);
  };

  const bool completed = gcode::parseAndLowerStream(input, {}, callbacks);
  ASSERT_TRUE(completed);
  ASSERT_EQ(messages.size(), 3u);

  ASSERT_TRUE(std::holds_alternative<gcode::G4Message>(messages[0]));
  const auto &first = std::get<gcode::G4Message>(messages[0]);
  EXPECT_EQ(first.source.line, 1);
  EXPECT_EQ(first.dwell_mode, gcode::DwellMode::Seconds);
  EXPECT_TRUE(closeEnough(first.dwell_value, 3.0));

  ASSERT_TRUE(std::holds_alternative<gcode::G1Message>(messages[1]));
  const auto &second = std::get<gcode::G1Message>(messages[1]);
  EXPECT_EQ(second.source.line, 2);
  ASSERT_TRUE(second.target_pose.x.has_value());
  ASSERT_TRUE(second.target_pose.y.has_value());
  ASSERT_TRUE(second.feed.has_value());
  EXPECT_TRUE(closeEnough(*second.target_pose.x, 10.0));
  EXPECT_TRUE(closeEnough(*second.target_pose.y, 20.0));
  EXPECT_TRUE(closeEnough(*second.feed, 30.0));

  ASSERT_TRUE(std::holds_alternative<gcode::G4Message>(messages[2]));
  const auto &third = std::get<gcode::G4Message>(messages[2]);
  EXPECT_EQ(third.source.line, 3);
  EXPECT_EQ(third.dwell_mode, gcode::DwellMode::Revolutions);
  EXPECT_TRUE(closeEnough(third.dwell_value, 40.0));
}

} // namespace
