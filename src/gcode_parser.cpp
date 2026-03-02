#include "gcode_parser.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>

#include "GCodeBaseVisitor.h"
#include "GCodeLexer.h"
#include "GCodeParser.h"
#include "antlr4-runtime.h"
#include "semantic_rules.h"

namespace gcode {
namespace {

Location locationFromToken(const antlr4::Token *token) {
  if (!token) {
    return {};
  }
  return {static_cast<int>(token->getLine()),
          static_cast<int>(token->getCharPositionInLine()) + 1};
}

std::string toUpper(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(),
      [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return value;
}

struct WordParts {
  std::string head;
  std::optional<std::string> value;
  bool has_equal = false;
};

WordParts splitWordText(const std::string &text) {
  WordParts parts;
  auto eq_pos = text.find('=');
  if (eq_pos != std::string::npos) {
    parts.has_equal = true;
    parts.head = text.substr(0, eq_pos);
    if (eq_pos + 1 < text.size()) {
      parts.value = text.substr(eq_pos + 1);
    }
    return parts;
  }

  auto is_value_char = [](char c) {
    return std::isdigit(static_cast<unsigned char>(c)) || c == '+' ||
           c == '-' || c == '.';
  };

  for (size_t i = 1; i < text.size(); ++i) {
    if (is_value_char(text[i])) {
      parts.head = text.substr(0, i);
      parts.value = text.substr(i);
      return parts;
    }
  }

  parts.head = text;
  return parts;
}

class DiagnosticErrorListener : public antlr4::BaseErrorListener {
public:
  explicit DiagnosticErrorListener(std::vector<Diagnostic> *out) : out_(out) {}

  void syntaxError(antlr4::Recognizer * /*recognizer*/,
                   antlr4::Token * /*offendingSymbol*/, size_t line,
                   size_t charPositionInLine, const std::string &msg,
                   std::exception_ptr /*e*/) override {
    if (!out_) {
      return;
    }
    Diagnostic diag;
    diag.severity = Diagnostic::Severity::Error;
    diag.message = buildSyntaxMessage(msg);
    diag.location = {static_cast<int>(line),
                     static_cast<int>(charPositionInLine) + 1};
    out_->push_back(std::move(diag));
  }

private:
  std::string buildSyntaxMessage(const std::string &raw) const {
    std::string message = "syntax error: " + raw;
    if (raw.find("token recognition error") != std::string::npos) {
      message += " (check for unsupported characters or malformed comments)";
    } else if (raw.find("mismatched input") != std::string::npos) {
      message += " (check token order for this line)";
    }
    return message;
  }

  std::vector<Diagnostic> *out_;
};

class AstBuilder : public GCodeBaseVisitor {
public:
  Program program;

  antlrcpp::Any visitProgram(GCodeParser::ProgramContext *ctx) override {
    program.lines.clear();
    for (auto *line_ctx : ctx->line()) {
      auto line_any = visitLine(line_ctx);
      if (auto *line_ptr = std::any_cast<Line>(&line_any)) {
        program.lines.push_back(*line_ptr);
      }
    }
    if (auto *tail_ctx = ctx->line_no_eol()) {
      if (tail_ctx->block_delete() || tail_ctx->line_number() ||
          tail_ctx->statement()) {
        auto line_any = visitLine_no_eol(tail_ctx);
        if (auto *line_ptr = std::any_cast<Line>(&line_any)) {
          program.lines.push_back(*line_ptr);
        }
      }
    }
    return program;
  }

  antlrcpp::Any visitLine(GCodeParser::LineContext *ctx) override {
    return buildLine(ctx->getStart(), ctx->block_delete(), ctx->line_number(),
                     ctx->statement());
  }

  antlrcpp::Any
  visitLine_no_eol(GCodeParser::Line_no_eolContext *ctx) override {
    return buildLine(ctx->getStart(), ctx->block_delete(), ctx->line_number(),
                     ctx->statement());
  }

private:
  static std::string
  jumpOpcodeFromKeyword(GCodeParser::Jump_keywordContext *ctx) {
    if (!ctx) {
      return "";
    }
    if (ctx->GOTOF_KW()) {
      return "GOTOF";
    }
    if (ctx->GOTOB_KW()) {
      return "GOTOB";
    }
    if (ctx->GOTOC_KW()) {
      return "GOTOC";
    }
    return "GOTO";
  }

  static Location jumpKeywordLocation(GCodeParser::Jump_keywordContext *ctx) {
    if (!ctx) {
      return {};
    }
    if (ctx->GOTO_KW()) {
      return locationFromToken(ctx->GOTO_KW()->getSymbol());
    }
    if (ctx->GOTOF_KW()) {
      return locationFromToken(ctx->GOTOF_KW()->getSymbol());
    }
    if (ctx->GOTOB_KW()) {
      return locationFromToken(ctx->GOTOB_KW()->getSymbol());
    }
    if (ctx->GOTOC_KW()) {
      return locationFromToken(ctx->GOTOC_KW()->getSymbol());
    }
    return {};
  }

  Condition buildCondition(GCodeParser::ConditionContext *ctx) {
    Condition condition;
    condition.location = locationFromToken(ctx->getStart());
    condition.raw_text = ctx->getText();
    condition.has_logical_and = !ctx->ops.empty();

    auto append_term = [&](GCodeParser::Condition_termContext *term_ctx) {
      if (!term_ctx) {
        return;
      }
      condition.and_terms_raw.push_back(term_ctx->getText());
      if (term_ctx->lhs && !condition.lhs) {
        condition.lhs = buildExpr(term_ctx->lhs);
        if (term_ctx->rhs) {
          condition.rhs = buildExpr(term_ctx->rhs);
        }
        if (term_ctx->op) {
          condition.op = term_ctx->op->getText();
        }
      }
    };

    append_term(ctx->first);
    for (auto *term_ctx : ctx->rest) {
      append_term(term_ctx);
    }

    if (condition.and_terms_raw.empty()) {
      condition.and_terms_raw.push_back(condition.raw_text);
    }
    return condition;
  }

  GotoStatement buildGotoStatement(GCodeParser::Goto_stmtContext *ctx) {
    GotoStatement stmt;
    stmt.opcode = jumpOpcodeFromKeyword(ctx->jump_keyword());
    stmt.keyword_location = jumpKeywordLocation(ctx->jump_keyword());
    auto *target = ctx->goto_target();
    if (target->WORD()) {
      stmt.target = toUpper(target->WORD()->getText());
      stmt.target_kind = "label";
      stmt.target_location = locationFromToken(target->WORD()->getSymbol());
    } else if (target->LINE_NUMBER()) {
      stmt.target = toUpper(target->LINE_NUMBER()->getText());
      stmt.target_kind = "line_number";
      stmt.target_location =
          locationFromToken(target->LINE_NUMBER()->getSymbol());
    } else if (target->NUMBER()) {
      stmt.target = target->NUMBER()->getText();
      stmt.target_kind = "number";
      stmt.target_location = locationFromToken(target->NUMBER()->getSymbol());
    } else if (target->SYSTEM_VAR()) {
      stmt.target = toUpper(target->SYSTEM_VAR()->getText());
      stmt.target_kind = "system_variable";
      stmt.target_location =
          locationFromToken(target->SYSTEM_VAR()->getSymbol());
    }
    return stmt;
  }

  std::shared_ptr<ExprNode> buildExpr(GCodeParser::ExprContext *ctx) {
    return buildAdditiveExpr(ctx->additive_expr());
  }

  std::shared_ptr<ExprNode>
  buildAdditiveExpr(GCodeParser::Additive_exprContext *ctx) {
    auto node = buildMultiplicativeExpr(ctx->first);
    const int term_count = static_cast<int>(ctx->rest.size());
    for (int i = 0; i < term_count; ++i) {
      const std::string op = ctx->ops[i]->getText();
      auto rhs = buildMultiplicativeExpr(ctx->rest[i]);
      auto binary = std::make_shared<ExprNode>();
      ExprBinary expr;
      expr.op = op;
      expr.lhs = node;
      expr.rhs = rhs;
      expr.location = locationFromToken(ctx->getStart());
      binary->node = std::move(expr);
      node = binary;
    }
    return node;
  }

  std::shared_ptr<ExprNode>
  buildMultiplicativeExpr(GCodeParser::Multiplicative_exprContext *ctx) {
    auto node = buildUnaryExpr(ctx->first);
    const int term_count = static_cast<int>(ctx->rest.size());
    for (int i = 0; i < term_count; ++i) {
      const std::string op = ctx->ops[i]->getText();
      auto rhs = buildUnaryExpr(ctx->rest[i]);
      auto binary = std::make_shared<ExprNode>();
      ExprBinary expr;
      expr.op = op;
      expr.lhs = node;
      expr.rhs = rhs;
      expr.location = locationFromToken(ctx->getStart());
      binary->node = std::move(expr);
      node = binary;
    }
    return node;
  }

  std::shared_ptr<ExprNode>
  buildUnaryExpr(GCodeParser::Unary_exprContext *ctx) {
    if (ctx->primary_expr()) {
      return buildPrimaryExpr(ctx->primary_expr());
    }
    auto unary = std::make_shared<ExprNode>();
    ExprUnary expr;
    expr.op = ctx->op->getText();
    expr.location = locationFromToken(ctx->getStart());
    expr.operand = buildUnaryExpr(ctx->unary_expr());
    unary->node = std::move(expr);
    return unary;
  }

  std::shared_ptr<ExprNode>
  buildPrimaryExpr(GCodeParser::Primary_exprContext *ctx) {
    auto node = std::make_shared<ExprNode>();
    if (ctx->NUMBER()) {
      ExprLiteral literal;
      literal.location = locationFromToken(ctx->NUMBER()->getSymbol());
      try {
        literal.value = std::stod(ctx->NUMBER()->getText());
      } catch (...) {
        literal.value = 0.0;
      }
      node->node = std::move(literal);
      return node;
    }

    ExprVariable variable;
    variable.location = locationFromToken(ctx->getStart());
    if (ctx->SYSTEM_VAR()) {
      variable.name = toUpper(ctx->SYSTEM_VAR()->getText());
      variable.is_system = true;
    } else if (ctx->WORD()) {
      variable.name = toUpper(ctx->WORD()->getText());
    }
    node->node = std::move(variable);
    return node;
  }

  Line buildLine(antlr4::Token *start,
                 GCodeParser::Block_deleteContext *block_ctx,
                 GCodeParser::Line_numberContext *number_ctx,
                 GCodeParser::StatementContext *statement_ctx) {
    Line line;
    if (block_ctx) {
      line.block_delete = true;
      line.block_delete_location = locationFromToken(block_ctx->getStart());
    }
    if (number_ctx) {
      auto *token = number_ctx->LINE_NUMBER()->getSymbol();
      LineNumber number;
      number.location = locationFromToken(token);
      std::string text = token->getText();
      if (text.size() > 1) {
        try {
          number.value = std::stoi(text.substr(1));
        } catch (...) {
          number.value = 0;
        }
      }
      line.line_number = number;
    }
    if (statement_ctx && statement_ctx->assignment_stmt()) {
      auto *assign_ctx = statement_ctx->assignment_stmt();
      Assignment assignment;
      assignment.lhs = toUpper(assign_ctx->WORD()->getText());
      assignment.location = locationFromToken(assign_ctx->WORD()->getSymbol());
      assignment.rhs = buildExpr(assign_ctx->expr());
      line.assignment = std::move(assignment);
    } else if (statement_ctx && statement_ctx->label_stmt()) {
      auto *label_ctx = statement_ctx->label_stmt();
      LabelDefinition label;
      label.name = toUpper(label_ctx->WORD()->getText());
      label.location = locationFromToken(label_ctx->WORD()->getSymbol());
      line.label_definition = std::move(label);
    } else if (statement_ctx && statement_ctx->goto_stmt()) {
      line.goto_statement = buildGotoStatement(statement_ctx->goto_stmt());
    } else if (statement_ctx && statement_ctx->if_goto_stmt()) {
      auto *if_ctx = statement_ctx->if_goto_stmt();
      IfGotoStatement if_stmt;
      if_stmt.keyword_location =
          locationFromToken(if_ctx->IF_KW()->getSymbol());
      if_stmt.condition = buildCondition(if_ctx->condition());
      if_stmt.then_branch = buildGotoStatement(if_ctx->goto_stmt(0));
      if (if_ctx->goto_stmt().size() > 1) {
        if_stmt.else_branch = buildGotoStatement(if_ctx->goto_stmt(1));
      }
      line.if_goto_statement = std::move(if_stmt);
    } else if (statement_ctx && statement_ctx->if_block_start_stmt()) {
      auto *if_ctx = statement_ctx->if_block_start_stmt();
      IfBlockStartStatement if_stmt;
      if_stmt.keyword_location =
          locationFromToken(if_ctx->IF_KW()->getSymbol());
      if_stmt.condition = buildCondition(if_ctx->condition());
      line.if_block_start_statement = std::move(if_stmt);
    } else if (statement_ctx && statement_ctx->else_stmt()) {
      ElseStatement else_stmt;
      else_stmt.keyword_location =
          locationFromToken(statement_ctx->else_stmt()->ELSE_KW()->getSymbol());
      line.else_statement = else_stmt;
    } else if (statement_ctx && statement_ctx->endif_stmt()) {
      EndIfStatement endif_stmt;
      endif_stmt.keyword_location = locationFromToken(
          statement_ctx->endif_stmt()->ENDIF_KW()->getSymbol());
      line.endif_statement = endif_stmt;
    } else if (statement_ctx && statement_ctx->while_stmt()) {
      auto *while_ctx = statement_ctx->while_stmt();
      WhileStatement while_stmt;
      while_stmt.keyword_location =
          locationFromToken(while_ctx->WHILE_KW()->getSymbol());
      while_stmt.condition = buildCondition(while_ctx->condition());
      line.while_statement = std::move(while_stmt);
    } else if (statement_ctx && statement_ctx->endwhile_stmt()) {
      EndWhileStatement endwhile_stmt;
      endwhile_stmt.keyword_location = locationFromToken(
          statement_ctx->endwhile_stmt()->ENDWHILE_KW()->getSymbol());
      line.endwhile_statement = endwhile_stmt;
    } else if (statement_ctx && statement_ctx->for_stmt()) {
      auto *for_ctx = statement_ctx->for_stmt();
      ForStatement for_stmt;
      for_stmt.keyword_location =
          locationFromToken(for_ctx->FOR_KW()->getSymbol());
      for_stmt.variable = toUpper(for_ctx->WORD()->getText());
      for_stmt.start = buildExpr(for_ctx->expr(0));
      for_stmt.end = buildExpr(for_ctx->expr(1));
      line.for_statement = std::move(for_stmt);
    } else if (statement_ctx && statement_ctx->endfor_stmt()) {
      EndForStatement endfor_stmt;
      endfor_stmt.keyword_location = locationFromToken(
          statement_ctx->endfor_stmt()->ENDFOR_KW()->getSymbol());
      line.endfor_statement = endfor_stmt;
    } else if (statement_ctx && statement_ctx->repeat_stmt()) {
      RepeatStatement repeat_stmt;
      repeat_stmt.keyword_location = locationFromToken(
          statement_ctx->repeat_stmt()->REPEAT_KW()->getSymbol());
      line.repeat_statement = repeat_stmt;
    } else if (statement_ctx && statement_ctx->until_stmt()) {
      auto *until_ctx = statement_ctx->until_stmt();
      UntilStatement until_stmt;
      until_stmt.keyword_location =
          locationFromToken(until_ctx->UNTIL_KW()->getSymbol());
      until_stmt.condition = buildCondition(until_ctx->condition());
      line.until_statement = std::move(until_stmt);
    } else if (statement_ctx && statement_ctx->loop_stmt()) {
      LoopStatement loop_stmt;
      loop_stmt.keyword_location =
          locationFromToken(statement_ctx->loop_stmt()->LOOP_KW()->getSymbol());
      line.loop_statement = loop_stmt;
    } else if (statement_ctx && statement_ctx->endloop_stmt()) {
      EndLoopStatement endloop_stmt;
      endloop_stmt.keyword_location = locationFromToken(
          statement_ctx->endloop_stmt()->ENDLOOP_KW()->getSymbol());
      line.endloop_statement = endloop_stmt;
    } else if (statement_ctx) {
      for (auto *item_ctx : statement_ctx->item()) {
        if (auto *word_node = item_ctx->WORD()) {
          auto *token = word_node->getSymbol();
          Word word;
          word.text = token->getText();
          word.location = locationFromToken(token);
          auto parts = splitWordText(word.text);
          word.head = toUpper(parts.head);
          word.value = parts.value;
          word.has_equal = parts.has_equal;
          line.items.emplace_back(std::move(word));
          continue;
        }
        if (auto *comment_node = item_ctx->COMMENT()) {
          auto *token = comment_node->getSymbol();
          Comment comment;
          comment.text = token->getText();
          comment.location = locationFromToken(token);
          line.items.emplace_back(std::move(comment));
        }
      }
    }
    if (!line.items.empty()) {
      if (std::holds_alternative<Word>(line.items.front())) {
        line.line_index = std::get<Word>(line.items.front()).location.line;
      } else {
        line.line_index = std::get<Comment>(line.items.front()).location.line;
      }
    } else if (line.assignment.has_value()) {
      line.line_index = line.assignment->location.line;
    } else if (line.label_definition.has_value()) {
      line.line_index = line.label_definition->location.line;
    } else if (line.goto_statement.has_value()) {
      line.line_index = line.goto_statement->keyword_location.line;
    } else if (line.if_goto_statement.has_value()) {
      line.line_index = line.if_goto_statement->keyword_location.line;
    } else if (line.if_block_start_statement.has_value()) {
      line.line_index = line.if_block_start_statement->keyword_location.line;
    } else if (line.else_statement.has_value()) {
      line.line_index = line.else_statement->keyword_location.line;
    } else if (line.endif_statement.has_value()) {
      line.line_index = line.endif_statement->keyword_location.line;
    } else if (line.while_statement.has_value()) {
      line.line_index = line.while_statement->keyword_location.line;
    } else if (line.endwhile_statement.has_value()) {
      line.line_index = line.endwhile_statement->keyword_location.line;
    } else if (line.for_statement.has_value()) {
      line.line_index = line.for_statement->keyword_location.line;
    } else if (line.endfor_statement.has_value()) {
      line.line_index = line.endfor_statement->keyword_location.line;
    } else if (line.repeat_statement.has_value()) {
      line.line_index = line.repeat_statement->keyword_location.line;
    } else if (line.until_statement.has_value()) {
      line.line_index = line.until_statement->keyword_location.line;
    } else if (line.loop_statement.has_value()) {
      line.line_index = line.loop_statement->keyword_location.line;
    } else if (line.endloop_statement.has_value()) {
      line.line_index = line.endloop_statement->keyword_location.line;
    } else if (line.line_number.has_value()) {
      line.line_index = line.line_number->location.line;
    } else if (line.block_delete_location.has_value()) {
      line.line_index = line.block_delete_location->line;
    } else if (start) {
      line.line_index = start->getLine();
    }
    return line;
  }
};

} // namespace

ParseResult parse(std::string_view input) {
  ParseResult result;
  antlr4::ANTLRInputStream stream{std::string(input)};
  GCodeLexer lexer(&stream);
  antlr4::CommonTokenStream tokens(&lexer);
  GCodeParser parser(&tokens);

  DiagnosticErrorListener error_listener(&result.diagnostics);
  lexer.removeErrorListeners();
  parser.removeErrorListeners();
  lexer.addErrorListener(&error_listener);
  parser.addErrorListener(&error_listener);

  parser.setBuildParseTree(true);
  auto *tree = parser.program();
  if (tree) {
    AstBuilder builder;
    auto program_any = builder.visitProgram(tree);
    if (auto *program_ptr = std::any_cast<Program>(&program_any)) {
      result.program = *program_ptr;
    }
  }

  addSemanticDiagnostics(result);
  return result;
}

} // namespace gcode
