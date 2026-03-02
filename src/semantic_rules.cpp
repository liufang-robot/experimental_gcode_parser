#include "semantic_rules.h"

#include <cctype>
#include <memory>
#include <unordered_map>
#include <vector>

#include "lowering_family_common.h"

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
    if (code == 1 || code == 2 || code == 3 || code == 4) {
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

bool isUnsignedIntegerText(const std::string &text) {
  if (text.empty()) {
    return false;
  }
  for (char c : text) {
    if (!std::isdigit(static_cast<unsigned char>(c))) {
      return false;
    }
  }
  return true;
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
      if (code == 4) {
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

class G4BlockRule final : public LineSemanticRule {
public:
  void apply(const Line &line,
             std::vector<Diagnostic> *diagnostics) const override {
    const Word *g4_word = nullptr;
    const Word *f_word = nullptr;
    const Word *s_word = nullptr;
    const Word *first_other_word = nullptr;

    for (const auto &item : line.items) {
      if (!std::holds_alternative<Word>(item)) {
        continue;
      }
      const auto &word = std::get<Word>(item);
      int code = 0;
      if (isMotionWord(word, &code) && code == 4) {
        if (!g4_word) {
          g4_word = &word;
        } else if (!first_other_word) {
          first_other_word = &word;
        }
        continue;
      }
      if (word.head == "F") {
        if (!f_word) {
          f_word = &word;
        } else if (!first_other_word) {
          first_other_word = &word;
        }
        continue;
      }
      if (word.head == "S") {
        if (!s_word) {
          s_word = &word;
        } else if (!first_other_word) {
          first_other_word = &word;
        }
        continue;
      }
      if (!first_other_word) {
        first_other_word = &word;
      }
    }

    if (!g4_word) {
      return;
    }

    if (first_other_word) {
      addDiagnostic(diagnostics, first_other_word->location,
                    "program G4 in a separate block; use only G4 with one of "
                    "F (seconds) or S (revolutions)");
      return;
    }

    if (!f_word && !s_word) {
      addDiagnostic(diagnostics, g4_word->location,
                    "G4 dwell requires F (seconds) or S (revolutions)");
      return;
    }

    if (f_word && s_word) {
      addDiagnostic(diagnostics, s_word->location,
                    "G4 dwell must use either F (seconds) or S "
                    "(revolutions), not both");
      return;
    }

    const Word *dwell_word = f_word ? f_word : s_word;
    double parsed_value = 0.0;
    if (!parseDoubleText(dwell_word->value, &parsed_value)) {
      addDiagnostic(diagnostics, dwell_word->location,
                    "G4 dwell value must be numeric");
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

class LineNumberWordRule final : public LineSemanticRule {
public:
  void apply(const Line &line,
             std::vector<Diagnostic> *diagnostics) const override {
    for (const auto &item : line.items) {
      if (!std::holds_alternative<Word>(item)) {
        continue;
      }
      const auto &word = std::get<Word>(item);
      if (word.head != "N") {
        continue;
      }
      if (!word.value.has_value() || !isUnsignedIntegerText(*word.value)) {
        addDiagnostic(diagnostics, word.location,
                      "invalid N-address; use unsigned integer form like N100");
        return;
      }
      addDiagnostic(diagnostics, word.location,
                    "N-address must be at block start (before statement)");
      return;
    }
  }
};

std::vector<std::unique_ptr<LineSemanticRule>> makeRules() {
  std::vector<std::unique_ptr<LineSemanticRule>> rules;
  rules.push_back(std::make_unique<G4BlockRule>());
  rules.push_back(std::make_unique<MotionExclusivityRule>());
  rules.push_back(std::make_unique<G1CoordinateModeRule>());
  rules.push_back(std::make_unique<LineNumberWordRule>());
  return rules;
}

} // namespace

void addSemanticDiagnostics(ParseResult &result) {
  auto rules = makeRules();
  std::unordered_map<int, Location> first_line_number_at;
  bool has_line_number_target_jump = false;
  for (const auto &line : result.program.lines) {
    if (line.goto_statement.has_value()) {
      const auto &target_kind = line.goto_statement->target_kind;
      if (target_kind == "line_number" || target_kind == "number") {
        has_line_number_target_jump = true;
        break;
      }
    }
    if (line.if_goto_statement.has_value()) {
      const auto &then_kind = line.if_goto_statement->then_branch.target_kind;
      if (then_kind == "line_number" || then_kind == "number") {
        has_line_number_target_jump = true;
        break;
      }
      if (line.if_goto_statement->else_branch.has_value()) {
        const auto &else_kind =
            line.if_goto_statement->else_branch->target_kind;
        if (else_kind == "line_number" || else_kind == "number") {
          has_line_number_target_jump = true;
          break;
        }
      }
    }
  }

  for (const auto &line : result.program.lines) {
    if (has_line_number_target_jump && line.line_number.has_value()) {
      const int n = line.line_number->value;
      const auto [it, inserted] =
          first_line_number_at.emplace(n, line.line_number->location);
      if (!inserted) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Warning;
        diag.message = "duplicate N-address N" + std::to_string(n) +
                       "; jumps by line number may be ambiguous";
        diag.location = line.line_number->location;
        result.diagnostics.push_back(std::move(diag));
      }
    }
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
