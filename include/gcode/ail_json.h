#pragma once

#include <string>

#include "gcode/ail.h"

namespace gcode {

std::string ailToJsonString(const AilResult &result, bool pretty = true);

} // namespace gcode
