#include "gcode/ail.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "gcode/execution_runtime.h"
#include "gcode/gcode_parser.h"

namespace gcode {
namespace {

ToolSelectionResolution
makeResolvedToolSelection(const ToolSelectionState &selection, bool substituted,
                          const std::string &message) {
  ToolSelectionResolution resolution;
  resolution.kind = ToolSelectionResolutionKind::Resolved;
  resolution.selection = selection;
  resolution.substituted = substituted;
  resolution.message = message;
  return resolution;
}

ToolSelectionResolution
makeUnresolvedToolSelection(const ToolSelectionState &selection,
                            const std::string &message) {
  ToolSelectionResolution resolution;
  resolution.kind = ToolSelectionResolutionKind::Unresolved;
  resolution.selection = selection;
  resolution.message = message;
  return resolution;
}

ToolSelectionResolution
defaultResolveToolSelection(const ToolSelectionState &selection,
                            const AilExecutorOptions &options) {
  if (selection.selector_value.empty()) {
    if (options.fallback_tool_selection.has_value()) {
      return makeResolvedToolSelection(
          *options.fallback_tool_selection, true,
          "tool unresolved; fallback selection applied by policy");
    }
    return makeUnresolvedToolSelection(selection,
                                       "tool selection unresolved by policy");
  }

  if (options.allow_tool_substitution) {
    const auto it =
        options.tool_substitution_map.find(selection.selector_value);
    if (it != options.tool_substitution_map.end()) {
      ToolSelectionState mapped = selection;
      mapped.selector_value = it->second;
      return makeResolvedToolSelection(
          mapped, mapped.selector_value != selection.selector_value,
          "tool selection substituted by policy map");
    }
  }

  return makeResolvedToolSelection(selection, false, "");
}

std::string bareSubprogramName(const std::string &target) {
  const auto pos = target.find_last_of("/\\");
  if (pos == std::string::npos || pos + 1 >= target.size()) {
    return target;
  }
  return target.substr(pos + 1);
}

SubprogramResolution
defaultResolveSubprogramTarget(const std::string &requested_target,
                               const LabelPositionMap &label_positions,
                               const AilExecutorOptions &options) {
  SubprogramResolution resolution;
  const auto exact_it = label_positions.find(requested_target);
  if (exact_it != label_positions.end() && !exact_it->second.empty()) {
    resolution.resolved = true;
    resolution.resolved_target = requested_target;
    return resolution;
  }

  const auto alias_it = options.subprogram_alias_map.find(requested_target);
  if (alias_it != options.subprogram_alias_map.end()) {
    const auto resolved_alias_it = label_positions.find(alias_it->second);
    if (resolved_alias_it != label_positions.end() &&
        !resolved_alias_it->second.empty()) {
      resolution.resolved = true;
      resolution.resolved_target = alias_it->second;
      resolution.message =
          "subprogram target resolved by alias map: " + requested_target +
          " -> " + alias_it->second;
      return resolution;
    }
  }

  if (options.subprogram_search_policy ==
      SubprogramSearchPolicy::ExactThenBareName) {
    const std::string fallback = bareSubprogramName(requested_target);
    if (fallback != requested_target) {
      const auto fallback_it = label_positions.find(fallback);
      if (fallback_it != label_positions.end() &&
          !fallback_it->second.empty()) {
        resolution.resolved = true;
        resolution.resolved_target = fallback;
        resolution.fallback_used = true;
        resolution.message =
            "subprogram target resolved by bare-name fallback: " +
            requested_target + " -> " + fallback;
        return resolution;
      }
    }
  }

  resolution.resolved = false;
  resolution.resolved_target = requested_target;
  return resolution;
}

AilInstruction toInstruction(const ParsedMessage &message) {
  return std::visit(
      [](const auto &msg) -> AilInstruction {
        using T = std::decay_t<decltype(msg)>;
        if constexpr (std::is_same_v<T, G0Message>) {
          AilLinearMoveInstruction inst;
          inst.source = msg.source;
          inst.modal = msg.modal;
          inst.opcode = "G0";
          inst.target_pose = msg.target_pose;
          inst.feed = msg.feed;
          return inst;
        } else if constexpr (std::is_same_v<T, G1Message>) {
          AilLinearMoveInstruction inst;
          inst.source = msg.source;
          inst.modal = msg.modal;
          inst.opcode = "G1";
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

std::optional<int64_t> parseUnsignedInt64Strict(std::string_view text) {
  if (text.empty()) {
    return std::nullopt;
  }
  int64_t value = 0;
  for (char c : text) {
    if (!std::isdigit(static_cast<unsigned char>(c))) {
      return std::nullopt;
    }
    const int digit = c - '0';
    if (value > (std::numeric_limits<int64_t>::max() - digit) / 10) {
      return std::nullopt;
    }
    value = value * 10 + digit;
  }
  return value;
}

std::optional<AilMCodeInstruction> mCodeFromWord(const Word &word,
                                                 const SourceInfo &source) {
  if (word.head.empty() || word.head[0] != 'M' || !word.value.has_value()) {
    return std::nullopt;
  }
  const auto value = parseUnsignedInt64Strict(*word.value);
  if (!value.has_value()) {
    return std::nullopt;
  }

  const std::string extension_text = word.head.substr(1);
  std::optional<int64_t> extension;
  if (!extension_text.empty()) {
    if (!word.has_equal) {
      return std::nullopt;
    }
    extension = parseUnsignedInt64Strict(extension_text);
    if (!extension.has_value()) {
      return std::nullopt;
    }
  }

  AilMCodeInstruction inst;
  inst.source = source;
  inst.address_extension = extension;
  inst.value = *value;
  return inst;
}

std::optional<AilRapidTraverseModeInstruction>
rapidTraverseModeFromWord(const Word &word, const SourceInfo &source) {
  if (word.value.has_value()) {
    return std::nullopt;
  }
  AilRapidTraverseModeInstruction inst;
  if (word.head == "RTLION") {
    inst.source = source;
    inst.mode = RapidInterpolationMode::Linear;
    return inst;
  }
  if (word.head == "RTLIOF") {
    inst.source = source;
    inst.mode = RapidInterpolationMode::NonLinear;
    return inst;
  }
  return std::nullopt;
}

std::optional<AilToolRadiusCompInstruction>
toolRadiusCompFromWord(const Word &word, const SourceInfo &source) {
  if (word.head != "G" || !word.value.has_value()) {
    return std::nullopt;
  }

  int code = 0;
  try {
    code = std::stoi(*word.value);
  } catch (...) {
    return std::nullopt;
  }

  AilToolRadiusCompInstruction inst;
  inst.source = source;
  if (code == 40) {
    inst.mode = ToolRadiusCompMode::Off;
    return inst;
  }
  if (code == 41) {
    inst.mode = ToolRadiusCompMode::Left;
    return inst;
  }
  if (code == 42) {
    inst.mode = ToolRadiusCompMode::Right;
    return inst;
  }
  return std::nullopt;
}

std::optional<AilWorkingPlaneInstruction>
workingPlaneFromWord(const Word &word, const SourceInfo &source) {
  if (word.head != "G" || !word.value.has_value()) {
    return std::nullopt;
  }

  int code = 0;
  try {
    code = std::stoi(*word.value);
  } catch (...) {
    return std::nullopt;
  }

  AilWorkingPlaneInstruction inst;
  inst.source = source;
  if (code == 17) {
    inst.plane = WorkingPlane::XY;
    return inst;
  }
  if (code == 18) {
    inst.plane = WorkingPlane::ZX;
    return inst;
  }
  if (code == 19) {
    inst.plane = WorkingPlane::YZ;
    return inst;
  }
  return std::nullopt;
}

bool isKnownPredefinedMFunction(int64_t value) {
  if (value == 0 || value == 1 || value == 2 || value == 3 || value == 4 ||
      value == 5 || value == 6 || value == 17 || value == 19 || value == 30 ||
      value == 70) {
    return true;
  }
  return value >= 40 && value <= 45;
}

bool isDeselectSelector(const ToolSelectionState &selection) {
  return !selection.selector_index.has_value() &&
         selection.selector_value == "0";
}

std::string toUpper(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(),
      [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return value;
}

bool isDigits(std::string_view text) {
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

bool isToolSelectorHead(std::string_view head) {
  if (head == "T") {
    return true;
  }
  if (head.size() <= 1 || head.front() != 'T') {
    return false;
  }
  return isDigits(head.substr(1));
}

ToolActionTiming timingFromOptions(const LowerOptions &options) {
  const ToolChangeMode mode =
      options.tool_change_mode.value_or(ToolChangeMode::DeferredM6);
  return mode == ToolChangeMode::DirectT ? ToolActionTiming::Immediate
                                         : ToolActionTiming::DeferredUntilM6;
}

std::optional<int64_t> parseSelectorIndex(std::string_view head) {
  if (head.size() <= 1) {
    return std::nullopt;
  }
  return parseUnsignedInt64Strict(head.substr(1));
}

std::optional<std::string> selectorTextFromExpr(const ExprNode &node) {
  if (auto *literal = std::get_if<ExprLiteral>(&node.node)) {
    if (std::floor(literal->value) == literal->value) {
      return std::to_string(static_cast<int64_t>(literal->value));
    }
    return std::to_string(literal->value);
  }
  if (auto *var = std::get_if<ExprVariable>(&node.node)) {
    return var->name;
  }
  return std::nullopt;
}

std::optional<AilToolSelectInstruction>
toolSelectFromWord(const Word &word, const SourceInfo &source,
                   const LowerOptions &options) {
  if (!isToolSelectorHead(word.head) || !word.value.has_value()) {
    return std::nullopt;
  }
  AilToolSelectInstruction inst;
  inst.source = source;
  inst.selector_index = parseSelectorIndex(word.head);
  inst.selector_value = *word.value;
  inst.timing = timingFromOptions(options);
  return inst;
}

std::optional<AilToolSelectInstruction>
toolSelectFromAssignment(const Assignment &assignment, const SourceInfo &source,
                         const LowerOptions &options) {
  if (!isToolSelectorHead(assignment.lhs) || !assignment.rhs) {
    return std::nullopt;
  }
  const auto selector = selectorTextFromExpr(*assignment.rhs);
  if (!selector.has_value()) {
    return std::nullopt;
  }
  AilToolSelectInstruction inst;
  inst.source = source;
  inst.selector_index = parseSelectorIndex(assignment.lhs);
  inst.selector_value = toUpper(*selector);
  inst.timing = timingFromOptions(options);
  return inst;
}

std::optional<AilToolChangeInstruction>
toolChangeFromMCode(const AilMCodeInstruction &mcode) {
  if (mcode.value != 6 || mcode.address_extension.has_value()) {
    return std::nullopt;
  }
  AilToolChangeInstruction inst;
  inst.source = mcode.source;
  return inst;
}

std::optional<AilReturnBoundaryInstruction>
returnBoundaryFromWord(const Word &word, const SourceInfo &source) {
  if (word.head != "RET" || word.value.has_value() || word.has_equal) {
    return std::nullopt;
  }
  AilReturnBoundaryInstruction inst;
  inst.source = source;
  inst.opcode = "RET";
  return inst;
}

std::optional<AilReturnBoundaryInstruction>
returnBoundaryFromMCode(const AilMCodeInstruction &mcode) {
  if (mcode.value != 17 || mcode.address_extension.has_value()) {
    return std::nullopt;
  }
  AilReturnBoundaryInstruction inst;
  inst.source = mcode.source;
  inst.opcode = "M17";
  return inst;
}

std::optional<int64_t> parseSubprogramRepeatCount(const Word &word) {
  if (word.head != "P" || !word.value.has_value()) {
    return std::nullopt;
  }
  return parseUnsignedInt64Strict(*word.value);
}

bool isSubprogramTargetWord(const Word &word) {
  if (word.quoted) {
    return !word.text.empty();
  }
  if (word.has_equal || word.text.empty() || word.head.empty()) {
    return false;
  }
  if (word.value.has_value()) {
    const char first = word.head.front();
    if (first == 'G' || first == 'M' || first == 'T' || first == 'P' ||
        first == 'N' || first == 'X' || first == 'Y' || first == 'Z' ||
        first == 'A' || first == 'B' || first == 'C' || first == 'I' ||
        first == 'J' || first == 'K' || first == 'F' || first == 'S' ||
        first == 'R') {
      return false;
    }
  }
  if (word.head == "RET" || word.head == "RTLION" || word.head == "RTLIOF") {
    return false;
  }
  return true;
}

std::string subprogramTargetFromWord(const Word &word) {
  return toUpper(word.text);
}

struct InlineParenSuffixInfo {
  Location location;
  bool empty = false;
};

std::optional<InlineParenSuffixInfo>
findInlineParenSuffixComment(const Line &line, const Word &anchor) {
  bool anchor_seen = false;
  for (const auto &item : line.items) {
    if (std::holds_alternative<Word>(item)) {
      const auto &word = std::get<Word>(item);
      if (!anchor_seen) {
        if (&word == &anchor) {
          anchor_seen = true;
        }
        continue;
      }
      // Suffix matching only applies if no additional word appears first.
      return std::nullopt;
    }
    if (!anchor_seen) {
      continue;
    }
    if (!std::holds_alternative<Comment>(item)) {
      continue;
    }
    const auto &comment = std::get<Comment>(item);
    if (comment.location.line != anchor.location.line ||
        comment.text.size() < 2 || comment.text.front() != '(' ||
        comment.text.back() != ')') {
      return std::nullopt;
    }
    InlineParenSuffixInfo info;
    info.location = comment.location;
    info.empty = comment.text == "()";
    return info;
  }
  return std::nullopt;
}

struct SubprogramDeclarationMatch {
  AilLabelInstruction instruction;
  std::optional<InlineParenSuffixInfo> inline_signature;
};

struct SubprogramCallMatch {
  AilSubprogramCallInstruction instruction;
  std::optional<InlineParenSuffixInfo> inline_argument;
};

std::optional<SubprogramDeclarationMatch>
subprogramDeclarationFromLine(const Line &line, const SourceInfo &source) {
  std::vector<const Word *> words;
  words.reserve(line.items.size());
  for (const auto &item : line.items) {
    if (!std::holds_alternative<Word>(item)) {
      continue;
    }
    words.push_back(&std::get<Word>(item));
  }
  if (words.size() != 2) {
    return std::nullopt;
  }
  if (words[0]->head != "PROC" || words[0]->value.has_value() ||
      words[0]->has_equal) {
    return std::nullopt;
  }
  if (!isSubprogramTargetWord(*words[1])) {
    return std::nullopt;
  }
  SubprogramDeclarationMatch match;
  match.instruction.source = source;
  match.instruction.name = subprogramTargetFromWord(*words[1]);
  match.inline_signature = findInlineParenSuffixComment(line, *words[1]);
  return match;
}

std::optional<Location> malformedProcDeclarationFromLine(const Line &line) {
  std::vector<const Word *> words;
  words.reserve(line.items.size());
  for (const auto &item : line.items) {
    if (!std::holds_alternative<Word>(item)) {
      continue;
    }
    words.push_back(&std::get<Word>(item));
  }
  if (words.empty() || words[0]->head != "PROC") {
    return std::nullopt;
  }
  if (words[0]->value.has_value() || words[0]->has_equal) {
    return words[0]->location;
  }
  if (words.size() == 1) {
    return words[0]->location;
  }
  if (!isSubprogramTargetWord(*words[1])) {
    return words[1]->location;
  }
  if (words.size() == 2) {
    return std::nullopt;
  }
  return words[2]->location;
}

std::optional<SubprogramCallMatch>
subprogramCallFromLine(const Line &line, const SourceInfo &source,
                       const LowerOptions &options) {
  std::vector<const Word *> words;
  words.reserve(line.items.size());
  for (const auto &item : line.items) {
    if (!std::holds_alternative<Word>(item)) {
      continue;
    }
    words.push_back(&std::get<Word>(item));
  }

  if (words.size() == 1) {
    if (!isSubprogramTargetWord(*words.front())) {
      return std::nullopt;
    }
    SubprogramCallMatch match;
    match.instruction.source = source;
    match.instruction.target = subprogramTargetFromWord(*words.front());
    match.inline_argument = findInlineParenSuffixComment(line, *words.front());
    return match;
  }

  if (words.size() != 2) {
    return std::nullopt;
  }

  const auto is_m98_word = [](const Word &word) {
    return word.head == "M" && !word.has_equal && word.value.has_value() &&
           *word.value == "98";
  };
  if (options.enable_iso_m98_calls && is_m98_word(*words[0]) &&
      words[1]->head == "P" && words[1]->value.has_value()) {
    SubprogramCallMatch match;
    match.instruction.source = source;
    match.instruction.target = toUpper(*words[1]->value);
    return match;
  }

  SubprogramCallMatch match;
  match.instruction.source = source;

  const auto first_repeat = parseSubprogramRepeatCount(*words[0]);
  const auto second_repeat = parseSubprogramRepeatCount(*words[1]);
  if (first_repeat.has_value() && isSubprogramTargetWord(*words[1])) {
    match.instruction.repeat_count = *first_repeat;
    match.instruction.target = subprogramTargetFromWord(*words[1]);
    match.inline_argument = findInlineParenSuffixComment(line, *words[1]);
    return match;
  }
  if (second_repeat.has_value() && isSubprogramTargetWord(*words[0])) {
    match.instruction.repeat_count = *second_repeat;
    match.instruction.target = subprogramTargetFromWord(*words[0]);
    match.inline_argument = findInlineParenSuffixComment(line, *words[0]);
    return match;
  }
  return std::nullopt;
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

  struct IfLowerContext {
    size_t branch_index = 0;
    std::string end_label;
    bool has_else = false;
  };

  std::vector<IfLowerContext> if_stack;
  int generated_label_counter = 0;
  std::optional<RapidInterpolationMode> current_rapid_mode;
  WorkingPlane current_working_plane = WorkingPlane::XY;
  const auto makeInternalLabel = [&](const std::string &prefix) {
    ++generated_label_counter;
    return "__CF_" + prefix + "_" + std::to_string(generated_label_counter);
  };

  const auto emitInternalLabel = [&](const std::string &name,
                                     const SourceInfo &source) {
    AilLabelInstruction label;
    label.source = source;
    label.name = name;
    result.instructions.push_back(std::move(label));
  };

  for (const auto &line : program.lines) {
    if (stop_line != 0 && line.line_index >= stop_line) {
      break;
    }
    if (shouldSkipLine(line, options)) {
      continue;
    }
    if (lineHasError(result.diagnostics, line.line_index)) {
      continue;
    }
    if (line.assignment.has_value()) {
      if (toUpper(line.assignment->lhs) == "PROC") {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Error;
        diag.message = "malformed PROC declaration; expected PROC <name>";
        diag.location = line.assignment->location;
        result.diagnostics.push_back(std::move(diag));
        continue;
      }
      if (const auto tool_select = toolSelectFromAssignment(
              *line.assignment, sourceFromLine(line, options), options);
          tool_select.has_value()) {
        result.instructions.push_back(*tool_select);
        continue;
      }
      AilAssignInstruction inst;
      inst.source = sourceFromLine(line, options);
      inst.lhs = line.assignment->lhs;
      inst.rhs = line.assignment->rhs;
      result.instructions.push_back(std::move(inst));
      continue;
    }
    if (line.if_block_start_statement.has_value()) {
      const auto source = sourceFromLine(line, options);

      AilBranchIfInstruction branch;
      branch.source = source;
      branch.condition = line.if_block_start_statement->condition;
      branch.then_branch.source = source;
      branch.then_branch.opcode = "GOTO";
      branch.then_branch.target = makeInternalLabel("IF_THEN");
      branch.then_branch.target_kind = "label";

      AilGotoInstruction else_branch;
      else_branch.source = source;
      else_branch.opcode = "GOTO";
      else_branch.target = makeInternalLabel("IF_END");
      else_branch.target_kind = "label";
      branch.else_branch = std::move(else_branch);

      const size_t branch_index = result.instructions.size();
      const std::string then_label = branch.then_branch.target;
      const std::string end_label = branch.else_branch->target;
      result.instructions.push_back(std::move(branch));
      emitInternalLabel(then_label, source);

      IfLowerContext ctx;
      ctx.branch_index = branch_index;
      ctx.end_label = end_label;
      if_stack.push_back(std::move(ctx));
      continue;
    }
    if (line.else_statement.has_value()) {
      const auto source = sourceFromLine(line, options);
      if (if_stack.empty()) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Error;
        diag.message = "ELSE without matching IF";
        diag.location = line.else_statement->keyword_location;
        result.diagnostics.push_back(std::move(diag));
        continue;
      }
      auto &ctx = if_stack.back();
      if (ctx.has_else) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Error;
        diag.message = "duplicate ELSE for IF block";
        diag.location = line.else_statement->keyword_location;
        result.diagnostics.push_back(std::move(diag));
        continue;
      }

      AilGotoInstruction skip_else;
      skip_else.source = source;
      skip_else.opcode = "GOTO";
      skip_else.target = ctx.end_label;
      skip_else.target_kind = "label";
      result.instructions.push_back(std::move(skip_else));

      const std::string else_label = makeInternalLabel("IF_ELSE");
      auto &branch_inst = std::get<AilBranchIfInstruction>(
          result.instructions[ctx.branch_index]);
      if (branch_inst.else_branch.has_value()) {
        branch_inst.else_branch->target = else_label;
      }
      emitInternalLabel(else_label, source);
      ctx.has_else = true;
      continue;
    }
    if (line.endif_statement.has_value()) {
      const auto source = sourceFromLine(line, options);
      if (if_stack.empty()) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Error;
        diag.message = "ENDIF without matching IF";
        diag.location = line.endif_statement->keyword_location;
        result.diagnostics.push_back(std::move(diag));
        continue;
      }
      const auto ctx = if_stack.back();
      if_stack.pop_back();
      emitInternalLabel(ctx.end_label, source);
      continue;
    }
    if (line.label_definition.has_value()) {
      AilLabelInstruction inst;
      inst.source = sourceFromLine(line, options);
      inst.name = line.label_definition->name;
      result.instructions.push_back(std::move(inst));
      continue;
    }
    if (line.goto_statement.has_value()) {
      AilGotoInstruction inst;
      inst.source = sourceFromLine(line, options);
      inst.opcode = line.goto_statement->opcode;
      inst.target = line.goto_statement->target;
      inst.target_kind = line.goto_statement->target_kind;
      result.instructions.push_back(std::move(inst));
      continue;
    }
    if (line.if_goto_statement.has_value()) {
      AilBranchIfInstruction inst;
      inst.source = sourceFromLine(line, options);
      inst.condition = line.if_goto_statement->condition;
      inst.then_branch.source = inst.source;
      inst.then_branch.opcode = line.if_goto_statement->then_branch.opcode;
      inst.then_branch.target = line.if_goto_statement->then_branch.target;
      inst.then_branch.target_kind =
          line.if_goto_statement->then_branch.target_kind;
      if (line.if_goto_statement->else_branch.has_value()) {
        AilGotoInstruction else_branch;
        else_branch.source = inst.source;
        else_branch.opcode = line.if_goto_statement->else_branch->opcode;
        else_branch.target = line.if_goto_statement->else_branch->target;
        else_branch.target_kind =
            line.if_goto_statement->else_branch->target_kind;
        inst.else_branch = std::move(else_branch);
      }
      result.instructions.push_back(std::move(inst));
      continue;
    }
    const auto source = sourceFromLine(line, options);
    if (const auto malformed_proc = malformedProcDeclarationFromLine(line);
        malformed_proc.has_value()) {
      Diagnostic diag;
      diag.severity = Diagnostic::Severity::Error;
      diag.message = "malformed PROC declaration; expected PROC <name>";
      diag.location = *malformed_proc;
      result.diagnostics.push_back(std::move(diag));
      continue;
    }
    if (const auto decl = subprogramDeclarationFromLine(line, source);
        decl.has_value()) {
      result.instructions.push_back(decl->instruction);
      if (decl->inline_signature.has_value() &&
          !decl->inline_signature->empty) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Warning;
        diag.message =
            "PROC signature parameters are not supported yet; inline "
            "parenthesized suffix is ignored";
        diag.location = decl->inline_signature->location;
        result.diagnostics.push_back(std::move(diag));
      }
      continue;
    }
    if (const auto call = subprogramCallFromLine(line, source, options);
        call.has_value()) {
      result.instructions.push_back(call->instruction);
      if (call->inline_argument.has_value() && !call->inline_argument->empty) {
        Diagnostic diag;
        diag.severity = Diagnostic::Severity::Warning;
        diag.message =
            "subprogram call arguments are not supported yet; inline "
            "parenthesized suffix is ignored";
        diag.location = call->inline_argument->location;
        result.diagnostics.push_back(std::move(diag));
      }
      continue;
    }
    for (const auto &item : line.items) {
      if (!std::holds_alternative<Word>(item)) {
        continue;
      }
      const auto &word = std::get<Word>(item);
      const auto rapid_mode = rapidTraverseModeFromWord(word, source);
      if (rapid_mode.has_value()) {
        current_rapid_mode = rapid_mode->mode;
        result.instructions.push_back(*rapid_mode);
      }
      const auto tool_radius_comp = toolRadiusCompFromWord(word, source);
      if (tool_radius_comp.has_value()) {
        result.instructions.push_back(*tool_radius_comp);
      }
      const auto working_plane = workingPlaneFromWord(word, source);
      if (working_plane.has_value()) {
        current_working_plane = working_plane->plane;
        result.instructions.push_back(*working_plane);
      }
      const auto tool_select = toolSelectFromWord(word, source, options);
      if (tool_select.has_value()) {
        result.instructions.push_back(*tool_select);
      }
      const auto return_boundary = returnBoundaryFromWord(word, source);
      if (return_boundary.has_value()) {
        result.instructions.push_back(*return_boundary);
        continue;
      }
      const auto mcode = mCodeFromWord(word, source);
      if (!mcode.has_value()) {
        continue;
      }
      const auto return_boundary_mcode = returnBoundaryFromMCode(*mcode);
      if (return_boundary_mcode.has_value()) {
        result.instructions.push_back(*return_boundary_mcode);
        continue;
      }
      auto tool_change = toolChangeFromMCode(*mcode);
      if (tool_change.has_value()) {
        tool_change->timing = timingFromOptions(options);
        result.instructions.push_back(std::move(*tool_change));
        continue;
      }
      result.instructions.push_back(*mcode);
    }
    const auto found = message_by_line.find(line.line_index);
    if (found != message_by_line.end()) {
      auto lowered_inst = found->second;
      if (std::holds_alternative<AilLinearMoveInstruction>(lowered_inst)) {
        auto &linear = std::get<AilLinearMoveInstruction>(lowered_inst);
        if (linear.opcode == "G0") {
          linear.rapid_mode_effective = current_rapid_mode;
        }
      } else if (std::holds_alternative<AilArcMoveInstruction>(lowered_inst)) {
        auto &arc = std::get<AilArcMoveInstruction>(lowered_inst);
        arc.plane_effective = current_working_plane;
      }
      result.instructions.push_back(std::move(lowered_inst));
    }
  }

  for (const auto &ctx : if_stack) {
    Diagnostic diag;
    diag.severity = Diagnostic::Severity::Error;
    diag.message = "missing ENDIF for IF block";
    const auto &branch =
        std::get<AilBranchIfInstruction>(result.instructions[ctx.branch_index]);
    diag.location.line = branch.source.line;
    diag.location.column = 1;
    result.diagnostics.push_back(std::move(diag));
  }
  return result;
}

AilResult parseAndLowerAil(std::string_view input,
                           const LowerOptions &options) {
  ParseOptions parse_options;
  parse_options.enable_iso_m98_calls = options.enable_iso_m98_calls;
  const auto parsed = parse(input, parse_options);
  return lowerToAil(parsed.program, parsed.diagnostics, options);
}

AilExecutor::AilExecutor(std::vector<AilInstruction> instructions,
                         AilExecutorOptions options)
    : instructions_(std::move(instructions)), options_(std::move(options)) {
  for (size_t i = 0; i < instructions_.size(); ++i) {
    const auto &inst = instructions_[i];
    std::visit(
        [i, this](const auto &node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, AilLabelInstruction>) {
            label_positions_[node.name].push_back(i);
          }
          if (node.source.line_number.has_value()) {
            line_number_positions_[*node.source.line_number].push_back(i);
          }
        },
        inst);
  }
}

void AilExecutor::notifyEvent(const WaitToken &wait_token) {
  pending_events_.insert(wait_token);
}

void AilExecutor::addFault(const SourceInfo &source,
                           const std::string &message) {
  state_.status = ExecutorStatus::Fault;
  state_.fault_message = message;
  Diagnostic diag;
  diag.severity = Diagnostic::Severity::Error;
  diag.message = message;
  diag.location.line = source.line;
  diag.location.column = 1;
  diagnostics_.push_back(std::move(diag));
}

void AilExecutor::addWarning(const SourceInfo &source,
                             const std::string &message) {
  Diagnostic diag;
  diag.severity = Diagnostic::Severity::Warning;
  diag.message = message;
  diag.location.line = source.line;
  diag.location.column = 1;
  diagnostics_.push_back(std::move(diag));
}

std::optional<size_t>
AilExecutor::resolveGotoTarget(size_t current_index,
                               const AilGotoInstruction &inst) {
  std::vector<size_t> candidates;
  if (inst.target_kind == "label") {
    auto it = label_positions_.find(inst.target);
    if (it != label_positions_.end()) {
      candidates = it->second;
    }
  } else if (inst.target_kind == "line_number" ||
             inst.target_kind == "number") {
    int line_number = 0;
    try {
      if (inst.target_kind == "line_number" && !inst.target.empty() &&
          (inst.target[0] == 'N' || inst.target[0] == 'n')) {
        line_number = std::stoi(inst.target.substr(1));
      } else {
        line_number = std::stoi(inst.target);
      }
    } catch (...) {
      return std::nullopt;
    }
    auto it = line_number_positions_.find(line_number);
    if (it != line_number_positions_.end()) {
      candidates = it->second;
    }
  } else {
    return std::nullopt;
  }

  if (candidates.empty()) {
    return std::nullopt;
  }

  auto pick_forward = [&]() -> std::optional<size_t> {
    size_t best = std::numeric_limits<size_t>::max();
    for (size_t idx : candidates) {
      if (idx > current_index && idx < best) {
        best = idx;
      }
    }
    if (best == std::numeric_limits<size_t>::max()) {
      return std::nullopt;
    }
    return best;
  };

  auto pick_backward = [&]() -> std::optional<size_t> {
    size_t best = 0;
    bool found = false;
    for (size_t idx : candidates) {
      if (idx < current_index && (!found || idx > best)) {
        best = idx;
        found = true;
      }
    }
    if (!found) {
      return std::nullopt;
    }
    return best;
  };

  if (inst.opcode == "GOTOF") {
    size_t count = 0;
    for (size_t idx : candidates) {
      if (idx > current_index) {
        ++count;
      }
    }
    if (count > 1 &&
        (inst.target_kind == "line_number" || inst.target_kind == "number")) {
      addWarning(inst.source, "multiple forward blocks match target " +
                                  inst.target +
                                  "; using nearest forward block");
    }
    return pick_forward();
  }
  if (inst.opcode == "GOTOB") {
    size_t count = 0;
    for (size_t idx : candidates) {
      if (idx < current_index) {
        ++count;
      }
    }
    if (count > 1 &&
        (inst.target_kind == "line_number" || inst.target_kind == "number")) {
      addWarning(inst.source, "multiple backward blocks match target " +
                                  inst.target +
                                  "; using nearest backward block");
    }
    return pick_backward();
  }
  if (inst.opcode == "GOTO" || inst.opcode == "GOTOC") {
    auto forward = pick_forward();
    if (forward.has_value()) {
      size_t count = 0;
      for (size_t idx : candidates) {
        if (idx > current_index) {
          ++count;
        }
      }
      if (count > 1 &&
          (inst.target_kind == "line_number" || inst.target_kind == "number")) {
        addWarning(inst.source, "multiple forward blocks match target " +
                                    inst.target +
                                    "; using nearest forward block");
      }
    } else {
      size_t count = 0;
      for (size_t idx : candidates) {
        if (idx < current_index) {
          ++count;
        }
      }
      if (count > 1 &&
          (inst.target_kind == "line_number" || inst.target_kind == "number")) {
        addWarning(inst.source, "multiple backward blocks match target " +
                                    inst.target +
                                    "; using nearest backward block");
      }
    }
    if (forward.has_value()) {
      return forward;
    }
    return pick_backward();
  }
  return std::nullopt;
}

bool AilExecutor::evaluateBranchAtPc(int64_t now_ms,
                                     const IConditionResolver &resolver) {
  const auto &branch =
      std::get<AilBranchIfInstruction>(instructions_[state_.pc]);
  const auto resolved = resolver.resolve(branch.condition, branch.source);
  if (resolved.kind == ConditionResolutionKind::Pending) {
    state_.status = ExecutorStatus::BlockedOnCondition;
    ExecutorBlockedState blocked;
    blocked.instruction_index = state_.pc;
    blocked.wait_token = resolved.wait_token;
    blocked.retry_at_ms = resolved.retry_at_ms;
    state_.blocked = std::move(blocked);
    (void)now_ms;
    return true;
  }
  if (resolved.kind == ConditionResolutionKind::Error) {
    addFault(branch.source, resolved.error_message.value_or(
                                "condition evaluation failed at runtime"));
    return true;
  }

  bool take_then = resolved.kind == ConditionResolutionKind::True;
  if (take_then) {
    auto target = resolveGotoTarget(state_.pc, branch.then_branch);
    if (!target.has_value()) {
      if (branch.then_branch.opcode == "GOTOC") {
        ++state_.pc;
        return true;
      }
      addFault(branch.source,
               "unresolved branch target: " + branch.then_branch.target);
      return true;
    }
    state_.pc = *target;
    return true;
  }

  if (!branch.else_branch.has_value()) {
    ++state_.pc;
    return true;
  }
  auto target = resolveGotoTarget(state_.pc, *branch.else_branch);
  if (!target.has_value()) {
    if (branch.else_branch->opcode == "GOTOC") {
      ++state_.pc;
      return true;
    }
    addFault(branch.source,
             "unresolved branch target: " + branch.else_branch->target);
    return true;
  }
  state_.pc = *target;
  return true;
}

bool AilExecutor::handleMCodeAtPc() {
  const auto &inst = std::get<AilMCodeInstruction>(instructions_[state_.pc]);
  if (isKnownPredefinedMFunction(inst.value)) {
    ++state_.pc;
    return true;
  }

  const std::string m_text = "M" + std::to_string(inst.value);
  if (options_.unknown_mcode_policy == ErrorPolicy::Error) {
    addFault(inst.source, "unsupported M function: " + m_text);
    return true;
  }
  if (options_.unknown_mcode_policy == ErrorPolicy::Warning) {
    addWarning(inst.source,
               "unsupported M function ignored by policy: " + m_text);
  }
  ++state_.pc;
  return true;
}

bool AilExecutor::handleToolSelectAtPc() {
  const auto &inst =
      std::get<AilToolSelectInstruction>(instructions_[state_.pc]);
  ToolSelectionState selection;
  selection.selector_index = inst.selector_index;
  selection.selector_value = inst.selector_value;

  if (inst.timing == ToolActionTiming::Immediate) {
    const auto resolved =
        options_.tool_selection_resolver
            ? options_.tool_selection_resolver(selection)
            : defaultResolveToolSelection(selection, options_);
    if (resolved.kind == ToolSelectionResolutionKind::Resolved) {
      if (resolved.substituted) {
        const std::string suffix =
            resolved.message.empty() ? "" : " (" + resolved.message + ")";
        addWarning(inst.source,
                   "tool selection substituted: " + selection.selector_value +
                       " -> " + resolved.selection.selector_value + suffix);
      }
      if (isDeselectSelector(resolved.selection)) {
        state_.active_tool_selection.reset();
      } else {
        state_.active_tool_selection = resolved.selection;
      }
    } else {
      const bool unresolved =
          resolved.kind == ToolSelectionResolutionKind::Unresolved;
      const ErrorPolicy policy = unresolved ? options_.unresolved_tool_policy
                                            : options_.ambiguous_tool_policy;
      const std::string reason =
          unresolved ? "tool selection unresolved" : "tool selection ambiguous";
      const std::string detail =
          resolved.message.empty() ? reason : reason + ": " + resolved.message;
      if (policy == ErrorPolicy::Error) {
        addFault(inst.source, detail);
      } else if (policy == ErrorPolicy::Warning) {
        addWarning(inst.source, detail + "; ignored by policy");
      }
    }
    state_.pending_tool_selection.reset();
    ++state_.pc;
    return true;
  }

  // Deferred mode keeps only the most recent selection before M6.
  state_.pending_tool_selection = std::move(selection);
  ++state_.pc;
  return true;
}

bool AilExecutor::handleToolChangeAtPc() {
  const auto &inst =
      std::get<AilToolChangeInstruction>(instructions_[state_.pc]);
  if (state_.pending_tool_selection.has_value()) {
    const ToolSelectionState pending = *state_.pending_tool_selection;
    const auto resolved = options_.tool_selection_resolver
                              ? options_.tool_selection_resolver(pending)
                              : defaultResolveToolSelection(pending, options_);
    if (resolved.kind == ToolSelectionResolutionKind::Resolved) {
      if (resolved.substituted) {
        const std::string suffix =
            resolved.message.empty() ? "" : " (" + resolved.message + ")";
        addWarning(inst.source,
                   "tool selection substituted: " + pending.selector_value +
                       " -> " + resolved.selection.selector_value + suffix);
      }
      if (isDeselectSelector(resolved.selection)) {
        state_.active_tool_selection.reset();
      } else {
        state_.active_tool_selection = resolved.selection;
      }
    } else {
      const bool unresolved =
          resolved.kind == ToolSelectionResolutionKind::Unresolved;
      const ErrorPolicy policy = unresolved ? options_.unresolved_tool_policy
                                            : options_.ambiguous_tool_policy;
      const std::string reason =
          unresolved ? "tool selection unresolved" : "tool selection ambiguous";
      const std::string detail =
          resolved.message.empty() ? reason : reason + ": " + resolved.message;
      if (policy == ErrorPolicy::Error) {
        state_.pending_tool_selection.reset();
        addFault(inst.source, detail);
        return true;
      }
      if (policy == ErrorPolicy::Warning) {
        addWarning(inst.source, detail + "; ignored by policy");
      }
    }
    state_.pending_tool_selection.reset();
    ++state_.pc;
    return true;
  }

  const std::string message = "M6 requested with no pending tool selection";
  if (options_.m6_without_pending_policy == ErrorPolicy::Error) {
    addFault(inst.source, message);
    return true;
  }
  if (options_.m6_without_pending_policy == ErrorPolicy::Warning) {
    addWarning(inst.source, message + "; ignored by policy");
  }
  ++state_.pc;
  return true;
}

bool AilExecutor::advanceOneInstruction(int64_t now_ms,
                                        const IConditionResolver &resolver) {
  if (state_.pc >= instructions_.size()) {
    state_.status = ExecutorStatus::Completed;
    return true;
  }

  const auto &inst = instructions_[state_.pc];
  if (std::holds_alternative<AilGotoInstruction>(inst)) {
    const auto &goto_inst = std::get<AilGotoInstruction>(inst);
    auto target = resolveGotoTarget(state_.pc, goto_inst);
    if (!target.has_value()) {
      if (goto_inst.opcode == "GOTOC") {
        ++state_.pc;
        return true;
      }
      addFault(goto_inst.source, "unresolved goto target: " + goto_inst.target);
      return true;
    }
    state_.pc = *target;
    return true;
  }
  if (std::holds_alternative<AilBranchIfInstruction>(inst)) {
    return evaluateBranchAtPc(now_ms, resolver);
  }
  if (std::holds_alternative<AilMCodeInstruction>(inst)) {
    return handleMCodeAtPc();
  }
  if (std::holds_alternative<AilRapidTraverseModeInstruction>(inst)) {
    const auto &rapid = std::get<AilRapidTraverseModeInstruction>(inst);
    state_.rapid_mode_current = rapid.mode;
    ++state_.pc;
    return true;
  }
  if (std::holds_alternative<AilToolRadiusCompInstruction>(inst)) {
    const auto &comp = std::get<AilToolRadiusCompInstruction>(inst);
    state_.tool_radius_comp_current = comp.mode;
    ++state_.pc;
    return true;
  }
  if (std::holds_alternative<AilWorkingPlaneInstruction>(inst)) {
    const auto &plane = std::get<AilWorkingPlaneInstruction>(inst);
    state_.working_plane_current = plane.plane;
    ++state_.pc;
    return true;
  }
  if (std::holds_alternative<AilToolSelectInstruction>(inst)) {
    return handleToolSelectAtPc();
  }
  if (std::holds_alternative<AilToolChangeInstruction>(inst)) {
    return handleToolChangeAtPc();
  }
  if (std::holds_alternative<AilReturnBoundaryInstruction>(inst)) {
    const auto &ret = std::get<AilReturnBoundaryInstruction>(inst);
    if (call_stack_frames_.empty()) {
      addFault(ret.source,
               "return boundary encountered with empty call stack: " +
                   ret.opcode);
      return true;
    }
    auto &frame = call_stack_frames_.back();
    if (frame.remaining_repeats > 1) {
      --frame.remaining_repeats;
      state_.pc = frame.target_pc;
      return true;
    }
    state_.pc = frame.return_pc;
    call_stack_frames_.pop_back();
    state_.call_stack_depth = call_stack_frames_.size();
    return true;
  }
  if (std::holds_alternative<AilSubprogramCallInstruction>(inst)) {
    const auto &call = std::get<AilSubprogramCallInstruction>(inst);
    const auto resolution =
        options_.subprogram_target_resolver
            ? options_.subprogram_target_resolver(call.target, label_positions_)
            : defaultResolveSubprogramTarget(call.target, label_positions_,
                                             options_);
    if (!resolution.resolved) {
      const std::string message =
          "unresolved subprogram target: " + call.target;
      if (options_.unresolved_subprogram_policy == ErrorPolicy::Error) {
        addFault(call.source, message);
        return true;
      }
      if (options_.unresolved_subprogram_policy == ErrorPolicy::Warning) {
        addWarning(call.source, message + "; ignored by policy");
      }
      ++state_.pc;
      return true;
    }
    const auto it = label_positions_.find(resolution.resolved_target);
    if (it == label_positions_.end() || it->second.empty()) {
      addFault(call.source, "subprogram policy resolved missing target: " +
                                resolution.resolved_target);
      return true;
    }
    if (!resolution.message.empty()) {
      addWarning(call.source, resolution.message);
    }
    if (it->second.size() > 1) {
      addWarning(call.source, "duplicate subprogram target labels for " +
                                  resolution.resolved_target +
                                  "; using first definition");
    }
    const int64_t repeat_count = call.repeat_count.value_or(1);
    if (repeat_count <= 0) {
      addWarning(call.source,
                 "subprogram repeat count <= 0 ignored for target " +
                     call.target);
      ++state_.pc;
      return true;
    }
    SubprogramCallFrame frame;
    frame.return_pc = state_.pc + 1;
    frame.target_pc = it->second.front();
    frame.remaining_repeats = repeat_count;
    call_stack_frames_.push_back(frame);
    state_.call_stack_depth = call_stack_frames_.size();
    state_.pc = frame.target_pc;
    return true;
  }
  ++state_.pc;
  return true;
}

bool AilExecutor::step(int64_t now_ms, const ConditionResolver &resolver) {
  FunctionConditionResolver adapter(resolver);
  return step(now_ms, adapter);
}

bool AilExecutor::step(int64_t now_ms, const IExecutionRuntime &runtime) {
  return step(now_ms, static_cast<const IConditionResolver &>(runtime));
}

bool AilExecutor::step(int64_t now_ms, const IConditionResolver &resolver) {
  if (state_.status == ExecutorStatus::Fault ||
      state_.status == ExecutorStatus::Completed) {
    return false;
  }

  if (state_.status == ExecutorStatus::BlockedOnCondition &&
      state_.blocked.has_value()) {
    bool event_ready = false;
    if (state_.blocked->wait_token.has_value()) {
      const auto it = pending_events_.find(*state_.blocked->wait_token);
      if (it != pending_events_.end()) {
        pending_events_.erase(it);
        event_ready = true;
      }
    }
    const bool time_ready = state_.blocked->retry_at_ms.has_value() &&
                            now_ms >= *state_.blocked->retry_at_ms;
    if (!event_ready && !time_ready) {
      return false;
    }
    state_.status = ExecutorStatus::Ready;
    state_.pc = state_.blocked->instruction_index;
    state_.blocked.reset();
  }

  return advanceOneInstruction(now_ms, resolver);
}

} // namespace gcode
