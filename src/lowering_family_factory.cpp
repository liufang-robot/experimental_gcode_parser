#include "lowering_family.h"

#include <memory>

#include "lowering_family_g1.h"
#include "lowering_family_g2.h"
#include "lowering_family_g3.h"

namespace gcode {

std::vector<std::unique_ptr<MotionFamilyLowerer>> createMotionFamilyLowerers() {
  std::vector<std::unique_ptr<MotionFamilyLowerer>> lowerers;
  lowerers.push_back(std::make_unique<G1Lowerer>());
  lowerers.push_back(std::make_unique<G2Lowerer>());
  lowerers.push_back(std::make_unique<G3Lowerer>());
  return lowerers;
}

} // namespace gcode
