#include "gcode/machine_profile.h"

namespace gcode {

void RangeSet::add(int min_inclusive, int max_inclusive) {
  if (max_inclusive < min_inclusive) {
    return;
  }
  ranges_.push_back({min_inclusive, max_inclusive});
}

bool RangeSet::contains(int value) const {
  for (const auto &range : ranges_) {
    if (value >= range.first && value <= range.second) {
      return true;
    }
  }
  return false;
}

MachineProfile MachineProfile::siemens840dBaseline() {
  MachineProfile profile;
  profile.controller = ControllerKind::Siemens840D;
  profile.tool_change_mode = ToolChangeMode::DeferredM6;
  profile.tool_management = false;
  profile.unknown_mcode_policy = ErrorPolicy::Error;
  profile.modal_conflict_policy = ErrorPolicy::Error;

  // Group 8 baseline: G54..G57 and G505..G599.
  profile.supported_work_offsets.add(54, 57);
  profile.supported_work_offsets.add(505, 599);

  return profile;
}

} // namespace gcode
