# Experimental G-code Parser (C++ / ANTLR4)

ANTLR-based parser/lowering library for CNC G-code with:

- AST + line/column diagnostics
- AIL as executable IR
- execution through a single public `ExecutionSession` API
- halt-fix-continue recovery through `ExecutionSession`
- normalized runtime-facing commands and injected machine interfaces
- execution contract fixtures and review-site generation for supported
  `ExecutionSession` traces

Current source-of-truth docs:

- `docs/src/product/spec.md` (behavior/spec contract)
- `docs/src/product/prd.md` (product requirements and API expectations)
- `docs/src/` mdBook sources (canonical project docs)

## Implemented Status

- Parser: words/comments/line numbers and syntax diagnostics.
- Semantic rules: motion-family exclusivity, `G1` mode mixing, `G4` constraints,
  reviewed control-flow semantics.
- Execution session:
  - `ExecutionSession`
  - `gcode_exec_session` fake-log CLI for rejected-line recovery review
  - `gcode_execution_contract_review` fixture and HTML review CLI
- Internal execution engine:
  - `gcode_stream_exec` fake-log CLI for internal engine/event inspection

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

Current supported public library surfaces are:

- parser/AST
- AIL
- execution runtime APIs

The primary public execution model is `ExecutionSession` with injected
interfaces:

- execution sink interface for deterministic emitted events
- runtime interface for slow/blocking external work such as motion submission
  and variable reads
- cancellation interface for cooperative stop

Key docs for the execution model:

- `docs/src/development/design/streaming_execution_architecture.md`
- `docs/src/product/spec.md` section 6 / 6.1 / 6.2
- `docs/src/execution_workflow.md`
- `docs/src/execution_contract_review.md`
- `docs/src/product/program_reference.md`

Execution-session shape:

```cpp
MyExecutionSink sink;
MyRuntime runtime;
MyCancellation cancellation;

ExecutionSession session(sink, runtime, cancellation);
session.pushChunk("N10 G1 X10 Y20 F100\n");
session.pushChunk("N20 G4 F3\n");

auto step = session.finish();
while (step.status == StepStatus::Progress) {
  step = session.pump();
}

if (step.status == StepStatus::Blocked) {
  session.resume(step.blocked->token);
}
```

Control-flow examples are documented in:

- `docs/src/execution_workflow.md`
  - `GOTO`
  - `IF / ELSE / ENDIF`
- `docs/src/product/program_reference.md`

Recovery demo CLI:

```bash
./build/gcode_exec_session --format debug \
  --replace-editable-suffix testdata/execution/session_fix_suffix.ngc \
  testdata/execution/session_reject.ngc
```

If you are starting from zero and just want the practical integration steps,
read:

- `docs/src/execution_workflow.md`
- `docs/src/execution_contract_review.md`
- `docs/src/product/program_reference.md`

Execution contract review CLI:

```bash
./build/gcode_execution_contract_review \
  --fixtures-root testdata/execution_contract/core \
  --output-root output/execution_contract_review
```

The internal engine-inspection CLI still exists for development and tests:

```bash
./build/gcode_stream_exec --format debug testdata/messages/g4_dwell.ngc
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
./dev/build_docs_site.sh
mdbook serve docs --open
```

Mermaid diagrams in `docs/src/development/design/*.md` require
`mdbook-mermaid`. Without it, the generated book will show raw `mermaid` code
blocks instead of rendered diagrams.

Main pages:

- `docs/src/development/index.md`
- `docs/src/product/program_reference.md`

GitHub Pages publishes `docs/book` on pushes to `main`, including the generated
execution contract review subsite under
`docs/book/generated/execution-contract-review/`.

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
- prefer folding tiny leaked support types into the owning public header rather
  than creating one-header-per-type public includes
- if only shared enums leak through public APIs, prefer a narrow public enum
  header over promoting the whole internal support header

## Contribution Notes

- Follow OODA flow using `docs/src/project/roadmap.md`,
  `docs/src/project/backlog.md`, and `docs/src/development/ooda.md`.
- Every change must update `CHANGELOG_AGENT.md`.
- Behavior/API changes must update `docs/src/product/spec.md` and relevant
  mdBook pages in the same PR.
