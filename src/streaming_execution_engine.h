#pragma once

#include <deque>
#include <optional>
#include <string>
#include <string_view>

#include "ail.h"
#include "execution_interfaces.h"
#include "messages.h"

namespace gcode {

class StreamingExecutionEngine {
public:
  StreamingExecutionEngine(IExecutionSink &sink, IRuntime &runtime,
                           ICancellation &cancellation,
                           const LowerOptions &options = {});

  bool pushChunk(std::string_view chunk);
  StepResult pump();
  StepResult finish();
  StepResult resume(const WaitToken &token);
  void cancel();

  EngineState state() const { return state_; }

private:
  struct PendingLine {
    int line = 0;
    std::string text;
  };

  bool enqueueCompleteLines();
  StepResult executeNextLine();
  StepResult executeAilInstruction(const AilInstruction &instruction, int line);
  StepResult makeBlockedResult(int line, const WaitToken &token,
                               std::string reason);
  StepResult faultWithDiagnostic(const Diagnostic &diag);
  void emitDiagnostics(const std::vector<Diagnostic> &diagnostics);
  std::optional<RejectedLineEvent>
  makeRejectedEvent(const MessageResult::RejectedLine &rejected) const;
  void remapDiagnostics(std::vector<Diagnostic> *diagnostics, int line) const;
  void
  remapRejectedLines(std::vector<MessageResult::RejectedLine> *rejected_lines,
                     int line) const;
  EffectiveModalState
  makeEffectiveModalState(const std::string &motion_code) const;

  IExecutionSink &sink_;
  IRuntime &runtime_;
  ICancellation &cancellation_;
  LowerOptions options_;
  EngineState state_ = EngineState::AcceptingInput;
  std::string input_buffer_;
  std::deque<PendingLine> pending_lines_;
  std::optional<BlockedState> blocked_;
  int next_line_number_ = 1;
  bool input_finished_ = false;
  std::optional<WorkingPlane> current_working_plane_ = WorkingPlane::XY;
  std::optional<RapidInterpolationMode> current_rapid_mode_;
  std::optional<ToolRadiusCompMode> current_tool_radius_comp_ =
      ToolRadiusCompMode::Off;
};

} // namespace gcode
