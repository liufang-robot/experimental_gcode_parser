#pragma once

#include <optional>
#include <string_view>

#include "execution_commands.h"

namespace gcode {

class IExecutionSink {
public:
  virtual ~IExecutionSink() = default;

  virtual void onDiagnostic(const Diagnostic &diag) = 0;
  virtual void onRejectedLine(const RejectedLineEvent &event) = 0;
  virtual void onLinearMove(const LinearMoveCommand &cmd) = 0;
  virtual void onArcMove(const ArcMoveCommand &cmd) = 0;
  virtual void onDwell(const DwellCommand &cmd) = 0;
};

class IRuntime {
public:
  virtual ~IRuntime() = default;

  virtual RuntimeResult<WaitToken>
  submitLinearMove(const LinearMoveCommand &cmd) = 0;
  virtual RuntimeResult<WaitToken> submitArcMove(const ArcMoveCommand &cmd) = 0;
  virtual RuntimeResult<WaitToken> submitDwell(const DwellCommand &cmd) = 0;
  virtual RuntimeResult<double> readSystemVariable(std::string_view name) = 0;
  virtual RuntimeResult<WaitToken> cancelWait(const WaitToken &token) = 0;
};

class ICancellation {
public:
  virtual ~ICancellation() = default;
  virtual bool isCancelled() const = 0;
};

} // namespace gcode
