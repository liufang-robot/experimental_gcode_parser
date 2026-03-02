#include "ast_printer.h"

#include <nlohmann/json.hpp>

#include <sstream>
#include <type_traits>

namespace gcode {
namespace {

void writeLocation(std::ostringstream &out, const Location &loc) {
  out << loc.line << ":" << loc.column;
}

nlohmann::json locationToJson(const Location &loc) {
  nlohmann::json j;
  j["line"] = loc.line;
  j["column"] = loc.column;
  return j;
}

nlohmann::json exprToJson(const std::shared_ptr<ExprNode> &expr) {
  if (!expr) {
    return nullptr;
  }
  return std::visit(
      [](const auto &node) -> nlohmann::json {
        using T = std::decay_t<decltype(node)>;
        nlohmann::json j;
        if constexpr (std::is_same_v<T, ExprLiteral>) {
          j["kind"] = "literal";
          j["value"] = node.value;
          j["location"] = locationToJson(node.location);
        } else if constexpr (std::is_same_v<T, ExprVariable>) {
          j["kind"] = node.is_system ? "system_variable" : "variable";
          j["name"] = node.name;
          j["location"] = locationToJson(node.location);
        } else if constexpr (std::is_same_v<T, ExprUnary>) {
          j["kind"] = "unary";
          j["op"] = node.op;
          j["operand"] = exprToJson(node.operand);
          j["location"] = locationToJson(node.location);
        } else {
          j["kind"] = "binary";
          j["op"] = node.op;
          j["lhs"] = exprToJson(node.lhs);
          j["rhs"] = exprToJson(node.rhs);
          j["location"] = locationToJson(node.location);
        }
        return j;
      },
      expr->node);
}

nlohmann::json conditionToJson(const Condition &condition) {
  nlohmann::json j;
  j["location"] = locationToJson(condition.location);
  j["raw"] = condition.raw_text;
  j["has_logical_and"] = condition.has_logical_and;
  j["and_terms"] = nlohmann::json::array();
  for (const auto &term : condition.and_terms_raw) {
    j["and_terms"].push_back(term);
  }
  j["lhs"] = exprToJson(condition.lhs);
  if (!condition.op.empty()) {
    j["op"] = condition.op;
    j["rhs"] = exprToJson(condition.rhs);
  } else {
    j["op"] = nullptr;
    j["rhs"] = nullptr;
  }
  return j;
}

nlohmann::json lineToJson(const Line &line) {
  nlohmann::json j;
  j["line_index"] = line.line_index;
  j["block_delete"] = line.block_delete;
  if (line.block_delete_location.has_value()) {
    j["block_delete_location"] = locationToJson(*line.block_delete_location);
  } else {
    j["block_delete_location"] = nullptr;
  }

  if (line.line_number.has_value()) {
    nlohmann::json line_number;
    line_number["value"] = line.line_number->value;
    line_number["location"] = locationToJson(line.line_number->location);
    j["line_number"] = line_number;
  } else {
    j["line_number"] = nullptr;
  }

  j["items"] = nlohmann::json::array();
  for (const auto &item : line.items) {
    if (std::holds_alternative<Word>(item)) {
      const auto &word = std::get<Word>(item);
      nlohmann::json ji;
      ji["kind"] = "word";
      ji["raw"] = word.text;
      ji["head"] = word.head;
      if (word.value.has_value()) {
        ji["value"] = *word.value;
      } else {
        ji["value"] = nullptr;
      }
      ji["has_equal"] = word.has_equal;
      ji["location"] = locationToJson(word.location);
      j["items"].push_back(ji);
    } else {
      const auto &comment = std::get<Comment>(item);
      nlohmann::json ji;
      ji["kind"] = "comment";
      ji["raw"] = comment.text;
      ji["location"] = locationToJson(comment.location);
      j["items"].push_back(ji);
    }
  }

  if (line.assignment.has_value()) {
    nlohmann::json assignment;
    assignment["lhs"] = line.assignment->lhs;
    assignment["location"] = locationToJson(line.assignment->location);
    assignment["rhs"] = exprToJson(line.assignment->rhs);
    j["assignment"] = assignment;
  } else {
    j["assignment"] = nullptr;
  }

  if (line.label_definition.has_value()) {
    nlohmann::json label;
    label["name"] = line.label_definition->name;
    label["location"] = locationToJson(line.label_definition->location);
    j["label"] = label;
  }

  if (line.goto_statement.has_value()) {
    nlohmann::json goto_stmt;
    goto_stmt["opcode"] = line.goto_statement->opcode;
    goto_stmt["target"] = line.goto_statement->target;
    goto_stmt["target_kind"] = line.goto_statement->target_kind;
    goto_stmt["keyword_location"] =
        locationToJson(line.goto_statement->keyword_location);
    goto_stmt["target_location"] =
        locationToJson(line.goto_statement->target_location);
    j["goto"] = goto_stmt;
  }

  if (line.if_goto_statement.has_value()) {
    nlohmann::json if_goto;
    if_goto["keyword_location"] =
        locationToJson(line.if_goto_statement->keyword_location);
    if_goto["condition"] = conditionToJson(line.if_goto_statement->condition);
    if_goto["then_opcode"] = line.if_goto_statement->then_branch.opcode;
    if_goto["then_target"] = line.if_goto_statement->then_branch.target;
    if (line.if_goto_statement->else_branch.has_value()) {
      if_goto["else_opcode"] = line.if_goto_statement->else_branch->opcode;
      if_goto["else_target"] = line.if_goto_statement->else_branch->target;
    }
    j["if_goto"] = if_goto;
  }

  if (line.if_block_start_statement.has_value()) {
    nlohmann::json if_block;
    if_block["keyword_location"] =
        locationToJson(line.if_block_start_statement->keyword_location);
    if_block["condition"] =
        conditionToJson(line.if_block_start_statement->condition);
    j["if_block_start"] = if_block;
  }

  if (line.else_statement.has_value()) {
    nlohmann::json else_stmt;
    else_stmt["keyword_location"] =
        locationToJson(line.else_statement->keyword_location);
    j["else"] = else_stmt;
  }

  if (line.endif_statement.has_value()) {
    nlohmann::json endif_stmt;
    endif_stmt["keyword_location"] =
        locationToJson(line.endif_statement->keyword_location);
    j["endif"] = endif_stmt;
  }

  if (line.while_statement.has_value()) {
    nlohmann::json while_stmt;
    while_stmt["keyword_location"] =
        locationToJson(line.while_statement->keyword_location);
    while_stmt["condition"] = conditionToJson(line.while_statement->condition);
    j["while"] = while_stmt;
  }

  if (line.endwhile_statement.has_value()) {
    nlohmann::json endwhile_stmt;
    endwhile_stmt["keyword_location"] =
        locationToJson(line.endwhile_statement->keyword_location);
    j["endwhile"] = endwhile_stmt;
  }

  if (line.for_statement.has_value()) {
    nlohmann::json for_stmt;
    for_stmt["keyword_location"] =
        locationToJson(line.for_statement->keyword_location);
    for_stmt["variable"] = line.for_statement->variable;
    for_stmt["start"] = exprToJson(line.for_statement->start);
    for_stmt["end"] = exprToJson(line.for_statement->end);
    j["for"] = for_stmt;
  }

  if (line.endfor_statement.has_value()) {
    nlohmann::json endfor_stmt;
    endfor_stmt["keyword_location"] =
        locationToJson(line.endfor_statement->keyword_location);
    j["endfor"] = endfor_stmt;
  }

  if (line.repeat_statement.has_value()) {
    nlohmann::json repeat_stmt;
    repeat_stmt["keyword_location"] =
        locationToJson(line.repeat_statement->keyword_location);
    j["repeat"] = repeat_stmt;
  }

  if (line.until_statement.has_value()) {
    nlohmann::json until_stmt;
    until_stmt["keyword_location"] =
        locationToJson(line.until_statement->keyword_location);
    until_stmt["condition"] = conditionToJson(line.until_statement->condition);
    j["until"] = until_stmt;
  }

  if (line.loop_statement.has_value()) {
    nlohmann::json loop_stmt;
    loop_stmt["keyword_location"] =
        locationToJson(line.loop_statement->keyword_location);
    j["loop"] = loop_stmt;
  }

  if (line.endloop_statement.has_value()) {
    nlohmann::json endloop_stmt;
    endloop_stmt["keyword_location"] =
        locationToJson(line.endloop_statement->keyword_location);
    j["endloop"] = endloop_stmt;
  }

  return j;
}

} // namespace

std::string format(const ParseResult &result) {
  std::ostringstream out;
  out << "program\n";
  for (const auto &line : result.program.lines) {
    out << "line " << line.line_index << "\n";
    out << "  block_delete: " << (line.block_delete ? "true" : "false") << "\n";
    out << "  line_number: ";
    if (line.line_number.has_value()) {
      out << line.line_number->value << " at ";
      writeLocation(out, line.line_number->location);
      out << "\n";
    } else {
      out << "none\n";
    }
    for (const auto &item : line.items) {
      if (std::holds_alternative<Word>(item)) {
        const auto &word = std::get<Word>(item);
        out << "  word " << word.head;
        if (word.value.has_value()) {
          out << "=" << *word.value;
        }
        out << " raw=" << word.text << " at ";
        writeLocation(out, word.location);
        if (word.has_equal) {
          out << " eq";
        }
        out << "\n";
      } else {
        const auto &comment = std::get<Comment>(item);
        out << "  comment raw=" << comment.text << " at ";
        writeLocation(out, comment.location);
        out << "\n";
      }
    }
    if (line.assignment.has_value()) {
      out << "  assign " << line.assignment->lhs << " at ";
      writeLocation(out, line.assignment->location);
      out << "\n";
    }
    if (line.label_definition.has_value()) {
      out << "  label " << line.label_definition->name << " at ";
      writeLocation(out, line.label_definition->location);
      out << "\n";
    }
    if (line.goto_statement.has_value()) {
      out << "  " << line.goto_statement->opcode << " "
          << line.goto_statement->target << " ("
          << line.goto_statement->target_kind << ") at ";
      writeLocation(out, line.goto_statement->keyword_location);
      out << "\n";
    }
    if (line.if_goto_statement.has_value()) {
      out << "  if_goto then " << line.if_goto_statement->then_branch.opcode
          << " " << line.if_goto_statement->then_branch.target;
      if (line.if_goto_statement->else_branch.has_value()) {
        out << " else " << line.if_goto_statement->else_branch->opcode << " "
            << line.if_goto_statement->else_branch->target;
      }
      out << " at ";
      writeLocation(out, line.if_goto_statement->keyword_location);
      out << "\n";
    }
    if (line.if_block_start_statement.has_value()) {
      out << "  if_block_start at ";
      writeLocation(out, line.if_block_start_statement->keyword_location);
      out << "\n";
    }
    if (line.else_statement.has_value()) {
      out << "  else at ";
      writeLocation(out, line.else_statement->keyword_location);
      out << "\n";
    }
    if (line.endif_statement.has_value()) {
      out << "  endif at ";
      writeLocation(out, line.endif_statement->keyword_location);
      out << "\n";
    }
    if (line.while_statement.has_value()) {
      out << "  while at ";
      writeLocation(out, line.while_statement->keyword_location);
      out << "\n";
    }
    if (line.endwhile_statement.has_value()) {
      out << "  endwhile at ";
      writeLocation(out, line.endwhile_statement->keyword_location);
      out << "\n";
    }
    if (line.for_statement.has_value()) {
      out << "  for " << line.for_statement->variable << " at ";
      writeLocation(out, line.for_statement->keyword_location);
      out << "\n";
    }
    if (line.endfor_statement.has_value()) {
      out << "  endfor at ";
      writeLocation(out, line.endfor_statement->keyword_location);
      out << "\n";
    }
    if (line.repeat_statement.has_value()) {
      out << "  repeat at ";
      writeLocation(out, line.repeat_statement->keyword_location);
      out << "\n";
    }
    if (line.until_statement.has_value()) {
      out << "  until at ";
      writeLocation(out, line.until_statement->keyword_location);
      out << "\n";
    }
    if (line.loop_statement.has_value()) {
      out << "  loop at ";
      writeLocation(out, line.loop_statement->keyword_location);
      out << "\n";
    }
    if (line.endloop_statement.has_value()) {
      out << "  endloop at ";
      writeLocation(out, line.endloop_statement->keyword_location);
      out << "\n";
    }
  }
  out << "diagnostics\n";
  if (result.diagnostics.empty()) {
    out << "  none\n";
  } else {
    for (const auto &diag : result.diagnostics) {
      out << "  "
          << (diag.severity == Diagnostic::Severity::Error ? "error"
                                                           : "warning")
          << " at ";
      writeLocation(out, diag.location);
      out << ": " << diag.message << "\n";
    }
  }
  return out.str();
}

std::string formatJson(const ParseResult &result, bool pretty) {
  nlohmann::json j;
  j["schema_version"] = 1;
  j["program"] = nlohmann::json::object();
  j["program"]["lines"] = nlohmann::json::array();
  for (const auto &line : result.program.lines) {
    j["program"]["lines"].push_back(lineToJson(line));
  }

  j["diagnostics"] = nlohmann::json::array();
  for (const auto &diag : result.diagnostics) {
    nlohmann::json jd;
    jd["severity"] =
        diag.severity == Diagnostic::Severity::Error ? "error" : "warning";
    jd["message"] = diag.message;
    jd["location"] = locationToJson(diag.location);
    j["diagnostics"].push_back(jd);
  }

  return pretty ? j.dump(2) + "\n" : j.dump();
}

} // namespace gcode
