#include "gcode_parser.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>

#include "GCodeBaseVisitor.h"
#include "GCodeLexer.h"
#include "GCodeParser.h"
#include "antlr4-runtime.h"

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
  std::transform(value.begin(), value.end(), value.begin(),
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
    return std::isdigit(static_cast<unsigned char>(c)) || c == '+' || c == '-' ||
           c == '.';
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
  explicit DiagnosticErrorListener(std::vector<Diagnostic> *out)
      : out_(out) {}

  void syntaxError(antlr4::Recognizer * /*recognizer*/,
                   antlr4::Token * /*offendingSymbol*/, size_t line,
                   size_t charPositionInLine, const std::string &msg,
                   std::exception_ptr /*e*/) override {
    if (!out_) {
      return;
    }
    Diagnostic diag;
    diag.severity = Diagnostic::Severity::Error;
    diag.message = msg;
    diag.location = {static_cast<int>(line),
                     static_cast<int>(charPositionInLine) + 1};
    out_->push_back(std::move(diag));
  }

private:
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
          !tail_ctx->item().empty()) {
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
                     ctx->item());
  }

  antlrcpp::Any
  visitLine_no_eol(GCodeParser::Line_no_eolContext *ctx) override {
    return buildLine(ctx->getStart(), ctx->block_delete(), ctx->line_number(),
                     ctx->item());
  }

private:
  Line buildLine(antlr4::Token *start,
                 GCodeParser::Block_deleteContext *block_ctx,
                 GCodeParser::Line_numberContext *number_ctx,
                 const std::vector<GCodeParser::ItemContext *> &items) {
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
    for (auto *item_ctx : items) {
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
    if (!line.items.empty()) {
      if (std::holds_alternative<Word>(line.items.front())) {
        line.line_index =
            std::get<Word>(line.items.front()).location.line;
      } else {
        line.line_index =
            std::get<Comment>(line.items.front()).location.line;
      }
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

bool isMotionWord(const Word &word, int *out_code) {
  if (word.head != "G" || !word.value.has_value()) {
    return false;
  }
  try {
    int code = std::stoi(*word.value);
    if (code == 1 || code == 2 || code == 3) {
      if (out_code) {
        *out_code = code;
      }
      return true;
    }
  } catch (...) {
    return false;
  }
  return false;
}

bool isCartesianWord(const Word &word) {
  return word.head == "X" || word.head == "Y" || word.head == "Z" ||
         word.head == "A";
}

bool isPolarWord(const Word &word) {
  return word.head == "AP" || word.head == "RP";
}

void addDiagnostic(std::vector<Diagnostic> &diags, const Location &loc,
                   const std::string &message) {
  Diagnostic diag;
  diag.severity = Diagnostic::Severity::Error;
  diag.message = message;
  diag.location = loc;
  diags.push_back(std::move(diag));
}

void addSemanticDiagnostics(ParseResult &result) {
  for (auto &line : result.program.lines) {
    int motion_code = 0;
    bool has_motion = false;
    bool has_cartesian = false;
    bool has_polar = false;

    for (const auto &item : line.items) {
      if (!std::holds_alternative<Word>(item)) {
        continue;
      }
      const auto &word = std::get<Word>(item);
      int code = 0;
      if (isMotionWord(word, &code)) {
        if (has_motion && code != motion_code) {
          addDiagnostic(result.diagnostics, word.location,
                        "multiple motion commands in a single line");
          break;
        }
        has_motion = true;
        motion_code = code;
        continue;
      }
      if (isCartesianWord(word)) {
        has_cartesian = true;
      }
      if (isPolarWord(word)) {
        has_polar = true;
        if (has_cartesian && has_motion && motion_code == 1) {
          addDiagnostic(result.diagnostics, word.location,
                        "mixed cartesian and polar words in G1 line");
          break;
        }
      }
      if (has_cartesian && has_polar && has_motion && motion_code == 1) {
        addDiagnostic(result.diagnostics, word.location,
                      "mixed cartesian and polar words in G1 line");
        break;
      }
    }
  }
}

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
