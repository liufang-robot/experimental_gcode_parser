#include <cmath>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "lowering_family_g4.h"

namespace {

bool closeEnough(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-9; }

gcode::Word makeWord(const std::string &head, const std::string &value,
                     int column) {
  gcode::Word word;
  word.head = head;
  word.value = value;
  word.location = {7, column};
  return word;
}

TEST(G4LowererTest, MotionCodeIsG4) {
  const gcode::G4Lowerer lowerer;
  EXPECT_EQ(lowerer.motionCode(), 4);
}

TEST(G4LowererTest, LowersSecondsFromFWord) {
  gcode::Line line;
  line.line_index = 7;
  line.items.push_back(makeWord("G", "4", 1));
  line.items.push_back(makeWord("F", "3.5", 4));

  const gcode::G4Lowerer lowerer;
  std::vector<gcode::Diagnostic> diagnostics;
  const gcode::ParsedMessage lowered =
      lowerer.lower(line, gcode::LowerOptions{}, &diagnostics);

  ASSERT_TRUE(std::holds_alternative<gcode::G4Message>(lowered));
  const auto &msg = std::get<gcode::G4Message>(lowered);
  EXPECT_EQ(msg.dwell_mode, gcode::DwellMode::Seconds);
  EXPECT_TRUE(closeEnough(msg.dwell_value, 3.5));
  EXPECT_TRUE(diagnostics.empty());
}

TEST(G4LowererTest, LowersRevolutionsFromSWord) {
  gcode::Line line;
  line.line_index = 7;
  line.items.push_back(makeWord("G", "4", 1));
  line.items.push_back(makeWord("S", "30", 4));

  const gcode::G4Lowerer lowerer;
  std::vector<gcode::Diagnostic> diagnostics;
  const gcode::ParsedMessage lowered =
      lowerer.lower(line, gcode::LowerOptions{}, &diagnostics);

  ASSERT_TRUE(std::holds_alternative<gcode::G4Message>(lowered));
  const auto &msg = std::get<gcode::G4Message>(lowered);
  EXPECT_EQ(msg.dwell_mode, gcode::DwellMode::Revolutions);
  EXPECT_TRUE(closeEnough(msg.dwell_value, 30.0));
  EXPECT_TRUE(diagnostics.empty());
}

} // namespace
