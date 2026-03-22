#pragma once

#include "gcode/lowering_types.h"

namespace gcode {

class ScopedRuntimeReadTraceSource {
public:
  explicit ScopedRuntimeReadTraceSource(const SourceInfo &source);
  ~ScopedRuntimeReadTraceSource();

  ScopedRuntimeReadTraceSource(const ScopedRuntimeReadTraceSource &) = delete;
  ScopedRuntimeReadTraceSource &
  operator=(const ScopedRuntimeReadTraceSource &) = delete;

private:
  const SourceInfo *previous_ = nullptr;
};

const SourceInfo *currentRuntimeReadTraceSource();

} // namespace gcode
