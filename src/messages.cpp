#include "messages.h"

#include <cctype>

#include "gcode_parser.h"

namespace gcode {
namespace {

bool parseDouble(const std::optional<std::string> &text, double *value) {
  if (!text.has_value()) {
    return false;
  }
  try {
    *value = std::stod(*text);
    return true;
  } catch (...) {
    return false;
  }
}

bool lineHasError(const std::vector<Diagnostic> &diagnostics, int line) {
  for (const auto &diag : diagnostics) {
    if (diag.severity == Diagnostic::Severity::Error &&
        diag.location.line == line) {
      return true;
    }
  }
  return false;
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

} // namespace

MessageResult lowerToMessages(const Program &program,
                              const std::vector<Diagnostic> &parse_diagnostics,
                              const LowerOptions &options) {
  MessageResult result;
  result.diagnostics = parse_diagnostics;

  for (const auto &line : program.lines) {
    if (options.skip_error_lines &&
        lineHasError(result.diagnostics, line.line_index)) {
      continue;
    }

    int found_motion = 0;
    for (const auto &item : line.items) {
      if (!isWord(item)) {
        continue;
      }
      const auto &word = std::get<Word>(item);
      const int code = motionCode(word);
      if (code == 1 || code == 2 || code == 3) {
        if (found_motion != 0 && code != found_motion) {
          found_motion = -1;
          break;
        }
        found_motion = code;
      }
    }

    if (found_motion != 1) {
      continue;
    }

    G1Message message;
    message.source.filename = options.filename;
    message.source.line = line.line_index;
    if (line.line_number.has_value()) {
      message.source.line_number = line.line_number->value;
    }

    for (const auto &item : line.items) {
      if (!isWord(item)) {
        continue;
      }
      const auto &word = std::get<Word>(item);
      double parsed = 0.0;
      if (word.head == "X" && parseDouble(word.value, &parsed)) {
        message.target_pose.x = parsed;
      } else if (word.head == "Y" && parseDouble(word.value, &parsed)) {
        message.target_pose.y = parsed;
      } else if (word.head == "Z" && parseDouble(word.value, &parsed)) {
        message.target_pose.z = parsed;
      } else if (word.head == "A" && parseDouble(word.value, &parsed)) {
        message.target_pose.a = parsed;
      } else if (word.head == "B" && parseDouble(word.value, &parsed)) {
        message.target_pose.b = parsed;
      } else if (word.head == "C" && parseDouble(word.value, &parsed)) {
        message.target_pose.c = parsed;
      } else if (word.head == "F" && parseDouble(word.value, &parsed)) {
        message.feed = parsed;
      }
    }

    result.messages.emplace_back(std::move(message));
  }

  return result;
}

MessageResult parseAndLower(std::string_view input,
                            const LowerOptions &options) {
  const ParseResult parsed = parse(input);
  return lowerToMessages(parsed.program, parsed.diagnostics, options);
}

} // namespace gcode
