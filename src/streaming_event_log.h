#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "gcode/execution_interfaces.h"

namespace gcode {

class EventLogRecorder {
public:
  void add(nlohmann::json entry);
  const std::vector<nlohmann::json> &entries() const { return entries_; }
  std::string toJsonLines() const;
  std::string toDebugText() const;

private:
  std::vector<nlohmann::json> entries_;
};

class RecordingExecutionSink : public IExecutionSink {
public:
  explicit RecordingExecutionSink(EventLogRecorder &recorder)
      : recorder_(recorder) {}

  void onDiagnostic(const Diagnostic &diag) override;
  void onRejectedLine(const RejectedLineEvent &event) override;
  void onLinearMove(const LinearMoveCommand &cmd) override;
  void onArcMove(const ArcMoveCommand &cmd) override;
  void onDwell(const DwellCommand &cmd) override;

private:
  EventLogRecorder &recorder_;
};

class ReadyRuntimeRecorder : public IRuntime {
public:
  explicit ReadyRuntimeRecorder(EventLogRecorder &recorder)
      : recorder_(recorder) {}

  RuntimeResult<WaitToken>
  submitLinearMove(const LinearMoveCommand &cmd) override;
  RuntimeResult<WaitToken> submitArcMove(const ArcMoveCommand &cmd) override;
  RuntimeResult<WaitToken> submitDwell(const DwellCommand &cmd) override;
  RuntimeResult<double> readSystemVariable(std::string_view name) override;
  RuntimeResult<WaitToken> cancelWait(const WaitToken &token) override;

private:
  EventLogRecorder &recorder_;
};

class NeverCancelled : public ICancellation {
public:
  bool isCancelled() const override { return false; }
};

} // namespace gcode
