#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "gcode/machine_profile.h"

namespace gcode {

enum class ModalGroup {
  Motion = 1,
  Plane = 6,
  ToolRadiusComp = 7,
  WorkOffset = 8,
  ExactStopPathControl = 10,
  ExactStopOneShot = 11,
  ExactStopCriterion = 12,
  Units = 13,
  Dimensions = 14,
  FeedType = 15,
};

struct ModalTransition {
  ModalGroup group = ModalGroup::Motion;
  std::string code;
};

class ModalRegistry {
public:
  explicit ModalRegistry(
      MachineProfile profile = MachineProfile::siemens840dBaseline());

  void beginBlock();
  bool apply(const ModalTransition &transition,
             std::optional<std::string> *error = nullptr);

  std::optional<std::string> activeCode(ModalGroup group) const;
  bool hasActiveCode(ModalGroup group) const;

private:
  bool isBlockScoped(ModalGroup group) const;

  MachineProfile profile_;
  std::unordered_map<int, std::string> persistent_state_;
  std::unordered_map<int, std::string> block_state_;
  std::unordered_set<int> touched_groups_this_block_;
};

} // namespace gcode
