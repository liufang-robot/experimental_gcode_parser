#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "messages.h"

namespace {

struct Failure {
  std::string message;
};

void require(bool condition, const std::string &message,
             std::vector<Failure> &failures) {
  if (!condition) {
    failures.push_back({message});
  }
}

bool closeEnough(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-9; }

const gcode::G1Message *asG1(const gcode::ParsedMessage &msg) {
  if (!std::holds_alternative<gcode::G1Message>(msg)) {
    return nullptr;
  }
  return &std::get<gcode::G1Message>(msg);
}

void testG1Extraction(std::vector<Failure> &failures) {
  const std::string input = "N10 G1 X10 Y20 Z30 A40 B50 C60 F100\n";
  gcode::LowerOptions options;
  options.filename = "job.ngc";
  const auto result = gcode::parseAndLower(input, options);

  require(result.diagnostics.empty(), "expected no diagnostics", failures);
  require(result.messages.size() == 1, "expected one G1 message", failures);
  if (result.messages.empty()) {
    return;
  }
  const auto *g1 = asG1(result.messages[0]);
  require(g1 != nullptr, "expected message type G1", failures);
  if (!g1) {
    return;
  }
  require(g1->source.filename.has_value() && g1->source.filename == "job.ngc",
          "expected filename in source", failures);
  require(g1->source.line == 1, "expected source line 1", failures);
  require(g1->source.line_number.has_value() && g1->source.line_number == 10,
          "expected N10 line number", failures);
  require(g1->target_pose.x.has_value() &&
              closeEnough(*g1->target_pose.x, 10.0),
          "expected X=10", failures);
  require(g1->target_pose.y.has_value() &&
              closeEnough(*g1->target_pose.y, 20.0),
          "expected Y=20", failures);
  require(g1->target_pose.z.has_value() &&
              closeEnough(*g1->target_pose.z, 30.0),
          "expected Z=30", failures);
  require(g1->target_pose.a.has_value() &&
              closeEnough(*g1->target_pose.a, 40.0),
          "expected A=40", failures);
  require(g1->target_pose.b.has_value() &&
              closeEnough(*g1->target_pose.b, 50.0),
          "expected B=50", failures);
  require(g1->target_pose.c.has_value() &&
              closeEnough(*g1->target_pose.c, 60.0),
          "expected C=60", failures);
  require(g1->feed.has_value() && closeEnough(*g1->feed, 100.0),
          "expected F=100", failures);
}

void testLowercaseWordsEquivalent(std::vector<Failure> &failures) {
  const std::string input = "n7 g1 x1.5 y2 z3 a4 b5 c6 f7\n";
  gcode::LowerOptions options;
  options.filename = "lower.ngc";
  const auto result = gcode::parseAndLower(input, options);

  require(result.diagnostics.empty(), "expected no diagnostics for lowercase",
          failures);
  require(result.rejected_lines.empty(),
          "expected no rejected lines for lowercase", failures);
  require(result.messages.size() == 1, "expected one lowercase G1 message",
          failures);
  if (result.messages.empty()) {
    return;
  }
  const auto *g1 = asG1(result.messages[0]);
  require(g1 != nullptr, "expected lowercase message type G1", failures);
  if (!g1) {
    return;
  }
  require(g1->source.filename.has_value() && g1->source.filename == "lower.ngc",
          "expected lowercase filename in source", failures);
  require(g1->source.line == 1, "expected lowercase source line 1", failures);
  require(g1->source.line_number.has_value() && g1->source.line_number == 7,
          "expected lowercase line number n7", failures);
  require(g1->target_pose.x.has_value() && closeEnough(*g1->target_pose.x, 1.5),
          "expected lowercase x=1.5", failures);
  require(g1->target_pose.y.has_value() && closeEnough(*g1->target_pose.y, 2.0),
          "expected lowercase y=2", failures);
  require(g1->target_pose.z.has_value() && closeEnough(*g1->target_pose.z, 3.0),
          "expected lowercase z=3", failures);
  require(g1->target_pose.a.has_value() && closeEnough(*g1->target_pose.a, 4.0),
          "expected lowercase a=4", failures);
  require(g1->target_pose.b.has_value() && closeEnough(*g1->target_pose.b, 5.0),
          "expected lowercase b=5", failures);
  require(g1->target_pose.c.has_value() && closeEnough(*g1->target_pose.c, 6.0),
          "expected lowercase c=6", failures);
  require(g1->feed.has_value() && closeEnough(*g1->feed, 7.0),
          "expected lowercase f=7", failures);
}

void testStopAtFirstError(std::vector<Failure> &failures) {
  const std::string input = "G1 X10\nG1 G2 X10\nG1 X20\n";
  const auto result = gcode::parseAndLower(input);

  require(result.messages.size() == 1,
          "expected one message when lowering stops at first error", failures);
  require(result.rejected_lines.size() == 1,
          "expected one rejected line in stop mode", failures);
  if (!result.rejected_lines.empty()) {
    require(result.rejected_lines[0].source.line == 2,
            "expected rejected line to be line 2 in stop mode", failures);
  }
  if (!result.messages.empty()) {
    const auto *first = asG1(result.messages[0]);
    require(first != nullptr, "expected first message to be G1 in stop mode",
            failures);
    if (first) {
      require(first->source.line == 1,
              "expected only first line message in stop mode", failures);
    }
  }
}

} // namespace

int main() {
  std::vector<Failure> failures;
  testG1Extraction(failures);
  testLowercaseWordsEquivalent(failures);
  testStopAtFirstError(failures);

  if (failures.empty()) {
    return 0;
  }
  for (const auto &failure : failures) {
    std::cerr << failure.message << "\n";
  }
  return 1;
}
