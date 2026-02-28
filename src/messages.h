#pragma once

#include <cstddef>
#include <functional>
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

enum class ModalGroupId {
  Motion = 1,
  NonModal = 2,
};

struct ModalState {
  ModalGroupId group = ModalGroupId::Motion;
  std::string code;
  bool updates_state = false;
};

struct G1Message {
  SourceInfo source;
  ModalState modal;
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
  ModalState modal;
  Pose6 target_pose;
  ArcParams arc;
  std::optional<double> feed;
};

struct G3Message {
  SourceInfo source;
  ModalState modal;
  Pose6 target_pose;
  ArcParams arc;
  std::optional<double> feed;
};

enum class DwellMode { Seconds, Revolutions };

struct G4Message {
  SourceInfo source;
  ModalState modal;
  DwellMode dwell_mode = DwellMode::Seconds;
  double dwell_value = 0.0;
};

using ParsedMessage = std::variant<G1Message, G2Message, G3Message, G4Message>;

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

struct StreamCallbacks {
  std::function<void(const ParsedMessage &message)> on_message;
  std::function<void(const Diagnostic &diagnostic)> on_diagnostic;
  std::function<void(const MessageResult::RejectedLine &rejected_line)>
      on_rejected_line;
};

struct StreamOptions {
  std::optional<size_t> max_lines;
  std::optional<size_t> max_messages;
  std::optional<size_t> max_diagnostics;
  std::function<bool()> should_cancel;
};

MessageResult lowerToMessages(const Program &program,
                              const std::vector<Diagnostic> &parse_diagnostics,
                              const LowerOptions &options = {});

MessageResult parseAndLower(std::string_view input,
                            const LowerOptions &options = {});

bool lowerToMessagesStream(const Program &program,
                           const std::vector<Diagnostic> &parse_diagnostics,
                           const LowerOptions &options,
                           const StreamCallbacks &callbacks,
                           const StreamOptions &stream_options = {});

bool parseAndLowerStream(std::string_view input, const LowerOptions &options,
                         const StreamCallbacks &callbacks,
                         const StreamOptions &stream_options = {});

bool parseAndLowerFileStream(const std::string &path,
                             const LowerOptions &options,
                             const StreamCallbacks &callbacks,
                             const StreamOptions &stream_options = {});

} // namespace gcode
