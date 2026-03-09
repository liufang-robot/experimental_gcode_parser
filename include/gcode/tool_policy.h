#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "gcode/machine_profile.h"
#include "gcode/tool_selection.h"

namespace gcode {

enum class ToolSelectionResolutionKind { Resolved, Unresolved, Ambiguous };

struct ToolSelectionResolution {
  ToolSelectionResolutionKind kind = ToolSelectionResolutionKind::Resolved;
  ToolSelectionState selection;
  bool substituted = false;
  std::string message;
};

struct ToolPolicyOptions {
  bool allow_substitution = false;
  ErrorPolicy on_unresolved_tool = ErrorPolicy::Error;
  ErrorPolicy on_ambiguous_tool = ErrorPolicy::Error;
  std::optional<ToolSelectionState> fallback_selection;
  std::unordered_map<std::string, std::string> substitution_map;
};

class ToolPolicy {
public:
  virtual ~ToolPolicy() = default;

  virtual ToolSelectionResolution
  resolveSelection(const ToolSelectionState &selection) const = 0;
  virtual ErrorPolicy unresolvedPolicy() const = 0;
  virtual ErrorPolicy ambiguousPolicy() const = 0;
};

class DefaultToolPolicy final : public ToolPolicy {
public:
  explicit DefaultToolPolicy(ToolPolicyOptions options = {})
      : options_(std::move(options)) {}

  ToolSelectionResolution
  resolveSelection(const ToolSelectionState &selection) const override;
  ErrorPolicy unresolvedPolicy() const override {
    return options_.on_unresolved_tool;
  }
  ErrorPolicy ambiguousPolicy() const override {
    return options_.on_ambiguous_tool;
  }

private:
  ToolPolicyOptions options_;
};

} // namespace gcode
