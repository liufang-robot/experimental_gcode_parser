#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "messages.h"

namespace gcode {

struct AilLinearMoveInstruction {
  SourceInfo source;
  ModalState modal;
  Pose6 target_pose;
  std::optional<double> feed;
};

struct AilArcMoveInstruction {
  SourceInfo source;
  ModalState modal;
  bool clockwise = true;
  Pose6 target_pose;
  ArcParams arc;
  std::optional<double> feed;
};

struct AilDwellInstruction {
  SourceInfo source;
  ModalState modal;
  DwellMode dwell_mode = DwellMode::Seconds;
  double dwell_value = 0.0;
};

// Placeholder variants for upcoming non-motion semantics.
struct AilAssignInstruction {
  SourceInfo source;
  std::string lhs;
  std::string rhs_expr;
};

struct AilSyncInstruction {
  SourceInfo source;
  std::string sync_tag;
};

using AilInstruction =
    std::variant<AilLinearMoveInstruction, AilArcMoveInstruction,
                 AilDwellInstruction, AilAssignInstruction, AilSyncInstruction>;

struct AilResult {
  std::vector<AilInstruction> instructions;
  std::vector<Diagnostic> diagnostics;
  std::vector<MessageResult::RejectedLine> rejected_lines;
};

AilResult lowerToAil(const Program &program,
                     const std::vector<Diagnostic> &parse_diagnostics,
                     const LowerOptions &options = {});

AilResult parseAndLowerAil(std::string_view input,
                           const LowerOptions &options = {});

} // namespace gcode
