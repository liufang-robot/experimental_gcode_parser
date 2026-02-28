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
