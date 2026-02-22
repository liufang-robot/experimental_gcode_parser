#pragma once

#include <string_view>

#include "ast.h"

namespace gcode {

ParseResult parse(std::string_view input);

} // namespace gcode
