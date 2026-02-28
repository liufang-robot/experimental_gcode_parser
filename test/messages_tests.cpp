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

const gcode::G2Message *asG2(const gcode::ParsedMessage &msg) {
  if (!std::holds_alternative<gcode::G2Message>(msg)) {
    return nullptr;
  }
  return &std::get<gcode::G2Message>(msg);
}

const gcode::G3Message *asG3(const gcode::ParsedMessage &msg) {
  if (!std::holds_alternative<gcode::G3Message>(msg)) {
    return nullptr;
  }
  return &std::get<gcode::G3Message>(msg);
}

const gcode::G4Message *asG4(const gcode::ParsedMessage &msg) {
  if (!std::holds_alternative<gcode::G4Message>(msg)) {
    return nullptr;
  }
  return &std::get<gcode::G4Message>(msg);
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
  EXPECT_EQ(g1->modal.group, gcode::ModalGroupId::Motion);
  EXPECT_EQ(g1->modal.code, "G1");
  EXPECT_TRUE(g1->modal.updates_state);
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
  EXPECT_EQ(g1->modal.group, gcode::ModalGroupId::Motion);
  EXPECT_EQ(g1->modal.code, "G1");
  EXPECT_TRUE(g1->modal.updates_state);
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

TEST(MessagesTest, G2G3Extraction) {
  const std::string input =
      "N20 G2 X10 Y20 I1 J2 K3 CR=40 F100\nN21 G3 X30 Y40 I4 J5 K6 R50 F200\n";
  gcode::LowerOptions options;
  options.filename = "arc.ngc";
  const auto result = gcode::parseAndLower(input, options);

  EXPECT_TRUE(result.diagnostics.empty());
  EXPECT_TRUE(result.rejected_lines.empty());
  ASSERT_EQ(result.messages.size(), 2u);

  const auto *g2 = asG2(result.messages[0]);
  ASSERT_NE(g2, nullptr);
  EXPECT_EQ(g2->source.line, 1);
  ASSERT_TRUE(g2->source.line_number.has_value());
  EXPECT_EQ(*g2->source.line_number, 20);
  ASSERT_TRUE(g2->target_pose.x.has_value());
  EXPECT_TRUE(closeEnough(*g2->target_pose.x, 10.0));
  ASSERT_TRUE(g2->target_pose.y.has_value());
  EXPECT_TRUE(closeEnough(*g2->target_pose.y, 20.0));
  ASSERT_TRUE(g2->arc.i.has_value());
  EXPECT_TRUE(closeEnough(*g2->arc.i, 1.0));
  ASSERT_TRUE(g2->arc.j.has_value());
  EXPECT_TRUE(closeEnough(*g2->arc.j, 2.0));
  ASSERT_TRUE(g2->arc.k.has_value());
  EXPECT_TRUE(closeEnough(*g2->arc.k, 3.0));
  ASSERT_TRUE(g2->arc.r.has_value());
  EXPECT_TRUE(closeEnough(*g2->arc.r, 40.0));
  ASSERT_TRUE(g2->feed.has_value());
  EXPECT_TRUE(closeEnough(*g2->feed, 100.0));
  EXPECT_EQ(g2->modal.group, gcode::ModalGroupId::Motion);
  EXPECT_EQ(g2->modal.code, "G2");
  EXPECT_TRUE(g2->modal.updates_state);

  const auto *g3 = asG3(result.messages[1]);
  ASSERT_NE(g3, nullptr);
  EXPECT_EQ(g3->source.line, 2);
  ASSERT_TRUE(g3->source.line_number.has_value());
  EXPECT_EQ(*g3->source.line_number, 21);
  ASSERT_TRUE(g3->target_pose.x.has_value());
  EXPECT_TRUE(closeEnough(*g3->target_pose.x, 30.0));
  ASSERT_TRUE(g3->target_pose.y.has_value());
  EXPECT_TRUE(closeEnough(*g3->target_pose.y, 40.0));
  ASSERT_TRUE(g3->arc.i.has_value());
  EXPECT_TRUE(closeEnough(*g3->arc.i, 4.0));
  ASSERT_TRUE(g3->arc.j.has_value());
  EXPECT_TRUE(closeEnough(*g3->arc.j, 5.0));
  ASSERT_TRUE(g3->arc.k.has_value());
  EXPECT_TRUE(closeEnough(*g3->arc.k, 6.0));
  ASSERT_TRUE(g3->arc.r.has_value());
  EXPECT_TRUE(closeEnough(*g3->arc.r, 50.0));
  ASSERT_TRUE(g3->feed.has_value());
  EXPECT_TRUE(closeEnough(*g3->feed, 200.0));
  EXPECT_EQ(g3->modal.group, gcode::ModalGroupId::Motion);
  EXPECT_EQ(g3->modal.code, "G3");
  EXPECT_TRUE(g3->modal.updates_state);
}

TEST(MessagesTest, ArcUnsupportedWordsEmitWarningsButKeepMessage) {
  const std::string input = "G2 AP=90 RP=10 AR=30 X10 Y20 F100\n";
  const auto result = gcode::parseAndLower(input);

  ASSERT_EQ(result.messages.size(), 1u);
  EXPECT_TRUE(result.rejected_lines.empty());

  const auto *g2 = asG2(result.messages[0]);
  ASSERT_NE(g2, nullptr);
  ASSERT_TRUE(g2->target_pose.x.has_value());
  ASSERT_TRUE(g2->target_pose.y.has_value());
  EXPECT_TRUE(closeEnough(*g2->target_pose.x, 10.0));
  EXPECT_TRUE(closeEnough(*g2->target_pose.y, 20.0));
  ASSERT_TRUE(g2->feed.has_value());
  EXPECT_TRUE(closeEnough(*g2->feed, 100.0));

  ASSERT_EQ(result.diagnostics.size(), 3u);
  for (const auto &diag : result.diagnostics) {
    EXPECT_EQ(diag.severity, gcode::Diagnostic::Severity::Warning);
    EXPECT_NE(diag.message.find("ignored unsupported arc word"),
              std::string::npos);
  }
}

TEST(MessagesTest, G4ExtractionSecondsAndRevolutions) {
  const std::string input = "N10 G1 X1 F200 S300\nN20 G4 F3\nN30 G4 S30\n";
  gcode::LowerOptions options;
  options.filename = "dwell.ngc";
  const auto result = gcode::parseAndLower(input, options);

  EXPECT_TRUE(result.diagnostics.empty());
  EXPECT_TRUE(result.rejected_lines.empty());
  ASSERT_EQ(result.messages.size(), 3u);

  const auto *g4_seconds = asG4(result.messages[1]);
  ASSERT_NE(g4_seconds, nullptr);
  EXPECT_EQ(g4_seconds->source.line, 2);
  ASSERT_TRUE(g4_seconds->source.line_number.has_value());
  EXPECT_EQ(*g4_seconds->source.line_number, 20);
  EXPECT_EQ(g4_seconds->dwell_mode, gcode::DwellMode::Seconds);
  EXPECT_TRUE(closeEnough(g4_seconds->dwell_value, 3.0));
  EXPECT_EQ(g4_seconds->modal.group, gcode::ModalGroupId::NonModal);
  EXPECT_EQ(g4_seconds->modal.code, "G4");
  EXPECT_FALSE(g4_seconds->modal.updates_state);

  const auto *g4_revs = asG4(result.messages[2]);
  ASSERT_NE(g4_revs, nullptr);
  EXPECT_EQ(g4_revs->source.line, 3);
  ASSERT_TRUE(g4_revs->source.line_number.has_value());
  EXPECT_EQ(*g4_revs->source.line_number, 30);
  EXPECT_EQ(g4_revs->dwell_mode, gcode::DwellMode::Revolutions);
  EXPECT_TRUE(closeEnough(g4_revs->dwell_value, 30.0));
  EXPECT_EQ(g4_revs->modal.group, gcode::ModalGroupId::NonModal);
  EXPECT_EQ(g4_revs->modal.code, "G4");
  EXPECT_FALSE(g4_revs->modal.updates_state);
}

TEST(MessagesTest, G4MustBeSeparateBlockAndFailFast) {
  const std::string input = "N1 G1 X10 F100\nN2 G4 F3 X20\nN3 G4 S30\n";
  const auto result = gcode::parseAndLower(input);

  ASSERT_EQ(result.messages.size(), 1u);
  ASSERT_EQ(result.rejected_lines.size(), 1u);
  EXPECT_EQ(result.rejected_lines[0].source.line, 2);
  ASSERT_FALSE(result.rejected_lines[0].reasons.empty());
  EXPECT_NE(result.rejected_lines[0].reasons[0].message.find("separate block"),
            std::string::npos);
}

TEST(MessagesTest, MotionModalGroupTracksG1ThenG2) {
  const std::string input = "G1 X10\nG2 X20 I1 J2\n";
  const auto result = gcode::parseAndLower(input);

  ASSERT_EQ(result.messages.size(), 2u);
  ASSERT_TRUE(std::holds_alternative<gcode::G1Message>(result.messages[0]));
  ASSERT_TRUE(std::holds_alternative<gcode::G2Message>(result.messages[1]));

  const auto &g1 = std::get<gcode::G1Message>(result.messages[0]);
  EXPECT_EQ(g1.modal.group, gcode::ModalGroupId::Motion);
  EXPECT_EQ(g1.modal.code, "G1");
  EXPECT_TRUE(g1.modal.updates_state);

  const auto &g2 = std::get<gcode::G2Message>(result.messages[1]);
  EXPECT_EQ(g2.modal.group, gcode::ModalGroupId::Motion);
  EXPECT_EQ(g2.modal.code, "G2");
  EXPECT_TRUE(g2.modal.updates_state);
}

} // namespace
