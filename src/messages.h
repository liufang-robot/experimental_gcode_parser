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

using ParsedMessage = std::variant<G1Message>;

struct LowerOptions {
  enum class OnError {
    Continue,
    StopAtFirstError,
  };

  std::optional<std::string> filename;
  OnError on_error = OnError::Continue;
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
