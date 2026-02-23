#pragma once

#include <string>

#include "messages.h"

namespace gcode {

std::string toJsonString(const MessageResult &result, bool pretty = true);
MessageResult fromJsonString(const std::string &json_text);

} // namespace gcode
