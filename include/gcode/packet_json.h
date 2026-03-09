#pragma once

#include <string>

#include "gcode/packet.h"

namespace gcode {

std::string packetToJsonString(const PacketResult &result, bool pretty = true);

} // namespace gcode
