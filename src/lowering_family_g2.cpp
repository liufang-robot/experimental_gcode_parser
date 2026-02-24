#include "lowering_family_g2.h"

#include "lowering_family_common.h"

namespace gcode {

int G2Lowerer::motionCode() const { return 2; }

ParsedMessage G2Lowerer::lower(const Line &line, const LowerOptions &options,
                               std::vector<Diagnostic> *diagnostics) const {
  G2Message message;
  message.source = sourceFromLine(line, options);
  fillPoseAndFeed(line, &message.target_pose, &message.feed, &message.arc);
  addArcLoweringWarnings(line, diagnostics);
  return message;
}

} // namespace gcode
