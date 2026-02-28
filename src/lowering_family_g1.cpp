#include "lowering_family_g1.h"

#include "lowering_family_common.h"

namespace gcode {

int G1Lowerer::motionCode() const { return 1; }

ParsedMessage
G1Lowerer::lower(const Line &line, const LowerOptions &options,
                 std::vector<Diagnostic> * /*diagnostics*/) const {
  G1Message message;
  message.source = sourceFromLine(line, options);
  message.modal.group = ModalGroupId::Motion;
  message.modal.code = "G1";
  message.modal.updates_state = true;
  fillPoseAndFeed(line, &message.target_pose, &message.feed, nullptr);
  return message;
}

} // namespace gcode
