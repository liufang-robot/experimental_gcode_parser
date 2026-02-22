#pragma once

#include <string>

#include "ast.h"

namespace gcode {

std::string format(const ParseResult &result);

} // namespace gcode
