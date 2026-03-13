#include "session.h"

#include <algorithm>
#include <sstream>

namespace gcode {
namespace {

std::string sanitizeLine(std::string_view raw) {
  std::string line(raw);
  while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
    line.pop_back();
  }
  return line;
}

} // namespace

ParseSession::ParseSession(std::string_view text, const LowerOptions &options)
    : lines_(splitLines(text)), options_(options) {
  current_ = parseAndLower(joinLines(), options_);
}

const MessageResult &ParseSession::result() const { return current_; }

std::string ParseSession::text() const { return joinLines(); }

ResumeResult ParseSession::applyLineEdit(int line_1based,
                                         std::string_view new_line) {
  int line = std::max(1, line_1based);
  if (static_cast<size_t>(line) > lines_.size()) {
    lines_.resize(static_cast<size_t>(line));
  }
  lines_[static_cast<size_t>(line - 1)] = sanitizeLine(new_line);
  return reparseFromLine(line);
}

ResumeResult ParseSession::reparseFromLine(int from_line_1based) {
  ResumeResult update;
  update.from_line = std::max(1, from_line_1based);
  current_ = parseAndLower(joinLines(), options_);
  update.result = current_;
  return update;
}

std::vector<std::string> ParseSession::splitLines(std::string_view text) {
  std::vector<std::string> lines;
  std::stringstream input{std::string(text)};
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    lines.push_back(line);
  }
  return lines;
}

std::string ParseSession::joinLines() const {
  std::string joined;
  for (size_t i = 0; i < lines_.size(); ++i) {
    if (i != 0) {
      joined.push_back('\n');
    }
    joined += lines_[i];
  }
  if (!lines_.empty()) {
    joined.push_back('\n');
  }
  return joined;
}

} // namespace gcode
