#include <cmath>

#include "gtest/gtest.h"

#include "message_diff.h"
#include "messages.h"

namespace {

bool closeEnough(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-9; }

const gcode::G1Message *asG1(const gcode::ParsedMessage &msg) {
  if (!std::holds_alternative<gcode::G1Message>(msg)) {
    return nullptr;
  }
  return &std::get<gcode::G1Message>(msg);
}

TEST(MessageDiffTest, UpdateLineProducesUpdatedEntry) {
  const auto before = gcode::parseAndLower("N1 G1 X1\nN2 G1 X2\nN3 G1 X3\n");
  const auto after = gcode::parseAndLower("N1 G1 X1\nN2 G1 X22\nN3 G1 X3\n");

  const auto diff = gcode::diffMessagesByLine(before, after);
  EXPECT_TRUE(diff.added.empty());
  EXPECT_TRUE(diff.removed_lines.empty());
  ASSERT_EQ(diff.updated.size(), 1u);
  EXPECT_EQ(diff.updated[0].line, 2);

  const auto applied = gcode::applyMessageDiff(before.messages, diff);
  ASSERT_EQ(applied.size(), after.messages.size());
  const auto *updated = asG1(applied[1]);
  ASSERT_NE(updated, nullptr);
  ASSERT_TRUE(updated->target_pose.x.has_value());
  EXPECT_TRUE(closeEnough(*updated->target_pose.x, 22.0));
}

TEST(MessageDiffTest, InsertLineProducesAddedEntry) {
  const auto before = gcode::parseAndLower("N1 G1 X1\n\nN3 G1 X3\n");
  const auto after = gcode::parseAndLower("N1 G1 X1\nN2 G1 X2\nN3 G1 X3\n");

  const auto diff = gcode::diffMessagesByLine(before, after);
  ASSERT_EQ(diff.added.size(), 1u);
  EXPECT_EQ(diff.added[0].line, 2);
  EXPECT_TRUE(diff.updated.empty());
  EXPECT_TRUE(diff.removed_lines.empty());

  const auto applied = gcode::applyMessageDiff(before.messages, diff);
  ASSERT_EQ(applied.size(), after.messages.size());
  const auto *added = asG1(applied[1]);
  ASSERT_NE(added, nullptr);
  ASSERT_TRUE(added->target_pose.x.has_value());
  EXPECT_TRUE(closeEnough(*added->target_pose.x, 2.0));
}

TEST(MessageDiffTest, DeleteLineProducesRemovedEntry) {
  const auto before = gcode::parseAndLower("N1 G1 X1\nN2 G1 X2\nN3 G1 X3\n");
  const auto after = gcode::parseAndLower("N1 G1 X1\n; removed\nN3 G1 X3\n");

  const auto diff = gcode::diffMessagesByLine(before, after);
  EXPECT_TRUE(diff.added.empty());
  EXPECT_TRUE(diff.updated.empty());
  ASSERT_EQ(diff.removed_lines.size(), 1u);
  EXPECT_EQ(diff.removed_lines[0], 2);

  const auto applied = gcode::applyMessageDiff(before.messages, diff);
  ASSERT_EQ(applied.size(), after.messages.size());
  const auto *first = asG1(applied[0]);
  const auto *second = asG1(applied[1]);
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(first->source.line, 1);
  EXPECT_EQ(second->source.line, 3);
}

TEST(MessageDiffTest, G4LineUpdateIsTrackedAsUpdatedEntry) {
  const auto before = gcode::parseAndLower("N1 G4 F3\n");
  const auto after = gcode::parseAndLower("N1 G4 S30\n");

  const auto diff = gcode::diffMessagesByLine(before, after);
  EXPECT_TRUE(diff.added.empty());
  EXPECT_TRUE(diff.removed_lines.empty());
  ASSERT_EQ(diff.updated.size(), 1u);
  EXPECT_EQ(diff.updated[0].line, 1);
}

} // namespace
