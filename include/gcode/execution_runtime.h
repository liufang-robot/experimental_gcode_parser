#pragma once

#include <functional>
#include <utility>

#include "gcode/condition_runtime.h"
#include "gcode/execution_interfaces.h"

namespace gcode {

class IExecutionRuntime : public IRuntime, public IConditionResolver {
public:
  ~IExecutionRuntime() override = default;
};

class FunctionExecutionRuntime final : public IExecutionRuntime {
public:
  using LinearMoveHandler =
      std::function<RuntimeResult<WaitToken>(const LinearMoveCommand &)>;
  using ArcMoveHandler =
      std::function<RuntimeResult<WaitToken>(const ArcMoveCommand &)>;
  using DwellHandler =
      std::function<RuntimeResult<WaitToken>(const DwellCommand &)>;
  using ReadVariableHandler =
      std::function<RuntimeResult<double>(std::string_view)>;
  using CancelWaitHandler =
      std::function<RuntimeResult<WaitToken>(const WaitToken &)>;

  FunctionExecutionRuntime(ConditionResolver condition_resolver,
                           LinearMoveHandler linear_move_handler,
                           ArcMoveHandler arc_move_handler,
                           DwellHandler dwell_handler,
                           ReadVariableHandler read_variable_handler,
                           CancelWaitHandler cancel_wait_handler)
      : condition_resolver_(std::move(condition_resolver)),
        linear_move_handler_(std::move(linear_move_handler)),
        arc_move_handler_(std::move(arc_move_handler)),
        dwell_handler_(std::move(dwell_handler)),
        read_variable_handler_(std::move(read_variable_handler)),
        cancel_wait_handler_(std::move(cancel_wait_handler)) {}

  ConditionResolution resolve(const Condition &condition,
                              const SourceInfo &source) const override {
    return condition_resolver_(condition, source);
  }

  RuntimeResult<WaitToken>
  submitLinearMove(const LinearMoveCommand &cmd) override {
    return linear_move_handler_(cmd);
  }

  RuntimeResult<WaitToken> submitArcMove(const ArcMoveCommand &cmd) override {
    return arc_move_handler_(cmd);
  }

  RuntimeResult<WaitToken> submitDwell(const DwellCommand &cmd) override {
    return dwell_handler_(cmd);
  }

  RuntimeResult<double> readSystemVariable(std::string_view name) override {
    return read_variable_handler_(name);
  }

  RuntimeResult<WaitToken> cancelWait(const WaitToken &token) override {
    return cancel_wait_handler_(token);
  }

private:
  ConditionResolver condition_resolver_;
  LinearMoveHandler linear_move_handler_;
  ArcMoveHandler arc_move_handler_;
  DwellHandler dwell_handler_;
  ReadVariableHandler read_variable_handler_;
  CancelWaitHandler cancel_wait_handler_;
};

} // namespace gcode
