#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "gcode/ail.h"
#include "gcode/execution_interfaces.h"
#include "gcode/execution_runtime.h"

namespace gcode {

class ExecutionSession;

class StreamingExecutionEngine {
public:
  StreamingExecutionEngine(IExecutionSink &sink, IRuntime &runtime,
                           ICancellation &cancellation,
                           const LowerOptions &options = {});
  StreamingExecutionEngine(IExecutionSink &sink, IExecutionRuntime &runtime,
                           ICancellation &cancellation,
                           const LowerOptions &options = {});

  bool pushChunk(std::string_view chunk);
  StepResult pump();
  StepResult finish();
  StepResult resume(const WaitToken &token);
  void cancel();

  EngineState state() const { return state_; }

  struct PendingLine {
    int line = 0;
    std::string text;
  };

private:
  bool enqueueCompleteLines();
  StepResult executePendingProgram();
  StepResult advanceActiveExecutor();
  StepResult makeBlockedResult(int line, const WaitToken &token,
                               std::string reason);
  StepResult makeRejectedResult(const RejectedState &rejected);
  StepResult faultWithDiagnostic(const Diagnostic &diag);
  void emitDiagnostics(const std::vector<Diagnostic> &diagnostics);
  std::optional<RejectedLineEvent>
  makeRejectedEvent(const RejectedLine &rejected) const;
  void remapDiagnostics(std::vector<Diagnostic> *diagnostics, int line) const;
  void remapRejectedLines(std::vector<RejectedLine> *rejected_lines,
                          int line) const;
  AilExecutorInitialState exportInitialState() const;
  void importInitialState(const AilExecutorInitialState &state,
                          int next_line_number);

  IExecutionSink &sink_;
  IRuntime &runtime_;
  IExecutionRuntime *execution_runtime_ = nullptr;
  ICancellation &cancellation_;
  LowerOptions options_;
  EngineState state_ = EngineState::AcceptingInput;
  std::string input_buffer_;
  std::deque<PendingLine> pending_lines_;
  std::optional<BlockedState> blocked_;
  std::optional<RejectedState> rejected_;
  std::optional<RejectedState> deferred_rejected_;
  std::unique_ptr<AilExecutor> active_executor_;
  int active_executor_line_ = 0;
  size_t active_executor_emitted_diagnostics_ = 0;
  int next_line_number_ = 1;
  bool input_finished_ = false;
  std::string current_motion_code_ = "G1";
  WorkingPlane current_working_plane_ = WorkingPlane::XY;
  RapidInterpolationMode current_rapid_mode_ = RapidInterpolationMode::Linear;
  ToolRadiusCompMode current_tool_radius_comp_ = ToolRadiusCompMode::Off;
  std::optional<ToolSelectionState> current_active_tool_selection_;
  std::optional<ToolSelectionState> current_pending_tool_selection_;
  std::unordered_map<std::string, double> current_user_variables_;

  friend class ExecutionSession;
};

} // namespace gcode
