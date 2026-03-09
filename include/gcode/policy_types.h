#pragma once

namespace gcode {

enum class ToolChangeMode {
  DirectT = 0,
  DeferredM6,
};

enum class ErrorPolicy {
  Error = 0,
  Warning,
  Ignore,
};

} // namespace gcode
