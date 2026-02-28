#include "ail.h"

#include <type_traits>
#include <unordered_map>

#include "gcode_parser.h"

namespace gcode {
namespace {

AilInstruction toInstruction(const ParsedMessage &message) {
  return std::visit(
      [](const auto &msg) -> AilInstruction {
        using T = std::decay_t<decltype(msg)>;
        if constexpr (std::is_same_v<T, G1Message>) {
          AilLinearMoveInstruction inst;
          inst.source = msg.source;
          inst.modal = msg.modal;
          inst.target_pose = msg.target_pose;
          inst.feed = msg.feed;
          return inst;
        } else if constexpr (std::is_same_v<T, G2Message>) {
          AilArcMoveInstruction inst;
          inst.source = msg.source;
          inst.modal = msg.modal;
          inst.clockwise = true;
          inst.target_pose = msg.target_pose;
          inst.arc = msg.arc;
          inst.feed = msg.feed;
          return inst;
        } else if constexpr (std::is_same_v<T, G3Message>) {
          AilArcMoveInstruction inst;
          inst.source = msg.source;
          inst.modal = msg.modal;
          inst.clockwise = false;
          inst.target_pose = msg.target_pose;
          inst.arc = msg.arc;
          inst.feed = msg.feed;
          return inst;
        } else {
          AilDwellInstruction inst;
          inst.source = msg.source;
          inst.modal = msg.modal;
          inst.dwell_mode = msg.dwell_mode;
          inst.dwell_value = msg.dwell_value;
          return inst;
        }
      },
      message);
}

} // namespace

bool lineHasError(const std::vector<Diagnostic> &diagnostics, int line) {
  for (const auto &diag : diagnostics) {
    if (diag.severity == Diagnostic::Severity::Error &&
        diag.location.line == line) {
      return true;
    }
  }
  return false;
}

SourceInfo sourceFromLine(const Line &line, const LowerOptions &options) {
  SourceInfo source;
  source.filename = options.filename;
  source.line = line.line_index;
  if (line.line_number.has_value()) {
    source.line_number = line.line_number->value;
  }
  return source;
}

AilResult lowerToAil(const Program &program,
                     const std::vector<Diagnostic> &parse_diagnostics,
                     const LowerOptions &options) {
  const auto lowered = lowerToMessages(program, parse_diagnostics, options);

  AilResult result;
  result.diagnostics = lowered.diagnostics;
  result.rejected_lines = lowered.rejected_lines;
  result.instructions.reserve(lowered.messages.size() + program.lines.size());

  std::unordered_map<int, AilInstruction> message_by_line;
  for (const auto &message : lowered.messages) {
    const int line =
        std::visit([](const auto &msg) { return msg.source.line; }, message);
    message_by_line.emplace(line, toInstruction(message));
  }

  int stop_line = 0;
  if (!lowered.rejected_lines.empty()) {
    stop_line = lowered.rejected_lines.front().source.line;
  }

  for (const auto &line : program.lines) {
    if (stop_line != 0 && line.line_index >= stop_line) {
      break;
    }
    if (lineHasError(result.diagnostics, line.line_index)) {
      continue;
    }
    if (line.assignment.has_value()) {
      AilAssignInstruction inst;
      inst.source = sourceFromLine(line, options);
      inst.lhs = line.assignment->lhs;
      inst.rhs = line.assignment->rhs;
      result.instructions.push_back(std::move(inst));
      continue;
    }
    const auto found = message_by_line.find(line.line_index);
    if (found != message_by_line.end()) {
      result.instructions.push_back(found->second);
    }
  }
  return result;
}

AilResult parseAndLowerAil(std::string_view input,
                           const LowerOptions &options) {
  const auto parsed = parse(input);
  return lowerToAil(parsed.program, parsed.diagnostics, options);
}

} // namespace gcode
