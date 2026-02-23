#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "ast.h"

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

struct G1Message {
  SourceInfo source;
  Pose6 target_pose;
  std::optional<double> feed;
};

struct ArcParams {
  std::optional<double> i;
  std::optional<double> j;
  std::optional<double> k;
  std::optional<double> r;
};

struct G2Message {
  SourceInfo source;
  Pose6 target_pose;
  ArcParams arc;
  std::optional<double> feed;
};

struct G3Message {
  SourceInfo source;
  Pose6 target_pose;
  ArcParams arc;
  std::optional<double> feed;
};

using ParsedMessage = std::variant<G1Message, G2Message, G3Message>;

struct LowerOptions {
  std::optional<std::string> filename;
};

struct MessageResult {
  struct RejectedLine {
    SourceInfo source;
    std::vector<Diagnostic> reasons;
  };

  std::vector<ParsedMessage> messages;
  std::vector<Diagnostic> diagnostics;
  std::vector<RejectedLine> rejected_lines;
};

MessageResult lowerToMessages(const Program &program,
                              const std::vector<Diagnostic> &parse_diagnostics,
                              const LowerOptions &options = {});

MessageResult parseAndLower(std::string_view input,
                            const LowerOptions &options = {});

} // namespace gcode
