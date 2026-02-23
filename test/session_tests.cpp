#include <cmath>

#include "gtest/gtest.h"

#include "messages.h"
#include "session.h"

namespace {

bool closeEnough(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-9; }

const gcode::G1Message *asG1(const gcode::ParsedMessage &msg) {
  if (!std::holds_alternative<gcode::G1Message>(msg)) {
    return nullptr;
  }
  return &std::get<gcode::G1Message>(msg);
}

TEST(SessionTest, EditAndResumeFromErrorLine) {
  const std::string input = "G1 X10\nG1 G2 X10\nG1 X20\n";
  gcode::ParseSession session(input);

  ASSERT_EQ(session.result().messages.size(), 1u);
  ASSERT_EQ(session.result().rejected_lines.size(), 1u);
  EXPECT_EQ(session.result().rejected_lines[0].source.line, 2);

  const auto update = session.applyLineEdit(2, "G1 X15");
  EXPECT_EQ(update.from_line, 2);
  EXPECT_TRUE(update.result.diagnostics.empty());
  EXPECT_TRUE(update.result.rejected_lines.empty());
  ASSERT_EQ(update.result.messages.size(), 3u);

  const auto *m1 = asG1(update.result.messages[0]);
  const auto *m2 = asG1(update.result.messages[1]);
  const auto *m3 = asG1(update.result.messages[2]);
  ASSERT_NE(m1, nullptr);
  ASSERT_NE(m2, nullptr);
  ASSERT_NE(m3, nullptr);
  EXPECT_EQ(m1->source.line, 1);
  EXPECT_EQ(m2->source.line, 2);
  EXPECT_EQ(m3->source.line, 3);
  ASSERT_TRUE(m1->target_pose.x.has_value());
  ASSERT_TRUE(m2->target_pose.x.has_value());
  ASSERT_TRUE(m3->target_pose.x.has_value());
  EXPECT_TRUE(closeEnough(*m1->target_pose.x, 10.0));
  EXPECT_TRUE(closeEnough(*m2->target_pose.x, 15.0));
  EXPECT_TRUE(closeEnough(*m3->target_pose.x, 20.0));
}

TEST(SessionTest, MessageOrderingIsDeterministicAfterEdit) {
  const std::string input = "N1 G1 X1\nN2 G1 X2\nN3 G1 X3\n";
  gcode::ParseSession session(input);

  const auto update = session.applyLineEdit(2, "N2 G1 X22");
  ASSERT_EQ(update.result.messages.size(), 3u);

  const auto *m1 = asG1(update.result.messages[0]);
  const auto *m2 = asG1(update.result.messages[1]);
  const auto *m3 = asG1(update.result.messages[2]);
  ASSERT_NE(m1, nullptr);
  ASSERT_NE(m2, nullptr);
  ASSERT_NE(m3, nullptr);
  EXPECT_EQ(m1->source.line, 1);
  EXPECT_EQ(m2->source.line, 2);
  EXPECT_EQ(m3->source.line, 3);
  ASSERT_TRUE(m2->target_pose.x.has_value());
  EXPECT_TRUE(closeEnough(*m2->target_pose.x, 22.0));
}

} // namespace
