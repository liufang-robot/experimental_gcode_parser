#include "lowering_family_g3.h"

#include "lowering_family_common.h"

namespace gcode {

int G3Lowerer::motionCode() const { return 3; }

ParsedMessage G3Lowerer::lower(const Line &line, const LowerOptions &options,
                               std::vector<Diagnostic> *diagnostics) const {
  G3Message message;
  message.source = sourceFromLine(line, options);
  message.modal.group = ModalGroupId::Motion;
  message.modal.code = "G3";
  message.modal.updates_state = true;
  fillPoseAndFeed(line, &message.target_pose, &message.feed, &message.arc);
  addArcLoweringWarnings(line, diagnostics);
  return message;
}

} // namespace gcode
