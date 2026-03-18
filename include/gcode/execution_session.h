#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "gcode/execution_runtime.h"

namespace gcode {

class StreamingExecutionEngine;

class ExecutionSession {
public:
  ExecutionSession(IExecutionSink &sink, IRuntime &runtime,
                   ICancellation &cancellation,
                   const LowerOptions &options = {});
  ExecutionSession(IExecutionSink &sink, IExecutionRuntime &runtime,
                   ICancellation &cancellation,
                   const LowerOptions &options = {});
  ~ExecutionSession();

  bool pushChunk(std::string_view chunk);
  StepResult pump();
  StepResult finish();
  StepResult resume(const WaitToken &token);
  void cancel();

  bool replaceEditableSuffix(std::string_view replacement_text);

  EngineState state() const { return state_; }
  std::optional<int> rejectedLine() const;

private:
  bool enqueueCompleteLinesFromBuffer();
  void appendReplacementText(std::string_view text);
  void rebuildEngineFromLockedPrefix();
  bool syncEngineWithEditableSuffix();
  StepResult runEngineStep(const StepResult &result);
  void commitAcceptedEditablePrefix(size_t line_count);

  IExecutionSink &sink_;
  IRuntime &runtime_;
  IExecutionRuntime *execution_runtime_ = nullptr;
  ICancellation &cancellation_;
  LowerOptions options_;
  std::unique_ptr<StreamingExecutionEngine> engine_;
  std::vector<std::string> locked_prefix_lines_;
  std::deque<std::string> editable_lines_;
  std::string input_buffer_;
  bool input_finished_ = false;
  bool engine_dirty_ = false;
  EngineState state_ = EngineState::AcceptingInput;
  std::optional<RejectedState> rejected_;
  AilExecutorInitialState prefix_state_;
  size_t in_flight_line_count_ = 0;
};

} // namespace gcode
