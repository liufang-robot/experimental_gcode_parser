#pragma once

#include <string>

#include "ast.h"

namespace gcode {

std::string format(const ParseResult &result);
std::string formatJson(const ParseResult &result, bool pretty = true);

} // namespace gcode
