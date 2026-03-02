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

struct Condition {
  std::shared_ptr<ExprNode> lhs;
  std::string op;
  std::shared_ptr<ExprNode> rhs;
  std::string raw_text;
  bool has_logical_and = false;
  std::vector<std::string> and_terms_raw;
  Location location;
};

struct LabelDefinition {
  std::string name;
  Location location;
};

struct GotoStatement {
  std::string opcode;
  std::string target;
  std::string target_kind;
  Location keyword_location;
  Location target_location;
};

struct IfGotoStatement {
  Condition condition;
  GotoStatement then_branch;
  std::optional<GotoStatement> else_branch;
  Location keyword_location;
};

struct IfBlockStartStatement {
  Condition condition;
  Location keyword_location;
};

struct ElseStatement {
  Location keyword_location;
};

struct EndIfStatement {
  Location keyword_location;
};

struct WhileStatement {
  Condition condition;
  Location keyword_location;
};

struct EndWhileStatement {
  Location keyword_location;
};

struct ForStatement {
  std::string variable;
  std::shared_ptr<ExprNode> start;
  std::shared_ptr<ExprNode> end;
  Location keyword_location;
};

struct EndForStatement {
  Location keyword_location;
};

struct RepeatStatement {
  Location keyword_location;
};

struct UntilStatement {
  Condition condition;
  Location keyword_location;
};

struct LoopStatement {
  Location keyword_location;
};

struct EndLoopStatement {
  Location keyword_location;
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
  std::optional<LabelDefinition> label_definition;
  std::optional<GotoStatement> goto_statement;
  std::optional<IfGotoStatement> if_goto_statement;
  std::optional<IfBlockStartStatement> if_block_start_statement;
  std::optional<ElseStatement> else_statement;
  std::optional<EndIfStatement> endif_statement;
  std::optional<WhileStatement> while_statement;
  std::optional<EndWhileStatement> endwhile_statement;
  std::optional<ForStatement> for_statement;
  std::optional<EndForStatement> endfor_statement;
  std::optional<RepeatStatement> repeat_statement;
  std::optional<UntilStatement> until_statement;
  std::optional<LoopStatement> loop_statement;
  std::optional<EndLoopStatement> endloop_statement;
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
