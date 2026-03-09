# Experimental G-code Parser (C++ / ANTLR4)

ANTLR-based parser/lowering library for CNC G-code with:

- AST + line/column diagnostics
- queue-oriented typed messages (`G1`, `G2`, `G3`, `G4`)
- per-message modal metadata (`group`, `code`, `updates_state`)
- batch and streaming compatibility APIs
- initial streaming-first execution engine for the motion subset with injected
  runtime interfaces
- JSON serialization for lowered message results

Current source-of-truth docs:

- `SPEC.md` (behavior/spec contract)
- `PRD.md` (product requirements and API expectations)
- `docs/` mdBook sources (developer and program reference)

## Implemented Status

- Parser: words/comments/line numbers and syntax diagnostics.
- Semantic rules: motion-family exclusivity, `G1` mode mixing, `G4` constraints.
- Lowering:
  - `G1Message` (pose + feed)
  - `G2Message` / `G3Message` (pose + arc + feed)
  - `G4Message` (dwell mode + value)
- Streaming callbacks:
  - `parseAndLowerStream(...)`
  - `parseAndLowerFileStream(...)`
  - note: current implementation parses the full input first, then emits
    callbacks while lowering; this is a transitional API rather than the final
    line-by-line execution surface
- Streaming execution engine:
  - `StreamingExecutionEngine`
  - `gcode_stream_exec` fake-log CLI for motion-subset event inspection
- Session/edit flow:
  - `ParseSession::applyLineEdit(...)`
  - `ParseSession::reparseFromLine(...)`
- Queue diff/apply helpers:
  - `diffMessagesByLine(...)`
  - `applyMessageDiff(...)`

## Prerequisites

- C++17 toolchain
- `cmake`
- `antlr4` tool + ANTLR4 C++ runtime
- `clang-format`, `clang-tidy`
- `gtest`
- `nlohmann/json`
- `mdbook` and `mdbook-mermaid` (for docs)

If ANTLR runtime is not in a default system path, set:

- `ANTLR4_RUNTIME_INCLUDE_DIR`
- `ANTLR4_RUNTIME_LIB`

For deterministic ANTLR tool version, CI uses `dev/antlr4.sh` with an explicit
jar path via `ANTLR4_EXECUTABLE` and `ANTLR4_JAR`.

## Build

```bash
cmake -S . -B build \
  -DANTLR4_RUNTIME_INCLUDE_DIR=/usr/include/antlr4-runtime \
  -DANTLR4_RUNTIME_LIB=/usr/lib/x86_64-linux-gnu/libantlr4-runtime.so
cmake --build build -j
```

## CLI Usage

```bash
./build/gcode_parse testdata/combined_samples.ngc
./build/gcode_parse --mode parse --format json testdata/combined_samples.ngc
./build/gcode_parse --mode ail --format json testdata/messages/g4_dwell.ngc
./build/gcode_parse --mode ail --format debug testdata/messages/g4_dwell.ngc
./build/gcode_parse --mode packet --format json testdata/packets/core_motion.ngc
./build/gcode_parse --mode packet --format debug testdata/packets/core_motion.ngc
./build/gcode_parse --mode lower --format json testdata/messages/g4_dwell.ngc
./build/gcode_parse --mode lower --format debug testdata/messages/g4_dwell.ngc
./build/gcode_parse --format debug testdata/combined_samples.ngc
```

## Library Usage

Current stable library surfaces are still the parse/lower batch APIs plus the
transitional callback-based streaming API.

The primary execution direction is a line-by-line streaming engine with
injected interfaces:

- execution sink interface for deterministic emitted events
- runtime interface for slow/blocking external work such as motion submission
  and variable reads
- cancellation interface for cooperative stop

Implemented in the current slice for `G0/G1/G2/G3/G4`:

- `docs/src/development/design/streaming_execution_architecture.md`
- `SPEC.md` section 6 / 6.1 / 6.2

Batch parse+lower:

```cpp
#include "messages.h"

gcode::LowerOptions options;
options.filename = "job.ngc";

const auto result = gcode::parseAndLower("N10 G1 X10 Y20 F100\n", options);
// result.messages, result.diagnostics, result.rejected_lines
```

Streaming parse+lower:

```cpp
#include "messages.h"

gcode::StreamCallbacks callbacks;
callbacks.on_message = [](const gcode::ParsedMessage& msg) {
  // process message incrementally
};
callbacks.on_diagnostic = [](const gcode::Diagnostic& d) {
  // record line/column diagnostics
};

gcode::LowerOptions options;
options.filename = "job.ngc";
gcode::parseAndLowerStream("G1 X10\nG4 F2\n", options, callbacks);
```

Streaming-first execution shape:

```cpp
MyExecutionSink sink;
MyRuntime runtime;
MyCancellation cancellation;

StreamingExecutionEngine engine(sink, runtime, cancellation);
engine.pushChunk("N10 G1 X10 Y20 F100\n");

auto step = engine.pump();
if (step.status == StepStatus::Blocked) {
  engine.resume(step.blocked->token);
}
```

Fake-log CLI for quick review:

```bash
./build/gcode_stream_exec --format debug testdata/messages/g4_dwell.ngc
./build/gcode_stream_exec --format json testdata/messages/g4_dwell.ngc
```

JSON conversion helpers:

```cpp
#include "messages_json.h"

std::string json = gcode::toJsonString(result, true);
gcode::MessageResult round_trip = gcode::fromJsonString(json);
```

## Quality Gate

Single-command gate:

```bash
./dev/check.sh
```

This runs format-check, configure/build, tests (including golden/regression/
fuzz-smoke), tidy, and sanitizer tests.

## Benchmark

```bash
./dev/bench.sh
./build/gcode_bench --iterations 5 --lines 10000 --output output/bench/latest.json
```

## Documentation (mdBook)

```bash
cargo install mdbook mdbook-mermaid
mdbook build docs
mdbook serve docs --open
```

Mermaid diagrams in `docs/src/development/design/*.md` require
`mdbook-mermaid`. Without it, the generated book will show raw `mermaid` code
blocks instead of rendered diagrams.

Main pages:

- `docs/src/development/index.md`
- `docs/src/program_reference.md`

GitHub Pages publishes `docs/book` on pushes to `main`.

## Project Layout

- `include/gcode/` public library headers
- `src/` implementation
- `test/` GoogleTest suites
- `testdata/` golden/assets
- `docs/` mdBook sources
- `dev/` local scripts

Header boundary direction:

- public headers should be included as `#include <gcode/...>`
- `src/` headers are internal unless explicitly re-exported through
  `include/gcode/`
- only stable library entry points belong in `include/gcode/`
- support/internal headers that are merely transitively needed by public APIs
  should remain under `src/` unless we intentionally promote them as
  standalone public includes
- if a public header exposes a support type in its API, that support type's
  header is part of the public surface too and should also live in
  `include/gcode/`

## Contribution Notes

- Follow OODA flow using `ROADMAP.md`, `BACKLOG.md`, and `OODA.md`.
- Every change must update `CHANGELOG_AGENT.md`.
- Behavior/API changes must update `SPEC.md` and relevant mdBook pages in the
  same PR.
