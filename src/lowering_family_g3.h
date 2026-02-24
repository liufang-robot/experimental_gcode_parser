#pragma once

#include "lowering_family.h"

namespace gcode {

class G3Lowerer final : public MotionFamilyLowerer {
public:
  int motionCode() const override;
  ParsedMessage lower(const Line &line, const LowerOptions &options,
                      std::vector<Diagnostic> *diagnostics) const override;
};

} // namespace gcode
