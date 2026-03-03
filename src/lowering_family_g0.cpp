#include "lowering_family_g0.h"

#include "lowering_family_common.h"

namespace gcode {

int G0Lowerer::motionCode() const { return 0; }

ParsedMessage
G0Lowerer::lower(const Line &line, const LowerOptions &options,
                 std::vector<Diagnostic> * /*diagnostics*/) const {
  G0Message message;
  message.source = sourceFromLine(line, options);
  message.modal.group = ModalGroupId::Motion;
  message.modal.code = "G0";
  message.modal.updates_state = true;
  fillPoseAndFeed(line, &message.target_pose, &message.feed, nullptr);
  return message;
}

} // namespace gcode
