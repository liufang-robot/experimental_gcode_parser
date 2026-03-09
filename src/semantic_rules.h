#pragma once

#include "gcode/gcode_parser.h"

namespace gcode {

void addSemanticDiagnostics(ParseResult &result,
                            bool enable_double_slash_comments,
                            bool tool_management = false,
                            bool enable_iso_m98_calls = false);

} // namespace gcode
