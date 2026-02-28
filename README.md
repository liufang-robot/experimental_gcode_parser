# Experimental G-code Parser (C++ / ANTLR4)

ANTLR-based parser/lowering library for CNC G-code with:

- AST + line/column diagnostics
- queue-oriented typed messages (`G1`, `G2`, `G3`, `G4`)
- per-message modal metadata (`group`, `code`, `updates_state`)
- batch and streaming APIs
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
- `mdbook` (for docs)

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
./build/gcode_parse --format debug testdata/combined_samples.ngc
./build/gcode_parse --format json testdata/combined_samples.ngc
```

## Library Usage

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
mdbook build docs
mdbook serve docs --open
```

Main pages:

- `docs/src/development_reference.md`
- `docs/src/program_reference.md`

GitHub Pages publishes `docs/book` on pushes to `main`.

## Project Layout

- `src/` implementation
- `test/` GoogleTest suites
- `testdata/` golden/assets
- `docs/` mdBook sources
- `dev/` local scripts

## Contribution Notes

- Follow OODA flow using `ROADMAP.md`, `BACKLOG.md`, and `OODA.md`.
- Every change must update `CHANGELOG_AGENT.md`.
- Behavior/API changes must update `SPEC.md` and relevant mdBook pages in the
  same PR.
