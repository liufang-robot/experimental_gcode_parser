#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace gcode {

struct ToolSelectionState {
  std::optional<int64_t> selector_index;
  std::string selector_value;
};

} // namespace gcode
