#include <cmath>

#include "gtest/gtest.h"

#include "messages.h"

namespace {

bool closeEnough(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-9; }

const gcode::G1Message *asG1(const gcode::ParsedMessage &msg) {
  if (!std::holds_alternative<gcode::G1Message>(msg)) {
    return nullptr;
  }
  return &std::get<gcode::G1Message>(msg);
}

TEST(MessagesTest, G1Extraction) {
  const std::string input = "N10 G1 X10 Y20 Z30 A40 B50 C60 F100\n";
  gcode::LowerOptions options;
  options.filename = "job.ngc";
  const auto result = gcode::parseAndLower(input, options);

  EXPECT_TRUE(result.diagnostics.empty());
  ASSERT_EQ(result.messages.size(), 1u);

  const auto *g1 = asG1(result.messages[0]);
  ASSERT_NE(g1, nullptr);
  ASSERT_TRUE(g1->source.filename.has_value());
  EXPECT_EQ(*g1->source.filename, "job.ngc");
  EXPECT_EQ(g1->source.line, 1);
  ASSERT_TRUE(g1->source.line_number.has_value());
  EXPECT_EQ(*g1->source.line_number, 10);

  ASSERT_TRUE(g1->target_pose.x.has_value());
  EXPECT_TRUE(closeEnough(*g1->target_pose.x, 10.0));
  ASSERT_TRUE(g1->target_pose.y.has_value());
  EXPECT_TRUE(closeEnough(*g1->target_pose.y, 20.0));
  ASSERT_TRUE(g1->target_pose.z.has_value());
  EXPECT_TRUE(closeEnough(*g1->target_pose.z, 30.0));
  ASSERT_TRUE(g1->target_pose.a.has_value());
  EXPECT_TRUE(closeEnough(*g1->target_pose.a, 40.0));
  ASSERT_TRUE(g1->target_pose.b.has_value());
  EXPECT_TRUE(closeEnough(*g1->target_pose.b, 50.0));
  ASSERT_TRUE(g1->target_pose.c.has_value());
  EXPECT_TRUE(closeEnough(*g1->target_pose.c, 60.0));
  ASSERT_TRUE(g1->feed.has_value());
  EXPECT_TRUE(closeEnough(*g1->feed, 100.0));
}

TEST(MessagesTest, LowercaseWordsEquivalent) {
  const std::string input = "n7 g1 x1.5 y2 z3 a4 b5 c6 f7\n";
  gcode::LowerOptions options;
  options.filename = "lower.ngc";
  const auto result = gcode::parseAndLower(input, options);

  EXPECT_TRUE(result.diagnostics.empty());
  EXPECT_TRUE(result.rejected_lines.empty());
  ASSERT_EQ(result.messages.size(), 1u);

  const auto *g1 = asG1(result.messages[0]);
  ASSERT_NE(g1, nullptr);
  ASSERT_TRUE(g1->source.filename.has_value());
  EXPECT_EQ(*g1->source.filename, "lower.ngc");
  EXPECT_EQ(g1->source.line, 1);
  ASSERT_TRUE(g1->source.line_number.has_value());
  EXPECT_EQ(*g1->source.line_number, 7);

  ASSERT_TRUE(g1->target_pose.x.has_value());
  EXPECT_TRUE(closeEnough(*g1->target_pose.x, 1.5));
  ASSERT_TRUE(g1->target_pose.y.has_value());
  EXPECT_TRUE(closeEnough(*g1->target_pose.y, 2.0));
  ASSERT_TRUE(g1->target_pose.z.has_value());
  EXPECT_TRUE(closeEnough(*g1->target_pose.z, 3.0));
  ASSERT_TRUE(g1->target_pose.a.has_value());
  EXPECT_TRUE(closeEnough(*g1->target_pose.a, 4.0));
  ASSERT_TRUE(g1->target_pose.b.has_value());
  EXPECT_TRUE(closeEnough(*g1->target_pose.b, 5.0));
  ASSERT_TRUE(g1->target_pose.c.has_value());
  EXPECT_TRUE(closeEnough(*g1->target_pose.c, 6.0));
  ASSERT_TRUE(g1->feed.has_value());
  EXPECT_TRUE(closeEnough(*g1->feed, 7.0));
}

TEST(MessagesTest, StopAtFirstError) {
  const std::string input = "G1 X10\nG1 G2 X10\nG1 X20\n";
  const auto result = gcode::parseAndLower(input);

  ASSERT_EQ(result.messages.size(), 1u);
  ASSERT_EQ(result.rejected_lines.size(), 1u);
  EXPECT_EQ(result.rejected_lines[0].source.line, 2);
  EXPECT_FALSE(result.rejected_lines[0].reasons.empty());

  const auto *first = asG1(result.messages[0]);
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(first->source.line, 1);
}

} // namespace
