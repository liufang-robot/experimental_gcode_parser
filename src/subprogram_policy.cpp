#include "subprogram_policy.h"

namespace gcode {
namespace {

std::string bareSubprogramName(const std::string &target) {
  const auto pos = target.find_last_of("/\\");
  if (pos == std::string::npos || pos + 1 >= target.size()) {
    return target;
  }
  return target.substr(pos + 1);
}

} // namespace

SubprogramResolution DefaultSubprogramPolicy::resolveTarget(
    const std::string &requested_target,
    const std::unordered_map<std::string, std::vector<size_t>> &label_positions)
    const {
  SubprogramResolution resolution;
  const auto exact_it = label_positions.find(requested_target);
  if (exact_it != label_positions.end() && !exact_it->second.empty()) {
    resolution.resolved = true;
    resolution.resolved_target = requested_target;
    return resolution;
  }

  if (options_.search_policy == SubprogramSearchPolicy::ExactThenBareName) {
    const std::string fallback = bareSubprogramName(requested_target);
    if (fallback != requested_target) {
      const auto fallback_it = label_positions.find(fallback);
      if (fallback_it != label_positions.end() &&
          !fallback_it->second.empty()) {
        resolution.resolved = true;
        resolution.resolved_target = fallback;
        resolution.fallback_used = true;
        resolution.message =
            "subprogram target resolved by bare-name fallback: " +
            requested_target + " -> " + fallback;
        return resolution;
      }
    }
  }

  resolution.resolved = false;
  resolution.resolved_target = requested_target;
  return resolution;
}

} // namespace gcode
