#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace gcode {

struct Location {
  int line = 0;
  int column = 0; // 1-based
};

struct Diagnostic {
  enum class Severity { Error, Warning };
  Severity severity = Severity::Error;
  std::string message;
  Location location;
};

struct Word {
  std::string text;
  std::string head;
  std::optional<std::string> value;
  bool has_equal = false;
  Location location;
};

struct Comment {
  std::string text;
  Location location;
};

using LineItem = std::variant<Word, Comment>;

struct ExprNode;

struct ExprLiteral {
  double value = 0.0;
  Location location;
};

struct ExprVariable {
  std::string name;
  bool is_system = false;
  Location location;
};

struct ExprUnary {
  std::string op;
  std::shared_ptr<ExprNode> operand;
  Location location;
};

struct ExprBinary {
  std::string op;
  std::shared_ptr<ExprNode> lhs;
  std::shared_ptr<ExprNode> rhs;
  Location location;
};

struct ExprNode {
  std::variant<ExprLiteral, ExprVariable, ExprUnary, ExprBinary> node;
};

struct Assignment {
  std::string lhs;
  std::shared_ptr<ExprNode> rhs;
  Location location;
};

struct LineNumber {
  int value = 0;
  Location location;
};

struct Line {
  bool block_delete = false;
  std::optional<Location> block_delete_location;
  std::optional<LineNumber> line_number;
  std::vector<LineItem> items;
  std::optional<Assignment> assignment;
  int line_index = 0;
};

struct Program {
  std::vector<Line> lines;
};

struct ParseResult {
  Program program;
  std::vector<Diagnostic> diagnostics;
};

} // namespace gcode
