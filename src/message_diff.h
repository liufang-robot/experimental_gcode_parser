#pragma once

#include <vector>

#include "messages.h"

namespace gcode {

struct LineMessage {
  int line = 0;
  ParsedMessage message;
};

struct MessageDiff {
  std::vector<LineMessage> added;
  std::vector<LineMessage> updated;
  std::vector<int> removed_lines;
};

MessageDiff diffMessagesByLine(const MessageResult &before,
                               const MessageResult &after);

std::vector<ParsedMessage>
applyMessageDiff(const std::vector<ParsedMessage> &current,
                 const MessageDiff &diff);

} // namespace gcode
