# CHANGELOG_AGENT

## 2026-02-22
- Added `./dev/check.sh` to run format checks, build, tests, and sanitizer tests.
- Added CI workflow to run the same checks.
- Updated contributor rules to require `./dev/check.sh` evidence and test/spec coverage.

SPEC sections / tests:
- SPEC: N/A (process change only)
- Tests: `ctest --test-dir build --output-on-failure` via `./dev/check.sh`

Known limitations:
- Requires `clang-format`, `clang-tidy`, `antlr4`, and ANTLR4 C++ runtime installed.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-02-22 (follow-up)
- Ran `./dev/check.sh`, fixed `clang-format` violations in parser/test sources.
- Updated sanitizer test invocation to set `ASAN_OPTIONS=detect_leaks=0` for environments where LeakSanitizer is blocked by ptrace.
- Updated `CMakeLists.txt` to read `ANTLR4_RUNTIME_INCLUDE_DIR` and `ANTLR4_RUNTIME_LIB` from environment variables, matching CI configuration.
- Reordered `./dev/check.sh` so `cmake --build` runs before `clang-tidy`, ensuring generated ANTLR headers exist for tidy analysis.
- Added `dev/antlr4.sh` and CI ANTLR 4.10.1 tool pinning, and updated `CMakeLists.txt` to honor `ANTLR4_EXECUTABLE` from environment for consistent tool/runtime compatibility.

SPEC sections / tests:
- SPEC: Section 6 (Testing Expectations)
- Tests: `./dev/check.sh` including normal and sanitizer `ctest`

Known limitations:
- Leak detection is disabled during sanitizer `ctest` in `./dev/check.sh`; dedicated leak checks should run in a compatible environment.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-02-22 (ooda governance docs)
- Added `ROADMAP.md` with milestone-based direction and exit criteria.
- Added `BACKLOG.md` with prioritized tasks, acceptance criteria, and task template.
- Added `OODA.md` with loop rules, merge policy, and definition of done.
- Updated `AGENTS.md` to require agent use of `BACKLOG.md`, `ROADMAP.md`, and `OODA.md` before implementation starts.

SPEC sections / tests:
- SPEC: Section 6 (Testing Expectations) alignment and process enforcement
- Tests: no code-path change; governance/documentation-only update

Known limitations:
- Backlog items are initial and will need regular reprioritization as CI/runtime data evolves.

How to reproduce locally (commands):
- `git checkout feature/ooda-repo-loop`
- `sed -n '1,220p' ROADMAP.md`
- `sed -n '1,260p' BACKLOG.md`
- `sed -n '1,260p' OODA.md`

## 2026-02-23 (message lowering v0 slice)
- Added standalone message-lowering module that converts parsed AST into
  polymorphic queue messages, starting with `G1Message`.
- Added `G1Message` extraction fields: optional filename/line source,
  optional `N` line number, target pose `xyzabc`, and feed `F`.
- Added tests validating valid-line extraction and skip-error-line behavior
  (`G1 X10`, `G1 G2 X10`, `G1 X20`).

SPEC sections / tests:
- SPEC: Section 6 (Message Lowering), Section 7 (Testing Expectations)
- Tests: `messages_tests`, `parser_tests`, plus `./dev/check.sh`

Known limitations:
- Lowering currently emits only `G1Message`; `G2/G3` message types are backlog items.
- Resume-from-line is behaviorally supported by full reparse + skip-error policy; no dedicated incremental session API yet.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure`
- `./dev/check.sh`

## 2026-02-23 (message lowering explicit rejection policy)
- Added explicit `rejected_lines` reporting in message lowering results so
  library users can detect invalid non-emitted lines and reasons directly.
- Enforced fail-fast lowering behavior: stop at first error line.
- Added/updated tests to assert explicit rejection reporting and fail-fast behavior.
- Added lowercase-equivalence coverage (`x`/`X`, `g1`/`G1`) in message-lowering tests.
- Updated SPEC testing requirement to standardize on GoogleTest for new unit tests.

SPEC sections / tests:
- SPEC: Section 6 (Message Lowering), Section 7 (Testing Expectations)
- Tests: `messages_tests`, `./dev/check.sh`

Known limitations:
- Existing tests currently include a custom harness; migration to GoogleTest is pending.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-02-23 (agent policy and PR template)
- Updated `AGENTS.md` with explicit single-command quality gate (`./dev/check.sh`)
  and tooling config pointers.
- Added `.github/pull_request_template.md` with OODA sections:
  Observe/Orient/Decide/Act, evidence, and follow-ups.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: process/docs-only change; validated with `./dev/check.sh`

Known limitations:
- PR template quality depends on maintainers filling each section with concrete evidence.

How to reproduce locally (commands):
- `sed -n '1,260p' AGENTS.md`
- `sed -n '1,260p' .github/pull_request_template.md`

## 2026-02-23 (migrate tests to GoogleTest)
- Refactored `test/parser_tests.cpp` from custom harness to GoogleTest cases.
- Refactored `test/messages_tests.cpp` from custom harness to GoogleTest cases.
- Updated CMake test registration to `gtest_discover_tests` and CI dependencies
  to include `libgtest-dev`.

SPEC sections / tests:
- SPEC: Section 7 (Testing Expectations, GoogleTest requirement)
- Tests: parser and message test suites now run through GoogleTest via `ctest`

Known limitations:
- Existing golden snapshots remain plain text; only harness changed.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-02-23 (message JSON serialization with nlohmann/json)
- Added `src/messages_json.h` + `src/messages_json.cpp` with message result
  JSON serialization/deserialization APIs.
- Added `nlohmann/json` dependency integration in CMake and CI.
- Added JSON tests: round-trip, invalid-JSON diagnostic, and asset-based
  golden message output validation under `testdata/messages/`.

SPEC sections / tests:
- SPEC: Section 2.2, Section 6 (JSON schema notes), Section 7 (message JSON goldens)
- Tests: `messages_json_tests`, `messages_tests`, `parser_tests`, and `./dev/check.sh`

Known limitations:
- `fromJson` currently reconstructs supported message types (`G1`) only.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-02-23 (backlog task for G2/G3 lowering)
- Added backlog task `T-012` for implementing typed `G2`/`G3` message lowering.
- Updated backlog tracking to mark `T-011` done and clear in-progress slot.

SPEC sections / tests:
- SPEC: planned coverage target in Section 6 (message lowering) and Section 7 (tests)
- Tests: no code-path change in this update

Known limitations:
- This is planning/governance only; no `G2/G3` lowering implementation included yet.

How to reproduce locally (commands):
- `sed -n '1,260p' BACKLOG.md`
- `sed -n '1,320p' CHANGELOG_AGENT.md`

## 2026-02-23 (backlog task for message golden matrix)
- Added backlog task `T-013` to expand `testdata/messages` beyond a single fail-fast fixture.
- Defined acceptance criteria for a table-driven golden suite covering valid, mixed, and case-equivalence scenarios.

SPEC sections / tests:
- SPEC: planned coverage target in Section 7 (testing)
- Tests: no code-path change in this update

Known limitations:
- Planning update only; fixture expansion is not implemented in this change.

How to reproduce locally (commands):
- `sed -n '1,260p' BACKLOG.md`
- `sed -n '1,360p' CHANGELOG_AGENT.md`
