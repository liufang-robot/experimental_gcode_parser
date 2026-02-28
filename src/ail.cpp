#include "ail.h"

#include <type_traits>

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

AilResult lowerToAil(const Program &program,
                     const std::vector<Diagnostic> &parse_diagnostics,
                     const LowerOptions &options) {
  const auto lowered = lowerToMessages(program, parse_diagnostics, options);

  AilResult result;
  result.diagnostics = lowered.diagnostics;
  result.rejected_lines = lowered.rejected_lines;
  result.instructions.reserve(lowered.messages.size());
  for (const auto &message : lowered.messages) {
    result.instructions.push_back(toInstruction(message));
  }
  return result;
}

AilResult parseAndLowerAil(std::string_view input,
                           const LowerOptions &options) {
  const auto parsed = parse(input);
  return lowerToAil(parsed.program, parsed.diagnostics, options);
}

} // namespace gcode
