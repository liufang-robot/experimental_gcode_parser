#pragma once

#include <functional>
#include <utility>

#include "gcode/ast.h"
#include "gcode/condition_runtime.h"
#include "gcode/execution_interfaces.h"

namespace gcode {

class IExecutionRuntime : public IRuntime, public IConditionResolver {
public:
  ~IExecutionRuntime() override = default;
  virtual RuntimeResult<double>
  evaluateExpression(const ExprNode &expression) = 0;
};

class FunctionExecutionRuntime final : public IExecutionRuntime {
public:
  using LinearMoveHandler =
      std::function<RuntimeResult<WaitToken>(const LinearMoveCommand &)>;
  using ArcMoveHandler =
      std::function<RuntimeResult<WaitToken>(const ArcMoveCommand &)>;
  using DwellHandler =
      std::function<RuntimeResult<WaitToken>(const DwellCommand &)>;
  using ReadUserVariableHandler =
      std::function<RuntimeResult<double>(std::string_view)>;
  using ReadVariableHandler =
      std::function<RuntimeResult<double>(std::string_view)>;
  using WriteVariableHandler =
      std::function<RuntimeResult<void>(std::string_view, double)>;
  using CancelWaitHandler =
      std::function<RuntimeResult<WaitToken>(const WaitToken &)>;
  using EvaluateExpressionHandler =
      std::function<RuntimeResult<double>(const ExprNode &)>;

  FunctionExecutionRuntime(
      ConditionResolver condition_resolver,
      LinearMoveHandler linear_move_handler, ArcMoveHandler arc_move_handler,
      DwellHandler dwell_handler,
      ReadUserVariableHandler read_user_variable_handler,
      ReadVariableHandler read_variable_handler,
      WriteVariableHandler write_variable_handler,
      CancelWaitHandler cancel_wait_handler,
      EvaluateExpressionHandler evaluate_expression_handler = {})
      : condition_resolver_(std::move(condition_resolver)),
        linear_move_handler_(std::move(linear_move_handler)),
        arc_move_handler_(std::move(arc_move_handler)),
        dwell_handler_(std::move(dwell_handler)),
        read_user_variable_handler_(std::move(read_user_variable_handler)),
        read_variable_handler_(std::move(read_variable_handler)),
        write_variable_handler_(std::move(write_variable_handler)),
        cancel_wait_handler_(std::move(cancel_wait_handler)),
        evaluate_expression_handler_(std::move(evaluate_expression_handler)) {}

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

  RuntimeResult<double> readVariable(std::string_view name) override {
    return read_user_variable_handler_(name);
  }

  RuntimeResult<void> writeVariable(std::string_view name,
                                    double value) override {
    return write_variable_handler_(name, value);
  }

  RuntimeResult<WaitToken> cancelWait(const WaitToken &token) override {
    return cancel_wait_handler_(token);
  }

  RuntimeResult<double>
  evaluateExpression(const ExprNode &expression) override {
    if (evaluate_expression_handler_) {
      return evaluate_expression_handler_(expression);
    }
    RuntimeResult<double> result;
    result.status = RuntimeCallStatus::Error;
    result.error_message =
        "execution runtime has no expression-evaluation handler configured";
    return result;
  }

private:
  ConditionResolver condition_resolver_;
  LinearMoveHandler linear_move_handler_;
  ArcMoveHandler arc_move_handler_;
  DwellHandler dwell_handler_;
  ReadUserVariableHandler read_user_variable_handler_;
  ReadVariableHandler read_variable_handler_;
  WriteVariableHandler write_variable_handler_;
  CancelWaitHandler cancel_wait_handler_;
  EvaluateExpressionHandler evaluate_expression_handler_;
};

} // namespace gcode
