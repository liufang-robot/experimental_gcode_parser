#include <cmath>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "lowering_family_g3.h"

namespace {

bool closeEnough(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-9; }

gcode::Word makeWord(const std::string &head, const std::string &value,
                     int column) {
  gcode::Word word;
  word.head = head;
  word.value = value;
  word.location = {5, column};
  return word;
}

TEST(G3LowererTest, MotionCodeIsG3) {
  const gcode::G3Lowerer lowerer;
  EXPECT_EQ(lowerer.motionCode(), 3);
}

TEST(G3LowererTest, LowersArcAndSupportsCREqualsAlias) {
  gcode::Line line;
  line.line_index = 5;
  line.items.push_back(makeWord("G", "3", 1));
  line.items.push_back(makeWord("X", "8", 4));
  line.items.push_back(makeWord("CR", "9", 8));

  const gcode::G3Lowerer lowerer;
  std::vector<gcode::Diagnostic> diagnostics;
  const gcode::ParsedMessage lowered =
      lowerer.lower(line, gcode::LowerOptions{}, &diagnostics);

  ASSERT_TRUE(std::holds_alternative<gcode::G3Message>(lowered));
  const auto &msg = std::get<gcode::G3Message>(lowered);
  ASSERT_TRUE(msg.target_pose.x.has_value());
  EXPECT_TRUE(closeEnough(*msg.target_pose.x, 8.0));
  ASSERT_TRUE(msg.arc.r.has_value());
  EXPECT_TRUE(closeEnough(*msg.arc.r, 9.0));
  EXPECT_TRUE(diagnostics.empty());
}

} // namespace
