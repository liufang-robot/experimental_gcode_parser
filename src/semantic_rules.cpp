#include "semantic_rules.h"

#include <memory>
#include <vector>

namespace gcode {
namespace {

class LineSemanticRule {
public:
  virtual ~LineSemanticRule() = default;
  virtual void apply(const Line &line,
                     std::vector<Diagnostic> *diagnostics) const = 0;
};

bool isMotionWord(const Word &word, int *out_code) {
  if (word.head != "G" || !word.value.has_value()) {
    return false;
  }
  try {
    int code = std::stoi(*word.value);
    if (code == 1 || code == 2 || code == 3) {
      if (out_code) {
        *out_code = code;
      }
      return true;
    }
  } catch (...) {
    return false;
  }
  return false;
}

bool isCartesianWord(const Word &word) {
  return word.head == "X" || word.head == "Y" || word.head == "Z" ||
         word.head == "A";
}

bool isPolarWord(const Word &word) {
  return word.head == "AP" || word.head == "RP";
}

void addDiagnostic(std::vector<Diagnostic> *diagnostics, const Location &loc,
                   const std::string &message) {
  Diagnostic diag;
  diag.severity = Diagnostic::Severity::Error;
  diag.message = message;
  diag.location = loc;
  diagnostics->push_back(std::move(diag));
}

class MotionExclusivityRule final : public LineSemanticRule {
public:
  void apply(const Line &line,
             std::vector<Diagnostic> *diagnostics) const override {
    int motion_code = 0;
    bool has_motion = false;
    for (const auto &item : line.items) {
      if (!std::holds_alternative<Word>(item)) {
        continue;
      }
      const auto &word = std::get<Word>(item);
      int code = 0;
      if (!isMotionWord(word, &code)) {
        continue;
      }
      if (has_motion && code != motion_code) {
        addDiagnostic(
            diagnostics, word.location,
            "multiple motion commands in one line; choose only one of "
            "G1/G2/G3");
        return;
      }
      has_motion = true;
      motion_code = code;
    }
  }
};

class G1CoordinateModeRule final : public LineSemanticRule {
public:
  void apply(const Line &line,
             std::vector<Diagnostic> *diagnostics) const override {
    int motion_code = 0;
    bool has_motion = false;
    bool has_cartesian = false;
    bool has_polar = false;

    for (const auto &item : line.items) {
      if (!std::holds_alternative<Word>(item)) {
        continue;
      }
      const auto &word = std::get<Word>(item);
      int code = 0;
      if (isMotionWord(word, &code)) {
        has_motion = true;
        motion_code = code;
        continue;
      }

      if (isCartesianWord(word)) {
        has_cartesian = true;
      }
      if (isPolarWord(word)) {
        has_polar = true;
      }

      if (has_cartesian && has_polar && has_motion && motion_code == 1) {
        addDiagnostic(diagnostics, word.location,
                      "mixed cartesian (X/Y/Z/A) and polar (AP/RP) words in G1 "
                      "line; choose one coordinate mode");
        return;
      }
    }
  }
};

std::vector<std::unique_ptr<LineSemanticRule>> makeRules() {
  std::vector<std::unique_ptr<LineSemanticRule>> rules;
  rules.push_back(std::make_unique<MotionExclusivityRule>());
  rules.push_back(std::make_unique<G1CoordinateModeRule>());
  return rules;
}

} // namespace

void addSemanticDiagnostics(ParseResult &result) {
  auto rules = makeRules();
  for (const auto &line : result.program.lines) {
    for (const auto &rule : rules) {
      const size_t before = result.diagnostics.size();
      rule->apply(line, &result.diagnostics);
      if (result.diagnostics.size() != before) {
        break;
      }
    }
  }
}

} // namespace gcode
