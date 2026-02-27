#include "messages.h"

#include <fstream>
#include <sstream>
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

bool shouldStop(const StreamOptions &stream_options, size_t lines_seen,
                size_t messages_seen, size_t diagnostics_seen) {
  if (stream_options.max_lines.has_value() &&
      lines_seen > *stream_options.max_lines) {
    return true;
  }
  if (stream_options.max_messages.has_value() &&
      messages_seen > *stream_options.max_messages) {
    return true;
  }
  if (stream_options.max_diagnostics.has_value() &&
      diagnostics_seen > *stream_options.max_diagnostics) {
    return true;
  }
  if (stream_options.should_cancel && stream_options.should_cancel()) {
    return true;
  }
  return false;
}

void emitParseDiagnostics(const std::vector<Diagnostic> &diagnostics,
                          const StreamCallbacks &callbacks,
                          const StreamOptions &stream_options,
                          size_t *diagnostics_seen, bool *stopped) {
  for (const auto &diag : diagnostics) {
    if (callbacks.on_diagnostic) {
      callbacks.on_diagnostic(diag);
    }
    ++(*diagnostics_seen);
    if (shouldStop(stream_options, 0, 0, *diagnostics_seen)) {
      *stopped = true;
      return;
    }
  }
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

bool lowerToMessagesStream(const Program &program,
                           const std::vector<Diagnostic> &parse_diagnostics,
                           const LowerOptions &options,
                           const StreamCallbacks &callbacks,
                           const StreamOptions &stream_options) {
  const auto lowerers = createMotionFamilyLowerers();
  const auto indexed_lowerers = indexLowerers(lowerers);

  size_t lines_seen = 0;
  size_t messages_seen = 0;
  size_t diagnostics_seen = 0;
  bool stopped = false;
  emitParseDiagnostics(parse_diagnostics, callbacks, stream_options,
                       &diagnostics_seen, &stopped);
  if (stopped) {
    return false;
  }

  for (const auto &line : program.lines) {
    ++lines_seen;
    if (shouldStop(stream_options, lines_seen, messages_seen,
                   diagnostics_seen)) {
      return false;
    }

    const bool has_line_error =
        lineHasError(parse_diagnostics, line.line_index);
    if (has_line_error) {
      MessageResult::RejectedLine rejected;
      rejected.source.filename = options.filename;
      rejected.source.line = line.line_index;
      if (line.line_number.has_value()) {
        rejected.source.line_number = line.line_number->value;
      }
      rejected.reasons = collectLineErrors(parse_diagnostics, line.line_index);
      if (callbacks.on_rejected_line) {
        callbacks.on_rejected_line(rejected);
      }
      return false;
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

    std::vector<Diagnostic> lowering_diags;
    auto message = found->second->lower(line, options, &lowering_diags);
    if (callbacks.on_message) {
      callbacks.on_message(message);
    }
    ++messages_seen;
    if (shouldStop(stream_options, lines_seen, messages_seen,
                   diagnostics_seen)) {
      return false;
    }

    for (const auto &diag : lowering_diags) {
      if (callbacks.on_diagnostic) {
        callbacks.on_diagnostic(diag);
      }
      ++diagnostics_seen;
      if (shouldStop(stream_options, lines_seen, messages_seen,
                     diagnostics_seen)) {
        return false;
      }
    }
  }

  return true;
}

bool parseAndLowerStream(std::string_view input, const LowerOptions &options,
                         const StreamCallbacks &callbacks,
                         const StreamOptions &stream_options) {
  const ParseResult parsed = parse(input);
  return lowerToMessagesStream(parsed.program, parsed.diagnostics, options,
                               callbacks, stream_options);
}

bool parseAndLowerFileStream(const std::string &path,
                             const LowerOptions &options,
                             const StreamCallbacks &callbacks,
                             const StreamOptions &stream_options) {
  std::ifstream input(path, std::ios::in);
  if (!input.is_open()) {
    if (callbacks.on_diagnostic) {
      Diagnostic diag;
      diag.severity = Diagnostic::Severity::Error;
      diag.message = "failed to open file: " + path;
      diag.location = {0, 0};
      callbacks.on_diagnostic(diag);
    }
    return false;
  }

  std::stringstream buffer;
  buffer << input.rdbuf();
  LowerOptions file_options = options;
  if (!file_options.filename.has_value()) {
    file_options.filename = path;
  }
  return parseAndLowerStream(buffer.str(), file_options, callbacks,
                             stream_options);
}

} // namespace gcode
