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
  if (engine_dirty_ && !syncEngineWithEditableSuffix()) {
    StepResult result;
    result.status = StepStatus::Faulted;
    return result;
  }
  if (editable_lines_.empty()) {
    if (input_finished_) {
      return runEngineStep(engine_->finish());
    }
    state_ = EngineState::ReadyToExecute;
    StepResult result;
    result.status = StepStatus::Progress;
    return result;
  }
  return runEngineStep(input_finished_ ? engine_->finish() : engine_->pump());
}

StepResult ExecutionSession::finish() {
  input_finished_ = true;
  if (!input_buffer_.empty()) {
    editable_lines_.push_back(input_buffer_);
    input_buffer_.clear();
    engine_dirty_ = true;
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
  rejected_.reset();
  in_flight_line_count_ = 0;
  state_ = EngineState::Cancelled;
}

bool ExecutionSession::replaceEditableSuffix(
    std::string_view replacement_text) {
  if (state_ != EngineState::Rejected) {
    return false;
  }
  editable_lines_.clear();
  input_buffer_.clear();
  appendReplacementText(replacement_text);
  rejected_.reset();
  rebuildEngineFromLockedPrefix();
  engine_dirty_ = true;
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
  bool appended = false;
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
    appended = true;
    start = newline_pos + 1;
  }
  input_buffer_.erase(0, start);
  if (appended) {
    engine_dirty_ = true;
  }
  return true;
}

void ExecutionSession::appendReplacementText(std::string_view text) {
  input_buffer_.append(text.data(), text.size());
  (void)enqueueCompleteLinesFromBuffer();
  if (input_finished_ && !input_buffer_.empty()) {
    editable_lines_.push_back(input_buffer_);
    input_buffer_.clear();
    engine_dirty_ = true;
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
  engine_dirty_ = false;
  in_flight_line_count_ = 0;
}

bool ExecutionSession::syncEngineWithEditableSuffix() {
  rebuildEngineFromLockedPrefix();
  if (editable_lines_.empty()) {
    return true;
  }

  std::string text;
  for (const auto &line : editable_lines_) {
    text += line;
    text.push_back('\n');
  }
  in_flight_line_count_ = editable_lines_.size();
  return engine_->pushChunk(text);
}

StepResult ExecutionSession::runEngineStep(const StepResult &result) {
  if (result.status == StepStatus::Rejected) {
    rejected_ = result.rejected;
    if (rejected_.has_value()) {
      const int accepted_lines_before_rejection =
          rejected_->source.line - 1 -
          static_cast<int>(locked_prefix_lines_.size());
      if (accepted_lines_before_rejection > 0) {
        commitAcceptedEditablePrefix(
            static_cast<size_t>(accepted_lines_before_rejection));
      }
    }
    in_flight_line_count_ = 0;
    state_ = EngineState::Rejected;
    return result;
  }
  if (result.status == StepStatus::Blocked) {
    state_ = EngineState::Blocked;
    return result;
  }
  if (result.status == StepStatus::Faulted) {
    in_flight_line_count_ = 0;
    state_ = EngineState::Faulted;
    return result;
  }
  if (result.status == StepStatus::Cancelled) {
    in_flight_line_count_ = 0;
    state_ = EngineState::Cancelled;
    return result;
  }

  if (in_flight_line_count_ != 0 &&
      (engine_->state() == EngineState::ReadyToExecute ||
       engine_->state() == EngineState::Completed)) {
    commitAcceptedEditablePrefix(in_flight_line_count_);
  }

  if (result.status == StepStatus::Completed) {
    state_ = EngineState::Completed;
    return result;
  }

  state_ = engine_->state();
  return result;
}

void ExecutionSession::commitAcceptedEditablePrefix(size_t line_count) {
  const size_t accepted = std::min(line_count, editable_lines_.size());
  for (size_t i = 0; i < accepted; ++i) {
    locked_prefix_lines_.push_back(editable_lines_.front());
    editable_lines_.pop_front();
  }
  prefix_state_ = engine_->exportInitialState();
  if (in_flight_line_count_ >= accepted) {
    in_flight_line_count_ -= accepted;
  } else {
    in_flight_line_count_ = 0;
  }
  engine_dirty_ = !editable_lines_.empty();
}

} // namespace gcode
