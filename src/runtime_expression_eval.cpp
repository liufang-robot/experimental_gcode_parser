#include "runtime_expression_eval.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

namespace gcode {
namespace {

ConditionResolution makeConditionError(std::string message) {
  ConditionResolution resolution;
  resolution.kind = ConditionResolutionKind::Error;
  resolution.error_message = std::move(message);
  return resolution;
}

std::optional<bool> compareValues(double lhs, std::string_view op, double rhs) {
  if (op == "==") {
    return lhs == rhs;
  }
  if (op == "!=") {
    return lhs != rhs;
  }
  if (op == "<") {
    return lhs < rhs;
  }
  if (op == "<=") {
    return lhs <= rhs;
  }
  if (op == ">") {
    return lhs > rhs;
  }
  if (op == ">=") {
    return lhs >= rhs;
  }
  return std::nullopt;
}

ConditionResolution conditionFromComparisonResult(bool value) {
  ConditionResolution resolution;
  resolution.kind =
      value ? ConditionResolutionKind::True : ConditionResolutionKind::False;
  return resolution;
}

RuntimeResult<double> makeReadyValue(double value) {
  RuntimeResult<double> result;
  result.status = RuntimeCallStatus::Ready;
  result.value = value;
  return result;
}

RuntimeResult<double> makeErrorValue(std::string message) {
  RuntimeResult<double> result;
  result.status = RuntimeCallStatus::Error;
  result.error_message = std::move(message);
  return result;
}

RuntimeResult<double> applyUnary(std::string_view op, double operand) {
  if (op == "+") {
    return makeReadyValue(operand);
  }
  if (op == "-") {
    return makeReadyValue(-operand);
  }
  return makeErrorValue("unsupported runtime unary operator: " +
                        std::string(op));
}

RuntimeResult<double> applyBinary(std::string_view op, double lhs, double rhs) {
  if (op == "+") {
    return makeReadyValue(lhs + rhs);
  }
  if (op == "-") {
    return makeReadyValue(lhs - rhs);
  }
  if (op == "*") {
    return makeReadyValue(lhs * rhs);
  }
  if (op == "/") {
    if (std::abs(rhs) < 1e-12) {
      return makeErrorValue("runtime division by zero");
    }
    return makeReadyValue(lhs / rhs);
  }
  return makeErrorValue("unsupported runtime binary operator: " +
                        std::string(op));
}

} // namespace

RuntimeResult<double> evaluateRuntimeExpression(const ExprNode &node,
                                                IRuntime &runtime) {
  if (const auto *literal = std::get_if<ExprLiteral>(&node.node)) {
    return makeReadyValue(literal->value);
  }
  if (const auto *variable = std::get_if<ExprVariable>(&node.node)) {
    auto result = variable->is_system
                      ? runtime.readSystemVariable(variable->name)
                      : runtime.readVariable(variable->name);
    if (result.status == RuntimeCallStatus::Pending &&
        !result.wait_token.has_value()) {
      return makeErrorValue(
          std::string("pending ") + (variable->is_system ? "system" : "user") +
          " variable read missing wait token: " + variable->name);
    }
    return result;
  }
  if (const auto *unary = std::get_if<ExprUnary>(&node.node)) {
    const auto operand = evaluateRuntimeExpression(*unary->operand, runtime);
    if (operand.status != RuntimeCallStatus::Ready ||
        !operand.value.has_value()) {
      return operand;
    }
    return applyUnary(unary->op, *operand.value);
  }
  if (const auto *binary = std::get_if<ExprBinary>(&node.node)) {
    const auto lhs = evaluateRuntimeExpression(*binary->lhs, runtime);
    if (lhs.status != RuntimeCallStatus::Ready || !lhs.value.has_value()) {
      return lhs;
    }
    const auto rhs = evaluateRuntimeExpression(*binary->rhs, runtime);
    if (rhs.status != RuntimeCallStatus::Ready || !rhs.value.has_value()) {
      return rhs;
    }
    return applyBinary(binary->op, *lhs.value, *rhs.value);
  }
  return makeErrorValue("runtime-only expression evaluation encountered "
                        "unsupported expression node");
}

ConditionResolution resolveRuntimeCondition(const Condition &condition,
                                            IRuntime &runtime) {
  const bool any_structurally_incomplete_term =
      std::any_of(condition.terms.begin(), condition.terms.end(),
                  [](const Condition::Term &term) {
                    return !term.lhs || !term.rhs || term.op.empty();
                  });
  const bool missing_structured_operands =
      (condition.terms.empty() && (!condition.lhs || !condition.rhs)) ||
      any_structurally_incomplete_term;
  const bool parser_limited_logical_and =
      missing_structured_operands &&
      (condition.has_logical_and || !condition.and_terms_raw.empty() ||
       condition.raw_text.find("AND") != std::string::npos);
  if (parser_limited_logical_and) {
    return makeConditionError(
        "runtime-only condition evaluation requires structured comparison "
        "terms; use IExecutionRuntime for parenthesized or parser-limited "
        "conditions");
  }

  Condition::Term synthetic_term;
  if (condition.terms.empty()) {
    synthetic_term.lhs = condition.lhs;
    synthetic_term.op = condition.op;
    synthetic_term.rhs = condition.rhs;
    synthetic_term.location = condition.location;
  }

  auto evaluate_term =
      [&](const Condition::Term &term) -> std::optional<ConditionResolution> {
    if (!term.lhs || !term.rhs) {
      return makeConditionError("runtime condition is missing lhs or rhs");
    }

    const auto lhs = evaluateRuntimeExpression(*term.lhs, runtime);
    if (lhs.status == RuntimeCallStatus::Pending) {
      ConditionResolution resolution;
      resolution.kind = ConditionResolutionKind::Pending;
      resolution.wait_token = lhs.wait_token;
      return resolution;
    }
    if (lhs.status == RuntimeCallStatus::Error || !lhs.value.has_value()) {
      return makeConditionError(lhs.error_message.empty()
                                    ? "runtime condition lhs evaluation failed"
                                    : lhs.error_message);
    }

    const auto rhs = evaluateRuntimeExpression(*term.rhs, runtime);
    if (rhs.status == RuntimeCallStatus::Pending) {
      ConditionResolution resolution;
      resolution.kind = ConditionResolutionKind::Pending;
      resolution.wait_token = rhs.wait_token;
      return resolution;
    }
    if (rhs.status == RuntimeCallStatus::Error || !rhs.value.has_value()) {
      return makeConditionError(rhs.error_message.empty()
                                    ? "runtime condition rhs evaluation failed"
                                    : rhs.error_message);
    }

    const auto comparison = compareValues(*lhs.value, term.op, *rhs.value);
    if (!comparison.has_value()) {
      return makeConditionError("unsupported runtime comparison operator: " +
                                term.op);
    }
    if (!*comparison) {
      return conditionFromComparisonResult(false);
    }
    return std::nullopt;
  };

  if (condition.terms.empty()) {
    const auto single = evaluate_term(synthetic_term);
    if (single.has_value()) {
      return *single;
    }
    return conditionFromComparisonResult(true);
  }

  for (const auto &term : condition.terms) {
    const auto resolution = evaluate_term(term);
    if (resolution.has_value()) {
      return *resolution;
    }
  }
  return conditionFromComparisonResult(true);
}

} // namespace gcode
