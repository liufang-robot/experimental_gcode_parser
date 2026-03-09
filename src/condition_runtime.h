#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

#include "runtime_status.h"

namespace gcode {

struct Condition;
struct SourceInfo;

enum class ConditionResolutionKind { True, False, Pending, Error };

struct ConditionResolution {
  ConditionResolutionKind kind = ConditionResolutionKind::Error;
  std::optional<WaitToken> wait_token;
  std::optional<int64_t> retry_at_ms;
  std::optional<std::string> error_message;
};

using ConditionResolver =
    std::function<ConditionResolution(const Condition &, const SourceInfo &)>;

class IConditionResolver {
public:
  virtual ~IConditionResolver() = default;
  virtual ConditionResolution resolve(const Condition &condition,
                                      const SourceInfo &source) const = 0;
};

class FunctionConditionResolver final : public IConditionResolver {
public:
  explicit FunctionConditionResolver(ConditionResolver resolver)
      : resolver_(std::move(resolver)) {}

  ConditionResolution resolve(const Condition &condition,
                              const SourceInfo &source) const override {
    return resolver_(condition, source);
  }

private:
  ConditionResolver resolver_;
};

} // namespace gcode
