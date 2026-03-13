#include "gcode/execution_session.h"

#include <utility>

#include "streaming_execution_engine.h"

namespace gcode {

ExecutionSession::ExecutionSession(IExecutionSink &sink, IRuntime &runtime,
                                   ICancellation &cancellation,
                                   const LowerOptions &options)
    : sink_(sink), runtime_(runtime), cancellation_(cancellation),
      options_(options), engine_(std::make_unique<StreamingExecutionEngine>(
                             sink, runtime, cancellation, options)) {}

ExecutionSession::ExecutionSession(IExecutionSink &sink,
                                   IExecutionRuntime &runtime,
                                   ICancellation &cancellation,
                                   const LowerOptions &options)
    : ExecutionSession(sink, static_cast<IRuntime &>(runtime), cancellation,
                       options) {
  execution_runtime_ = &runtime;
  engine_ = std::make_unique<StreamingExecutionEngine>(sink, runtime,
                                                       cancellation, options);
}

ExecutionSession::~ExecutionSession() = default;

bool ExecutionSession::pushChunk(std::string_view chunk) {
  if (state_ == EngineState::Rejected || state_ == EngineState::Cancelled ||
      state_ == EngineState::Faulted || state_ == EngineState::Completed) {
    return false;
  }
  input_buffer_.append(chunk.data(), chunk.size());
  return enqueueCompleteLinesFromBuffer();
}

StepResult ExecutionSession::pump() {
  if (state_ == EngineState::Rejected) {
    StepResult result;
    result.status = StepStatus::Rejected;
    result.rejected = rejected_;
    return result;
  }
  if (state_ == EngineState::Faulted) {
    StepResult result;
    result.status = StepStatus::Faulted;
    return result;
  }
  if (state_ == EngineState::Cancelled) {
    StepResult result;
    result.status = StepStatus::Cancelled;
    return result;
  }
  if (state_ == EngineState::Blocked) {
    return runEngineStep(engine_->pump());
  }
  if (!current_line_.has_value() && !editable_lines_.empty()) {
    current_line_ = std::move(editable_lines_.front());
    editable_lines_.pop_front();
    std::string line = *current_line_;
    line.push_back('\n');
    (void)engine_->pushChunk(line);
  }
  if (!current_line_.has_value()) {
    if (input_finished_) {
      return runEngineStep(engine_->finish());
    }
    state_ = EngineState::ReadyToExecute;
    StepResult result;
    result.status = StepStatus::Progress;
    return result;
  }
  return runEngineStep(engine_->pump());
}

StepResult ExecutionSession::finish() {
  input_finished_ = true;
  if (!input_buffer_.empty()) {
    editable_lines_.push_back(input_buffer_);
    input_buffer_.clear();
  }
  return pump();
}

StepResult ExecutionSession::resume(const WaitToken &token) {
  if (state_ != EngineState::Blocked) {
    return runEngineStep(engine_->resume(token));
  }
  return runEngineStep(engine_->resume(token));
}

void ExecutionSession::cancel() {
  engine_->cancel();
  current_line_.reset();
  rejected_.reset();
  state_ = EngineState::Cancelled;
}

bool ExecutionSession::replaceEditableSuffix(
    std::string_view replacement_text) {
  if (state_ != EngineState::Rejected) {
    return false;
  }
  editable_lines_.clear();
  current_line_.reset();
  input_buffer_.clear();
  appendReplacementText(replacement_text);
  rejected_.reset();
  rebuildEngineFromLockedPrefix();
  state_ = EngineState::AcceptingInput;
  return true;
}

std::optional<int> ExecutionSession::rejectedLine() const {
  if (!rejected_.has_value()) {
    return std::nullopt;
  }
  return rejected_->source.line;
}

bool ExecutionSession::enqueueCompleteLinesFromBuffer() {
  size_t start = 0;
  while (start < input_buffer_.size()) {
    const size_t newline_pos = input_buffer_.find('\n', start);
    if (newline_pos == std::string::npos) {
      break;
    }
    std::string line = input_buffer_.substr(start, newline_pos - start);
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    editable_lines_.push_back(std::move(line));
    start = newline_pos + 1;
  }
  input_buffer_.erase(0, start);
  return true;
}

void ExecutionSession::appendReplacementText(std::string_view text) {
  input_buffer_.append(text.data(), text.size());
  (void)enqueueCompleteLinesFromBuffer();
  if (input_finished_ && !input_buffer_.empty()) {
    editable_lines_.push_back(input_buffer_);
    input_buffer_.clear();
  }
}

void ExecutionSession::rebuildEngineFromLockedPrefix() {
  if (execution_runtime_ != nullptr) {
    engine_ = std::make_unique<StreamingExecutionEngine>(
        sink_, *execution_runtime_, cancellation_, options_);
  } else {
    engine_ = std::make_unique<StreamingExecutionEngine>(
        sink_, runtime_, cancellation_, options_);
  }
  engine_->importInitialState(
      prefix_state_, static_cast<int>(locked_prefix_lines_.size() + 1));
}

StepResult ExecutionSession::runEngineStep(const StepResult &result) {
  if (result.status == StepStatus::Rejected) {
    rejected_ = result.rejected;
    if (current_line_.has_value()) {
      editable_lines_.push_front(*current_line_);
      current_line_.reset();
    }
    state_ = EngineState::Rejected;
    return result;
  }
  if (result.status == StepStatus::Blocked) {
    state_ = EngineState::Blocked;
    return result;
  }
  if (result.status == StepStatus::Faulted) {
    state_ = EngineState::Faulted;
    return result;
  }
  if (result.status == StepStatus::Cancelled) {
    state_ = EngineState::Cancelled;
    return result;
  }

  if (current_line_.has_value() &&
      (engine_->state() == EngineState::ReadyToExecute ||
       engine_->state() == EngineState::Completed)) {
    commitAcceptedCurrentLine();
  }

  if (result.status == StepStatus::Completed) {
    state_ = EngineState::Completed;
    return result;
  }

  state_ = engine_->state();
  return result;
}

void ExecutionSession::commitAcceptedCurrentLine() {
  locked_prefix_lines_.push_back(*current_line_);
  prefix_state_ = engine_->exportInitialState();
  current_line_.reset();
  rejected_.reset();
}

} // namespace gcode
