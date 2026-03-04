#include "tool_policy.h"

namespace gcode {
namespace {

ToolSelectionResolution makeResolved(const ToolSelectionState &selection,
                                     bool substituted,
                                     const std::string &message) {
  ToolSelectionResolution resolution;
  resolution.kind = ToolSelectionResolutionKind::Resolved;
  resolution.selection = selection;
  resolution.substituted = substituted;
  resolution.message = message;
  return resolution;
}

ToolSelectionResolution makeUnresolved(const ToolSelectionState &selection,
                                       const std::string &message) {
  ToolSelectionResolution resolution;
  resolution.kind = ToolSelectionResolutionKind::Unresolved;
  resolution.selection = selection;
  resolution.message = message;
  return resolution;
}

} // namespace

ToolSelectionResolution
DefaultToolPolicy::resolveSelection(const ToolSelectionState &selection) const {
  if (selection.selector_value.empty()) {
    if (options_.fallback_selection.has_value()) {
      return makeResolved(
          *options_.fallback_selection, true,
          "tool unresolved; fallback selection applied by policy");
    }
    return makeUnresolved(selection, "tool selection unresolved by policy");
  }

  if (options_.allow_substitution) {
    const auto it = options_.substitution_map.find(selection.selector_value);
    if (it != options_.substitution_map.end()) {
      ToolSelectionState mapped = selection;
      mapped.selector_value = it->second;
      return makeResolved(mapped,
                          mapped.selector_value != selection.selector_value,
                          "tool selection substituted by policy map");
    }
  }

  return makeResolved(selection, false, "");
}

} // namespace gcode
