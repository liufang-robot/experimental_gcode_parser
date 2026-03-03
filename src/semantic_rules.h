#pragma once

#include "gcode_parser.h"

namespace gcode {

void addSemanticDiagnostics(ParseResult &result,
                            bool enable_double_slash_comments);

} // namespace gcode
