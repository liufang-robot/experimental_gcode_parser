#pragma once

#include <functional>
#include <optional>
#include <string>

namespace gcode {

struct WaitToken {
  std::string kind;
  std::string id;

  bool operator==(const WaitToken &other) const {
    return kind == other.kind && id == other.id;
  }
};

struct WaitTokenHash {
  size_t operator()(const WaitToken &token) const {
    return std::hash<std::string>{}(token.kind) ^
           (std::hash<std::string>{}(token.id) << 1U);
  }
};

enum class RuntimeCallStatus { Ready, Pending, Error };

template <typename T> struct RuntimeResult {
  RuntimeCallStatus status = RuntimeCallStatus::Error;
  std::optional<T> value;
  std::optional<WaitToken> wait_token;
  std::string error_message;
};

template <> struct RuntimeResult<void> {
  RuntimeCallStatus status = RuntimeCallStatus::Error;
  std::optional<WaitToken> wait_token;
  std::string error_message;
};

} // namespace gcode
