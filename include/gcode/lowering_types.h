#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "gcode/ast.h"
#include "gcode/policy_types.h"

namespace gcode {

struct SourceInfo {
  std::optional<std::string> filename;
  int line = 0;
  std::optional<int> line_number;
};

struct Pose6 {
  std::optional<double> x;
  std::optional<double> y;
  std::optional<double> z;
  std::optional<double> a;
  std::optional<double> b;
  std::optional<double> c;
};

enum class ModalGroupId {
  Motion = 1,
  NonModal = 2,
};

struct ModalState {
  ModalGroupId group = ModalGroupId::Motion;
  std::string code;
  bool updates_state = false;
};

struct ArcParams {
  std::optional<double> i;
  std::optional<double> j;
  std::optional<double> k;
  std::optional<double> r;
};

enum class DwellMode { Seconds, Revolutions };

struct LowerOptions {
  std::optional<std::string> filename;
  std::vector<int> active_skip_levels;
  std::optional<ToolChangeMode> tool_change_mode;
  bool enable_iso_m98_calls = false;
};

struct RejectedLine {
  SourceInfo source;
  std::vector<Diagnostic> reasons;
};

} // namespace gcode
