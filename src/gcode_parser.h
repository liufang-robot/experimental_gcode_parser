#pragma once

#include <string_view>

#include "ast.h"

namespace gcode {

struct ParseOptions {
  bool enable_double_slash_comments = false;
  bool tool_management = false;
  bool enable_iso_m98_calls = false;
};

ParseResult parse(std::string_view input, const ParseOptions &options);
ParseResult parse(std::string_view input);

} // namespace gcode
