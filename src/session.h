#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "messages.h"

namespace gcode {

struct ResumeResult {
  int from_line = 1;
  MessageResult result;
};

class ParseSession {
public:
  ParseSession(std::string_view text, const LowerOptions &options = {});

  const MessageResult &result() const;
  std::string text() const;

  ResumeResult applyLineEdit(int line_1based, std::string_view new_line);
  ResumeResult reparseFromLine(int from_line_1based);

private:
  static std::vector<std::string> splitLines(std::string_view text);
  std::string joinLines() const;

  std::vector<std::string> lines_;
  LowerOptions options_;
  MessageResult current_;
};

} // namespace gcode
