#include "messages.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "gcode_parser.h"
#include "lowering_family.h"

namespace gcode {
namespace {

enum class WorkingPlaneState { XY, ZX, YZ };

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

bool isSkipLevelActive(const LowerOptions &options, int level) {
  return std::find(options.active_skip_levels.begin(),
                   options.active_skip_levels.end(),
                   level) != options.active_skip_levels.end();
}

bool shouldSkipLine(const Line &line, const LowerOptions &options) {
  if (!line.block_delete) {
    return false;
  }
  const int level = line.block_delete_level.value_or(0);
  return isSkipLevelActive(options, level);
}

int motionCode(const Word &word) {
  if (word.head != "G" || !word.value.has_value()) {
    return -1;
  }
  try {
    return std::stoi(*word.value);
  } catch (...) {
    return -1;
  }
}

std::optional<WorkingPlaneState> planeFromWord(const Word &word) {
  if (word.head != "G" || !word.value.has_value()) {
    return std::nullopt;
  }
  try {
    const int code = std::stoi(*word.value);
    if (code == 17) {
      return WorkingPlaneState::XY;
    }
    if (code == 18) {
      return WorkingPlaneState::ZX;
    }
    if (code == 19) {
      return WorkingPlaneState::YZ;
    }
  } catch (...) {
    return std::nullopt;
  }
  return std::nullopt;
}

void addErrorDiagnostic(std::vector<Diagnostic> *diagnostics,
                        const Location &location, const std::string &message) {
  Diagnostic diag;
  diag.severity = Diagnostic::Severity::Error;
  diag.message = message;
  diag.location = location;
  diagnostics->push_back(std::move(diag));
}

void validateArcPlaneWords(const Line &line, WorkingPlaneState plane,
                           std::vector<Diagnostic> *diagnostics) {
  for (const auto &item : line.items) {
    if (!isWord(item)) {
      continue;
    }
    const auto &word = std::get<Word>(item);
    if ((word.head != "I" && word.head != "J" && word.head != "K") ||
        !word.value.has_value()) {
      continue;
    }

    bool allowed = false;
    if (plane == WorkingPlaneState::XY) {
      allowed = (word.head == "I" || word.head == "J");
    } else if (plane == WorkingPlaneState::ZX) {
      allowed = (word.head == "I" || word.head == "K");
    } else {
      allowed = (word.head == "J" || word.head == "K");
    }

    if (!allowed) {
      const char *plane_name =
          plane == WorkingPlaneState::XY
              ? "G17 (XY)"
              : (plane == WorkingPlaneState::ZX ? "G18 (ZX)" : "G19 (YZ)");
      addErrorDiagnostic(diagnostics, word.location,
                         "arc center word '" + word.head +
                             "' does not match active working plane " +
                             plane_name);
    }
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
  WorkingPlaneState current_plane = WorkingPlaneState::XY;

  for (const auto &line : program.lines) {
    if (shouldSkipLine(line, options)) {
      continue;
    }
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

    bool has_motion = false;
    int found_motion = 0;
    std::optional<WorkingPlaneState> line_plane_override;
    for (const auto &item : line.items) {
      if (!isWord(item)) {
        continue;
      }
      const auto &word = std::get<Word>(item);
      const auto parsed_plane = planeFromWord(word);
      if (parsed_plane.has_value()) {
        line_plane_override = *parsed_plane;
      }
      const int code = motionCode(word);
      if (code >= 0 && code <= 4) {
        if (has_motion && code != found_motion) {
          found_motion = -1;
          break;
        }
        has_motion = true;
        found_motion = code;
      }
    }

    if (!has_motion) {
      if (line_plane_override.has_value()) {
        current_plane = *line_plane_override;
      }
      continue;
    }

    const WorkingPlaneState effective_plane =
        line_plane_override.value_or(current_plane);

    if (found_motion == 2 || found_motion == 3) {
      validateArcPlaneWords(line, effective_plane, &result.diagnostics);
      if (lineHasError(result.diagnostics, line.line_index)) {
        MessageResult::RejectedLine rejected;
        rejected.source.filename = options.filename;
        rejected.source.line = line.line_index;
        if (line.line_number.has_value()) {
          rejected.source.line_number = line.line_number->value;
        }
        rejected.reasons =
            collectLineErrors(result.diagnostics, line.line_index);
        result.rejected_lines.push_back(std::move(rejected));
        break;
      }
    }

    const auto found = indexed_lowerers.find(found_motion);
    if (found == indexed_lowerers.end()) {
      if (line_plane_override.has_value()) {
        current_plane = *line_plane_override;
      }
      continue;
    }
    result.messages.push_back(
        found->second->lower(line, options, &result.diagnostics));
    if (line_plane_override.has_value()) {
      current_plane = *line_plane_override;
    }
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
  WorkingPlaneState current_plane = WorkingPlaneState::XY;

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
    if (shouldSkipLine(line, options)) {
      continue;
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

    bool has_motion = false;
    int found_motion = 0;
    std::optional<WorkingPlaneState> line_plane_override;
    for (const auto &item : line.items) {
      if (!isWord(item)) {
        continue;
      }
      const auto &word = std::get<Word>(item);
      const auto parsed_plane = planeFromWord(word);
      if (parsed_plane.has_value()) {
        line_plane_override = *parsed_plane;
      }
      const int code = motionCode(word);
      if (code >= 0 && code <= 4) {
        if (has_motion && code != found_motion) {
          found_motion = -1;
          break;
        }
        has_motion = true;
        found_motion = code;
      }
    }

    if (!has_motion) {
      if (line_plane_override.has_value()) {
        current_plane = *line_plane_override;
      }
      continue;
    }
    const WorkingPlaneState effective_plane =
        line_plane_override.value_or(current_plane);
    if (found_motion == 2 || found_motion == 3) {
      std::vector<Diagnostic> plane_diags;
      validateArcPlaneWords(line, effective_plane, &plane_diags);
      for (const auto &diag : plane_diags) {
        if (callbacks.on_diagnostic) {
          callbacks.on_diagnostic(diag);
        }
        ++diagnostics_seen;
      }
      const bool has_plane_error = std::any_of(
          plane_diags.begin(), plane_diags.end(), [](const Diagnostic &diag) {
            return diag.severity == Diagnostic::Severity::Error;
          });
      if (has_plane_error) {
        MessageResult::RejectedLine rejected;
        rejected.source.filename = options.filename;
        rejected.source.line = line.line_index;
        if (line.line_number.has_value()) {
          rejected.source.line_number = line.line_number->value;
        }
        rejected.reasons = std::move(plane_diags);
        if (callbacks.on_rejected_line) {
          callbacks.on_rejected_line(rejected);
        }
        return false;
      }
    }
    const auto found = indexed_lowerers.find(found_motion);
    if (found == indexed_lowerers.end()) {
      if (line_plane_override.has_value()) {
        current_plane = *line_plane_override;
      }
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
    if (line_plane_override.has_value()) {
      current_plane = *line_plane_override;
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
