#include "ast_printer.h"

#include <sstream>

namespace gcode {
namespace {

void writeLocation(std::ostringstream &out, const Location &loc) {
  out << loc.line << ":" << loc.column;
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

} // namespace gcode
