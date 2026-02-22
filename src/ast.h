#pragma once

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

struct LineNumber {
  int value = 0;
  Location location;
};

struct Line {
  bool block_delete = false;
  std::optional<Location> block_delete_location;
  std::optional<LineNumber> line_number;
  std::vector<LineItem> items;
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
