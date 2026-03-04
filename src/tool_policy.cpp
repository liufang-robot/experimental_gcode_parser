#include "tool_policy.h"

#include <sstream>

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

ToolSelectionResolution makeAmbiguous(const ToolSelectionState &selection,
                                      const std::string &message) {
  ToolSelectionResolution resolution;
  resolution.kind = ToolSelectionResolutionKind::Ambiguous;
  resolution.selection = selection;
  resolution.message = message;
  return resolution;
}

} // namespace

bool DefaultToolPolicy::startsWith(std::string_view text,
                                   std::string_view prefix) {
  return text.size() >= prefix.size() &&
         text.substr(0, prefix.size()) == prefix;
}

std::optional<ToolSelectionState>
DefaultToolPolicy::parseAmbiguousFirstCandidate(
    const ToolSelectionState &selection) {
  static constexpr std::string_view kPrefix = "AMBIG:";
  if (!startsWith(selection.selector_value, kPrefix)) {
    return std::nullopt;
  }

  const std::string raw = selection.selector_value.substr(kPrefix.size());
  const auto split = raw.find('|');
  const std::string first =
      split == std::string::npos ? raw : raw.substr(0, split);
  if (first.empty()) {
    return std::nullopt;
  }

  ToolSelectionState chosen = selection;
  chosen.selector_value = first;
  return chosen;
}

ToolSelectionResolution
DefaultToolPolicy::resolveSelection(const ToolSelectionState &selection) const {
  if (selection.selector_value == "999991" ||
      selection.selector_value == "UNRESOLVED" ||
      startsWith(selection.selector_value, "UNRESOLVED:")) {
    if (options_.fallback_selection.has_value()) {
      return makeResolved(
          *options_.fallback_selection, true,
          "tool unresolved; fallback selection applied by policy");
    }
    return makeUnresolved(selection, "tool selection unresolved by policy");
  }

  if (selection.selector_value == "999992") {
    if (options_.fallback_selection.has_value()) {
      return makeResolved(
          *options_.fallback_selection, true,
          "ambiguous tool selection; fallback selection applied by policy");
    }
    return makeAmbiguous(selection, "tool selection ambiguous by policy");
  }

  if (const auto chosen = parseAmbiguousFirstCandidate(selection);
      chosen.has_value()) {
    if (options_.allow_substitution) {
      return makeResolved(
          *chosen, true,
          "ambiguous tool selection resolved by substitution policy");
    }
    if (options_.fallback_selection.has_value()) {
      return makeResolved(
          *options_.fallback_selection, true,
          "ambiguous tool selection; fallback selection applied by policy");
    }
    return makeAmbiguous(selection, "tool selection ambiguous by policy");
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
