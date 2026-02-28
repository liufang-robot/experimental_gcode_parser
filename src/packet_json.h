#pragma once

#include <string>

#include "packet.h"

namespace gcode {

std::string packetToJsonString(const PacketResult &result, bool pretty = true);

} // namespace gcode
