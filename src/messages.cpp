#include "messages.h"

#include <unordered_map>

#include "gcode_parser.h"
#include "lowering_family.h"

namespace gcode {
namespace {

bool lineHasError(const std::vector<Diagnostic> &diagnostics, int line) {
  for (const auto &diag : diagnostics) {
    if (diag.severity == Diagnostic::Severity::Error &&
        diag.location.line == line) {
      return true;
    }
  }
  return false;
}

std::vector<Diagnostic>
collectLineErrors(const std::vector<Diagnostic> &diagnostics, int line) {
  std::vector<Diagnostic> errors;
  for (const auto &diag : diagnostics) {
    if (diag.severity == Diagnostic::Severity::Error &&
        diag.location.line == line) {
      errors.push_back(diag);
    }
  }
  return errors;
}

bool isWord(const LineItem &item) { return std::holds_alternative<Word>(item); }

int motionCode(const Word &word) {
  if (word.head != "G" || !word.value.has_value()) {
    return 0;
  }
  try {
    return std::stoi(*word.value);
  } catch (...) {
    return 0;
  }
}

std::unordered_map<int, const MotionFamilyLowerer *> indexLowerers(
    const std::vector<std::unique_ptr<MotionFamilyLowerer>> &lowerers) {
  std::unordered_map<int, const MotionFamilyLowerer *> indexed;
  for (const auto &lowerer : lowerers) {
    indexed[lowerer->motionCode()] = lowerer.get();
  }
  return indexed;
}

} // namespace

MessageResult lowerToMessages(const Program &program,
                              const std::vector<Diagnostic> &parse_diagnostics,
                              const LowerOptions &options) {
  MessageResult result;
  result.diagnostics = parse_diagnostics;
  const auto lowerers = createMotionFamilyLowerers();
  const auto indexed_lowerers = indexLowerers(lowerers);

  for (const auto &line : program.lines) {
    const bool has_line_error =
        lineHasError(result.diagnostics, line.line_index);
    if (has_line_error) {
      MessageResult::RejectedLine rejected;
      rejected.source.filename = options.filename;
      rejected.source.line = line.line_index;
      if (line.line_number.has_value()) {
        rejected.source.line_number = line.line_number->value;
      }
      rejected.reasons = collectLineErrors(result.diagnostics, line.line_index);
      result.rejected_lines.push_back(std::move(rejected));
      break;
    }

    int found_motion = 0;
    for (const auto &item : line.items) {
      if (!isWord(item)) {
        continue;
      }
      const auto &word = std::get<Word>(item);
      const int code = motionCode(word);
      if (code == 1 || code == 2 || code == 3 || code == 4) {
        if (found_motion != 0 && code != found_motion) {
          found_motion = -1;
          break;
        }
        found_motion = code;
      }
    }

    if (found_motion == 0) {
      continue;
    }

    const auto found = indexed_lowerers.find(found_motion);
    if (found == indexed_lowerers.end()) {
      continue;
    }
    result.messages.push_back(
        found->second->lower(line, options, &result.diagnostics));
  }

  return result;
}

MessageResult parseAndLower(std::string_view input,
                            const LowerOptions &options) {
  const ParseResult parsed = parse(input);
  return lowerToMessages(parsed.program, parsed.diagnostics, options);
}

} // namespace gcode
