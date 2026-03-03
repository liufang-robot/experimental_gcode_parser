#include <cmath>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "lowering_family_g0.h"

namespace {

bool closeEnough(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-9; }

gcode::Word makeWord(const std::string &head, const std::string &value,
                     int column) {
  gcode::Word word;
  word.head = head;
  word.value = value;
  word.location = {1, column};
  return word;
}

gcode::Line makeLine() {
  gcode::Line line;
  line.line_index = 3;
  line.line_number = gcode::LineNumber{42, {3, 1}};
  line.items.push_back(makeWord("G", "0", 5));
  line.items.push_back(makeWord("X", "10", 8));
  line.items.push_back(makeWord("Y", "20", 12));
  line.items.push_back(makeWord("F", "150", 16));
  return line;
}

TEST(G0LowererTest, MotionCodeIsG0) {
  const gcode::G0Lowerer lowerer;
  EXPECT_EQ(lowerer.motionCode(), 0);
}

TEST(G0LowererTest, LowersPoseAndFeedAndSourceInfo) {
  const gcode::G0Lowerer lowerer;
  const gcode::Line line = makeLine();

  gcode::LowerOptions options;
  options.filename = "job.ngc";
  std::vector<gcode::Diagnostic> diagnostics;

  const gcode::ParsedMessage lowered =
      lowerer.lower(line, options, &diagnostics);

  ASSERT_TRUE(std::holds_alternative<gcode::G0Message>(lowered));
  const auto &msg = std::get<gcode::G0Message>(lowered);

  ASSERT_TRUE(msg.source.filename.has_value());
  EXPECT_EQ(*msg.source.filename, "job.ngc");
  EXPECT_EQ(msg.source.line, 3);
  ASSERT_TRUE(msg.source.line_number.has_value());
  EXPECT_EQ(*msg.source.line_number, 42);

  ASSERT_TRUE(msg.target_pose.x.has_value());
  EXPECT_TRUE(closeEnough(*msg.target_pose.x, 10.0));
  ASSERT_TRUE(msg.target_pose.y.has_value());
  EXPECT_TRUE(closeEnough(*msg.target_pose.y, 20.0));
  ASSERT_TRUE(msg.feed.has_value());
  EXPECT_TRUE(closeEnough(*msg.feed, 150.0));
  EXPECT_EQ(msg.modal.group, gcode::ModalGroupId::Motion);
  EXPECT_EQ(msg.modal.code, "G0");
  EXPECT_TRUE(msg.modal.updates_state);
  EXPECT_TRUE(diagnostics.empty());
}

} // namespace
