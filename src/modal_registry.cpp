#include "modal_registry.h"

namespace gcode {

ModalRegistry::ModalRegistry(MachineProfile profile)
    : profile_(std::move(profile)) {}

void ModalRegistry::beginBlock() {
  block_state_.clear();
  touched_groups_this_block_.clear();
}

bool ModalRegistry::apply(const ModalTransition &transition,
                          std::optional<std::string> *error) {
  const int key = static_cast<int>(transition.group);
  const bool touched = touched_groups_this_block_.count(key) != 0;

  if (touched) {
    const auto prior = activeCode(transition.group);
    if (prior.has_value() && *prior != transition.code &&
        profile_.modal_conflict_policy == ErrorPolicy::Error) {
      if (error != nullptr) {
        *error = "modal conflict in block for group " + std::to_string(key) +
                 ": '" + *prior + "' vs '" + transition.code + "'";
      }
      return false;
    }
  }

  touched_groups_this_block_.insert(key);
  if (isBlockScoped(transition.group)) {
    block_state_[key] = transition.code;
  } else {
    persistent_state_[key] = transition.code;
  }
  return true;
}

std::optional<std::string> ModalRegistry::activeCode(ModalGroup group) const {
  const int key = static_cast<int>(group);
  if (isBlockScoped(group)) {
    const auto found = block_state_.find(key);
    if (found != block_state_.end()) {
      return found->second;
    }
    return std::nullopt;
  }

  const auto found = persistent_state_.find(key);
  if (found != persistent_state_.end()) {
    return found->second;
  }
  return std::nullopt;
}

bool ModalRegistry::hasActiveCode(ModalGroup group) const {
  return activeCode(group).has_value();
}

bool ModalRegistry::isBlockScoped(ModalGroup group) const {
  return group == ModalGroup::ExactStopOneShot;
}

} // namespace gcode
