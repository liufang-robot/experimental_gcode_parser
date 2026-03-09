#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "gcode/policy_types.h"

namespace gcode {

enum class ControllerKind {
  Generic = 0,
  Siemens840D,
};

class RangeSet {
public:
  void add(int min_inclusive, int max_inclusive);
  bool contains(int value) const;
  bool empty() const { return ranges_.empty(); }

private:
  std::vector<std::pair<int, int>> ranges_;
};

struct MachineProfile {
  ControllerKind controller = ControllerKind::Generic;
  ToolChangeMode tool_change_mode = ToolChangeMode::DeferredM6;
  bool tool_management = false;
  ErrorPolicy unknown_mcode_policy = ErrorPolicy::Error;
  ErrorPolicy modal_conflict_policy = ErrorPolicy::Error;
  RangeSet supported_work_offsets;

  static MachineProfile siemens840dBaseline();
};

} // namespace gcode
