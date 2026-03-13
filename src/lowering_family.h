#pragma once

#include <memory>
#include <vector>

#include "messages.h"

namespace gcode {

class MotionFamilyLowerer {
public:
  virtual ~MotionFamilyLowerer() = default;
  virtual int motionCode() const = 0;
  virtual ParsedMessage lower(const Line &line, const LowerOptions &options,
                              std::vector<Diagnostic> *diagnostics) const = 0;
};

std::vector<std::unique_ptr<MotionFamilyLowerer>> createMotionFamilyLowerers();

} // namespace gcode
