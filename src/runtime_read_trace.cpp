#include "runtime_read_trace.h"

namespace gcode {
namespace {

thread_local const SourceInfo *g_current_runtime_read_trace_source = nullptr;

} // namespace

ScopedRuntimeReadTraceSource::ScopedRuntimeReadTraceSource(
    const SourceInfo &source)
    : previous_(g_current_runtime_read_trace_source) {
  g_current_runtime_read_trace_source = &source;
}

ScopedRuntimeReadTraceSource::~ScopedRuntimeReadTraceSource() {
  g_current_runtime_read_trace_source = previous_;
}

const SourceInfo *currentRuntimeReadTraceSource() {
  return g_current_runtime_read_trace_source;
}

} // namespace gcode
