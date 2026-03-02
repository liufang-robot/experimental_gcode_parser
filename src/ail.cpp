#include "ail.h"

#include <limits>
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

  struct IfLowerContext {
    size_t branch_index = 0;
    std::string end_label;
    bool has_else = false;
  };

  std::vector<IfLowerContext> if_stack;
  int generated_label_counter = 0;
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
    const auto found = message_by_line.find(line.line_index);
    if (found != message_by_line.end()) {
      result.instructions.push_back(found->second);
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
  const auto parsed = parse(input);
  return lowerToAil(parsed.program, parsed.diagnostics, options);
}

AilExecutor::AilExecutor(std::vector<AilInstruction> instructions)
    : instructions_(std::move(instructions)) {
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

void AilExecutor::notifyEvent(const std::string &wait_key) {
  pending_events_.insert(wait_key);
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

std::optional<size_t>
AilExecutor::resolveGotoTarget(size_t current_index,
                               const AilGotoInstruction &inst) const {
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
    return pick_forward();
  }
  if (inst.opcode == "GOTOB") {
    return pick_backward();
  }
  if (inst.opcode == "GOTO" || inst.opcode == "GOTOC") {
    auto forward = pick_forward();
    if (forward.has_value()) {
      return forward;
    }
    return pick_backward();
  }
  return std::nullopt;
}

bool AilExecutor::evaluateBranchAtPc(int64_t now_ms,
                                     const ConditionResolver &resolver) {
  const auto &branch =
      std::get<AilBranchIfInstruction>(instructions_[state_.pc]);
  const auto resolved = resolver(branch.condition, branch.source);
  if (resolved.kind == ConditionResolutionKind::Pending) {
    state_.status = ExecutorStatus::BlockedOnCondition;
    ExecutorBlockedState blocked;
    blocked.instruction_index = state_.pc;
    blocked.wait_key = resolved.wait_key;
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

bool AilExecutor::advanceOneInstruction(int64_t now_ms,
                                        const ConditionResolver &resolver) {
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
  ++state_.pc;
  return true;
}

bool AilExecutor::step(int64_t now_ms, const ConditionResolver &resolver) {
  if (state_.status == ExecutorStatus::Fault ||
      state_.status == ExecutorStatus::Completed) {
    return false;
  }

  if (state_.status == ExecutorStatus::BlockedOnCondition &&
      state_.blocked.has_value()) {
    bool event_ready = false;
    if (state_.blocked->wait_key.has_value()) {
      const auto it = pending_events_.find(*state_.blocked->wait_key);
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
