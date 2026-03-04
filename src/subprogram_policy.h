#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "machine_profile.h"

namespace gcode {

enum class SubprogramSearchPolicy { ExactOnly, ExactThenBareName };

struct SubprogramResolution {
  bool resolved = false;
  std::string resolved_target;
  bool fallback_used = false;
  std::string message;
};

struct SubprogramPolicyOptions {
  SubprogramSearchPolicy search_policy = SubprogramSearchPolicy::ExactOnly;
  ErrorPolicy on_unresolved_target = ErrorPolicy::Error;
};

class SubprogramPolicy {
public:
  virtual ~SubprogramPolicy() = default;

  virtual SubprogramResolution
  resolveTarget(const std::string &requested_target,
                const std::unordered_map<std::string, std::vector<size_t>>
                    &label_positions) const = 0;

  virtual ErrorPolicy unresolvedPolicy() const = 0;
};

class DefaultSubprogramPolicy final : public SubprogramPolicy {
public:
  explicit DefaultSubprogramPolicy(SubprogramPolicyOptions options = {})
      : options_(options) {}

  SubprogramResolution
  resolveTarget(const std::string &requested_target,
                const std::unordered_map<std::string, std::vector<size_t>>
                    &label_positions) const override;

  ErrorPolicy unresolvedPolicy() const override {
    return options_.on_unresolved_target;
  }

private:
  SubprogramPolicyOptions options_;
};

} // namespace gcode
