#pragma once

#include <string>

#include "ail.h"

namespace gcode {

std::string ailToJsonString(const AilResult &result, bool pretty = true);

} // namespace gcode
