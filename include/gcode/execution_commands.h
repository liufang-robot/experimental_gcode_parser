#pragma once

#include <optional>
#include <string>
#include <vector>

#include "gcode/ail.h"
#include "gcode/runtime_status.h"

namespace gcode {

struct SourceRef {
  std::optional<std::string> filename;
  int line = 0;
  std::optional<int> line_number;
};

struct EffectiveModalSnapshot {
  std::string motion_code;
  WorkingPlane working_plane = WorkingPlane::XY;
  RapidInterpolationMode rapid_mode = RapidInterpolationMode::Linear;
  ToolRadiusCompMode tool_radius_comp = ToolRadiusCompMode::Off;
  std::optional<ToolSelectionState> active_tool_selection;
  std::optional<ToolSelectionState> pending_tool_selection;
};

struct PoseTarget {
  std::optional<double> x;
  std::optional<double> y;
  std::optional<double> z;
  std::optional<double> a;
  std::optional<double> b;
  std::optional<double> c;
};

struct LinearMoveCommand {
  SourceRef source;
  PoseTarget target;
  std::optional<double> feed;
  EffectiveModalSnapshot effective;
};

struct ArcMoveCommand {
  SourceRef source;
  bool clockwise = true;
  PoseTarget target;
  ArcParams arc;
  std::optional<double> feed;
  EffectiveModalSnapshot effective;
};

struct DwellCommand {
  SourceRef source;
  DwellMode dwell_mode = DwellMode::Seconds;
  double dwell_value = 0.0;
  EffectiveModalSnapshot effective;
};

struct RejectedLineEvent {
  SourceRef source;
  std::vector<Diagnostic> reasons;
};

enum class EngineState {
  AcceptingInput,
  ReadyToExecute,
  Blocked,
  Completed,
  Cancelled,
  Faulted,
};

struct BlockedState {
  int line = 0;
  WaitToken token;
  std::string reason;
};

enum class StepStatus { Progress, Blocked, Completed, Cancelled, Faulted };

struct StepResult {
  StepStatus status = StepStatus::Progress;
  std::optional<BlockedState> blocked;
  std::optional<Diagnostic> fault;
};

} // namespace gcode
