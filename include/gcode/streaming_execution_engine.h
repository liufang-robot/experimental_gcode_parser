#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "gcode/ail.h"
#include "gcode/execution_interfaces.h"
#include "gcode/execution_runtime.h"
#include "gcode/messages.h"

namespace gcode {

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

private:
  struct PendingLine {
    int line = 0;
    std::string text;
  };

  bool enqueueCompleteLines();
  StepResult executeNextLine();
  StepResult advanceExecutor();
  StepResult makeWaitingForInputResult(int line, std::string reason);
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

  IExecutionSink &sink_;
  IRuntime &runtime_;
  IExecutionRuntime *execution_runtime_ = nullptr;
  ICancellation &cancellation_;
  LowerOptions options_;
  EngineState state_ = EngineState::AcceptingInput;
  std::string input_buffer_;
  std::deque<PendingLine> pending_lines_;
  std::optional<BlockedState> blocked_;
  std::unique_ptr<AilExecutor> executor_;
  std::vector<AilInstruction> buffered_instructions_;
  size_t executor_emitted_diagnostics_ = 0;
  int next_line_number_ = 1;
  bool input_finished_ = false;
};

} // namespace gcode
