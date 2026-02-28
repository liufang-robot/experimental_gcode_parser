# CHANGELOG_AGENT

## 2026-02-28 (T-028 AIL to MotionPacket stage)
- Added packet model + conversion stage (`parseLowerAndPacketize`) from AIL
  into deterministic motion packets with 1-based `packet_id`.
- Added packet JSON serialization (`packetToJsonString`) and packet golden
  fixtures under `testdata/packets/`.
- Added CLI packet stage mode: `gcode_parse --mode packet --format json|debug`.

SPEC sections / tests:
- SPEC: Section 2.2 (CLI output modes), Section 6 (packet stage),
  Section 7 (packet goldens + CLI coverage)
- Tests: `test/packet_tests.cpp`, `test/cli_tests.cpp`,
  `testdata/packets/*.golden.json`, `./dev/check.sh`

Known limitations:
- v0 packetization only emits motion/dwell packets; non-motion AIL
  instructions (for example assignment) are skipped with warning diagnostics.
- Packet JSON currently supports serialization only (no `fromJson` parser yet).

How to reproduce locally (commands):
- `./build/gcode_parse --mode packet --format json testdata/packets/core_motion.ngc`
- `./build/gcode_parse --mode packet --format debug testdata/packets/assignment_skip.ngc`
- `ctest --test-dir build --output-on-failure -R \"PacketTest|CliFormatTest\"`
- `./dev/check.sh`

## 2026-02-28 (T-029 expression AST for assignments)
- Extended grammar/parser to support assignment statements with expressions,
  including system-variable operands (example: `R1 = $P_ACT_X + 2*R2`).
- Added typed expression AST nodes (literal, variable/system-variable, unary,
  binary) and assignment AST storage in `Line`.
- Added AIL assignment instruction lowering using typed expression AST instead
  of raw expression strings.
- Extended AIL JSON/debug output to serialize assignment expression trees.
- Added parser, AIL, and CLI tests for assignment expression parsing/lowering.

SPEC sections / tests:
- SPEC: Section 3.6 (assignment syntax), Section 5 (syntax diagnostics),
  Section 6 (AIL assignment coverage)
- Tests: `parser_tests`, `ail_tests`, `cli_tests`, `./dev/check.sh`

Known limitations:
- Parenthesized subexpressions are not supported yet in v0.
- Expression support currently targets assignment statements only; runtime
  evaluation/execution is still out of scope.

How to reproduce locally (commands):
- `./build/gcode_parse --mode ail --format json <file.ngc>`
- `./build/gcode_parse --mode ail --format debug <file.ngc>`
- `ctest --test-dir build --output-on-failure -R \"ParserExpressionTest|AilTest|CliFormatTest\"`
- `./dev/check.sh`

## 2026-02-28 (T-026 AIL intermediate representation v0)
- Added AIL IR model (`src/ail.h`, `src/ail.cpp`) with instruction variants:
  linear motion, arc motion, dwell, and placeholders for assignment/sync.
- Added AIL JSON export (`src/ail_json.h`, `src/ail_json.cpp`) with stable
  top-level schema (`schema_version`, `instructions`, `diagnostics`,
  `rejected_lines`).
- Extended CLI with `--mode ail --format json|debug` stage visibility.
- Added AIL unit tests and CLI tests for AIL mode.
- Updated specs/docs to describe parse -> AIL -> lower visibility.

SPEC sections / tests:
- SPEC: Section 2.2 (CLI modes), Section 6 (pipeline/lowering notes)
- Tests: `test/ail_tests.cpp`, `test/cli_tests.cpp`, `./dev/check.sh`

Known limitations:
- AIL v0 currently covers the existing motion subset derived from message
  lowering and includes assignment/sync as placeholder instruction types only.

How to reproduce locally (commands):
- `./build/gcode_parse --mode ail --format json testdata/messages/g4_dwell.ngc`
- `./build/gcode_parse --mode ail --format debug testdata/messages/g4_dwell.ngc`
- `ctest --test-dir build --output-on-failure -R \"AilTest|CliFormatTest\"`
- `./dev/check.sh`

## 2026-02-28 (pipeline roadmap backlog: AIL -> packets)
- Updated `BACKLOG.md` after `T-025` merge (`PR #36`).
- Added new queued tasks `T-026` to `T-030` to evolve architecture toward:
  `Source G-code -> parse -> AIL -> MotionPacket`.
- Marked `T-026` as current in-progress planning/implementation slice.

SPEC sections / tests:
- SPEC: planning targets for Section 2.2, Section 3, Section 5, Section 6, Section 7
- Tests: no code-path change (planning/backlog update only)

Known limitations:
- This update defines the work plan only; AIL/packet implementation is not part
  of this change.

How to reproduce locally (commands):
- `sed -n '1,320p' BACKLOG.md`

## 2026-02-28 (T-025 CLI lower mode)
- Extended `gcode_parse` with `--mode parse|lower` (default `parse`).
- Added lower mode outputs:
  - `--mode lower --format json` emits lowered `MessageResult` JSON.
  - `--mode lower --format debug` emits stable human-readable summary lines.
- Added CLI tests for lower mode JSON/debug outputs and unsupported-mode error.
- Updated `SPEC.md`, README, and development docs for the new CLI behavior.

SPEC sections / tests:
- SPEC: Section 2.2 (CLI modes/formats), Section 7 (CLI test expectation)
- Tests: `test/cli_tests.cpp`, `./dev/check.sh`

Known limitations:
- Lower debug output is line-oriented summary text, not a formal schema.
- No streaming CLI mode yet.

How to reproduce locally (commands):
- `./build/gcode_parse --mode lower --format json testdata/messages/g4_dwell.ngc`
- `./build/gcode_parse --mode lower --format debug testdata/messages/g4_dwell.ngc`
- `ctest --test-dir build --output-on-failure -R CliFormatTest`
- `./dev/check.sh`

## 2026-02-28 (backlog: add T-025 CLI lower mode task)
- Added backlog task `T-025` to implement CLI lower mode
  (`--mode lower --format json|debug`) for file-to-message output.
- Synced backlog state to mark `T-024` done (`PR #35`).

SPEC sections / tests:
- SPEC: planning target references Section 2.2, Section 6, and Section 7
- Tests: no code-path change (backlog planning/sync only)

Known limitations:
- This is planning only; CLI lower mode is not implemented in this change.

How to reproduce locally (commands):
- `sed -n '1,280p' BACKLOG.md`

## 2026-02-28 (T-024 modal-group metadata on messages)
- Added modal metadata to all lowered message types (`G1/G2/G3/G4`):
  `modal.group`, `modal.code`, and `modal.updates_state`.
- Implemented Siemens-preferred baseline mapping for supported functions:
  `G1/G2/G3 -> GGroup1 (updates_state=true)`, `G4 -> GGroup2 (updates_state=false)`.
- Extended JSON serialization/deserialization and message JSON goldens to
  include modal metadata.
- Added/updated unit tests for lowering families, message extraction,
  streaming callbacks, and JSON modal fields.

SPEC sections / tests:
- SPEC: Section 6 (Message Lowering modal metadata + JSON schema notes),
  Section 9 (reference docs alignment)
- Tests: `messages_tests`, `streaming_tests`, `messages_json_tests`,
  `lowering_family_g{1,2,3,4}_tests`, message JSON goldens, `./dev/check.sh`

Known limitations:
- v0.1 provides modal metadata for supported functions only; it does not
  implement full multi-group modal conflict validation or machine-specific
  modal behavior beyond this baseline mapping.

How to reproduce locally (commands):
- `./dev/check.sh`
- `ctest --test-dir build --output-on-failure -R \"MessagesJsonTest|MessagesTest|StreamingTest|G[1-4]LowererTest\"`

## 2026-02-27 (backlog sync after T-023 merge)
- Updated `BACKLOG.md` to mark `T-023` done (`PR #33`) and clear stale Ready
  Queue / In Progress entries.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: not applicable (backlog state sync only)

Known limitations:
- Remaining backlog work is currently in Icebox only.

How to reproduce locally (commands):
- `sed -n '1,260p' BACKLOG.md`

## 2026-02-27 (T-023 README rewrite)
- Rewrote `README.md` into a comprehensive, implementation-aligned guide:
  feature status, prerequisites, build/test/benchmark/docs workflow, and API
  entry points.
- Added concrete usage snippets for batch parse/lower, streaming callbacks, and
  JSON conversion helpers.
- Added contribution/documentation alignment notes linking `SPEC.md`, `PRD.md`,
  and mdBook policy.

SPEC sections / tests:
- SPEC: Section 6 (Message Lowering summary references), Section 9 (Documentation Policy)
- Tests: `./dev/check.sh`

Known limitations:
- README examples are intentionally minimal and are not compiled as doctests.

How to reproduce locally (commands):
- `./dev/check.sh`
- `sed -n '1,260p' README.md`

## 2026-02-27 (backlog: add README rewrite task T-023)
- Added backlog task `T-023` in Ready Queue to rewrite `README.md` as a
  comprehensive, implementation-aligned guide.

SPEC sections / tests:
- SPEC: planning target references Section 9 and Section 6
- Tests: no code-path change (backlog planning update only)

Known limitations:
- This adds planning/task definition only; README rewrite implementation is not
  included in this change.

How to reproduce locally (commands):
- `sed -n '1,260p' BACKLOG.md`

## 2026-02-27 (backlog sync after T-022 merge)
- Updated `BACKLOG.md` to move `T-022` out of Ready/In Progress and into Done
  with `PR #31`.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: not applicable (backlog state sync only)

Known limitations:
- Remaining backlog items (`coverage threshold policy`, `multi-file include`)
  are still pending future scoped PRs.

How to reproduce locally (commands):
- `sed -n '1,260p' BACKLOG.md`

## 2026-02-27 (T-022 mdBook docs + Pages pipeline)
- Added mdBook documentation source tree under `docs/` with:
  - `docs/src/development_reference.md`
  - `docs/src/program_reference.md`
- Added CI docs build job and GitHub Pages deploy job from `main` in
  `.github/workflows/ci.yml`.
- Updated `SPEC.md` documentation policy to require mdBook updates on code/API
  behavior changes.
- Added generated docs output ignore rule (`docs/book`) in `.gitignore`.

SPEC sections / tests:
- SPEC: Section 9 (Documentation Policy), Section 7 (CI/testing expectations reference)
- Tests: CI docs build step (`mdbook build docs`)

Known limitations:
- This change establishes mdBook structure and publishing pipeline; it does not
  fully migrate all legacy markdown docs into mdBook in one pass.

How to reproduce locally (commands):
- `mdbook build docs`
- `mdbook serve docs --open`
- `./dev/check.sh`

## 2026-02-27 (CI dependency: gcovr)
- Added `gcovr` to CI apt dependency installation for both `build-test` and
  `benchmark-smoke` jobs in `.github/workflows/ci.yml`.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: not applicable (CI environment dependency update only)

Known limitations:
- Coverage thresholds/report publishing are not enabled yet; this change only
  prepares CI tooling availability.

How to reproduce locally (commands):
- `sudo apt-get update && sudo apt-get install -y gcovr`
- `gcovr --version`

## 2026-02-27 (backlog status sync after merged T-021)
- Updated `BACKLOG.md` to mark `T-021` as done (`PR #28`) and clear stale
  `Ready Queue` / `In Progress` references.
- Removed the stale "Performance benchmarking harness" item from Icebox because
  it was completed in `T-020`.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: not applicable (documentation/backlog status update only)

Known limitations:
- Icebox still contains larger roadmap items (`coverage policy`, `multi-file include`)
  that need separate scoped tasks before implementation.

How to reproduce locally (commands):
- `sed -n '1,260p' BACKLOG.md`
- `sed -n '1,120p' CHANGELOG_AGENT.md`

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

## 2026-02-23 (T-001 parser fuzz smoke gate)
- Added `fuzz_smoke_tests` GoogleTest target that runs parser/lowering smoke
  checks over deterministic corpus files and fixed-seed generated inputs.
- Added deterministic corpus assets under `testdata/fuzz/corpus/`.
- Integrated fuzz smoke execution into `./dev/check.sh` (normal + sanitizer runs).

SPEC sections / tests:
- SPEC: Section 7 (property/fuzz testing + runtime budget)
- Tests: `FuzzSmokeTest.CorpusAndGeneratedInputsDoNotCrashOrHang`,
  `./dev/check.sh`, CI `build-test`

Known limitations:
- This is a smoke gate, not coverage-guided fuzzing; it improves crash/hang
  detection but does not prove complete memory-boundedness.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R FuzzSmokeTest`
- `./dev/check.sh`

## 2026-02-23 (T-002 regression-test protocol)
- Added `test/regression_tests.cpp` with a concrete regression case:
  `Regression_MixedCartesianAndPolar_G1ReportsError`.
- Added regression test target wiring in CMake (`regression_tests`).
- Documented regression naming convention in `README.md`.
- Updated `OODA.md` Definition of Done to require bug/issue -> regression test linkage.

SPEC sections / tests:
- SPEC: Section 7 (regression test expectation)
- Tests: `RegressionTest.Regression_MixedCartesianAndPolar_G1ReportsError`,
  `./dev/check.sh`

Known limitations:
- Regression suite currently has one seed case; additional fixed bugs should
  extend this suite over time.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R RegressionTest`
- `./dev/check.sh`

## 2026-02-23 (T-003 actionable diagnostics)
- Improved parser diagnostics text to be actionable:
  - syntax diagnostics now use `syntax error:` prefix plus context hints
  - semantic diagnostics include correction guidance for motion conflicts and
    G1 cartesian/polar mixing
- Added parser unit coverage for actionable syntax + semantic diagnostics.
- Refreshed affected golden outputs and message JSON goldens for updated text.

SPEC sections / tests:
- SPEC: Section 5 (Diagnostics), Section 7 (testing)
- Tests: `ParserDiagnosticsTest.ActionableSyntaxAndSemanticMessages`,
  parser golden tests, message JSON golden test, `./dev/check.sh`

Known limitations:
- Syntax hinting currently pattern-matches common ANTLR/lexer messages and may
  remain generic for uncommon parser errors.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-02-23 (T-004 CLI JSON/debug output mode)
- Added CLI option `--format json|debug` to `gcode_parse` (default: `debug`).
- Added parse-result JSON formatter with stable top-level keys:
  `schema_version`, `program`, and `diagnostics`.
- Added CLI tests validating debug-mode golden compatibility and JSON output schema.
- Updated README examples and SPEC CLI output notes.

SPEC sections / tests:
- SPEC: Section 2.2 (CLI output modes), Section 7 (testing)
- Tests: `CliFormatTest.DebugFormatMatchesExistingGoldenOutput`,
  `CliFormatTest.JsonFormatOutputsStableSchema`, `./dev/check.sh`

Known limitations:
- JSON mode currently targets parse/diagnostic output only; message-lowering
  JSON remains in separate message serialization APIs.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R CliFormatTest`
- `./build/gcode_parse --format json testdata/g2g3_samples.ngc`
- `./dev/check.sh`

## 2026-02-23 (T-005 grammar nullable-tail warning cleanup)
- Updated `line_no_eol` grammar alternatives to disallow an empty match.
- Removed ANTLR warning about optional block matching empty string in `program`.
- Kept parser behavior stable for existing fixtures and tests.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: parser goldens, message tests, CLI tests, and `./dev/check.sh`

Known limitations:
- Grammar still accepts broad word/value forms by design in v0; this change
  only addresses nullable-tail warning cleanup.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `./dev/check.sh`

## 2026-02-23 (T-012 G2/G3 message lowering)
- Added typed `G2Message` and `G3Message` queue payloads with source info,
  target pose, arc params (`I/J/K/R`), and optional feed.
- Extended AST-to-message lowering to emit `G2`/`G3` messages and map `CR` to
  arc radius field `r`.
- Extended message JSON serialization/deserialization for `G2`/`G3` and `arc`.
- Added tests for G2/G3 extraction and G2/G3 message JSON round-trip/golden output.

SPEC sections / tests:
- SPEC: Section 2.2 (typed message support), Section 6 (message lowering schema),
  Section 7 (testing)
- Tests: `MessagesTest.G2G3Extraction`,
  `MessagesJsonTest.RoundTripWithG2G3PreservesResult`,
  `MessagesJsonTest.GoldenMessageOutput`, `./dev/check.sh`

Known limitations:
- Arc lowering currently captures `I/J/K/R` only; other arc forms (for example
  `AR`, `CIP`, `CT`) are parsed in AST but not lowered into dedicated fields.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R \"MessagesTest|MessagesJsonTest\"`
- `./dev/check.sh`

## 2026-02-23 (T-013 message golden matrix expansion)
- Expanded `testdata/messages` with additional representative fixtures:
  valid multi-line G1, lowercase equivalence, no-filename source case, and
  comments/blank-lines handling.
- Refactored message JSON golden test to table-driven fixture discovery over
  `testdata/messages/*.ngc` with `<name>.golden.json` pairing.
- Documented message fixture naming convention in SPEC Section 7.

SPEC sections / tests:
- SPEC: Section 7 (message fixture naming convention)
- Tests: `MessagesJsonTest.GoldenMessageOutput`, `./dev/check.sh`

Known limitations:
- Table-driven golden runner currently supports one special naming override:
  `nofilename_` fixtures run with `LowerOptions.filename` unset.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R MessagesJsonTest.GoldenMessageOutput`
- `./dev/check.sh`

## 2026-02-23 (T-008 resume-from-line session API)
- Added `ParseSession` API (`src/session.h/.cpp`) to support line edits and
  resume-style reparsing from a requested line index.
- Added `SessionTest` coverage for edit-and-resume flow after fixing an error
  line and deterministic message ordering after edits.
- Wired new `session_tests` target into CMake and test discovery.

SPEC sections / tests:
- SPEC: Section 6 (resume session API notes), Section 7 (testing)
- Tests: `SessionTest.EditAndResumeFromErrorLine`,
  `SessionTest.MessageOrderingIsDeterministicAfterEdit`, `./dev/check.sh`

Known limitations:
- Current implementation reparses full text internally while exposing
  resume-from-line API semantics; targeted incremental diff/apply remains a
  separate backlog item.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R SessionTest`
- `./dev/check.sh`

## 2026-02-23 (T-009 queue diff/apply API)
- Added line-keyed queue diff API in `src/message_diff.h/.cpp`:
  `diffMessagesByLine(before, after)` with `added`, `updated`, and `removed_lines`.
- Added queue apply helper `applyMessageDiff(current, diff)` that reconstructs
  deterministic line-ordered message list.
- Added `message_diff_tests` coverage for update/insert/delete edit scenarios.

SPEC sections / tests:
- SPEC: Section 6 (queue diff/apply API notes), Section 7 (testing)
- Tests: `MessageDiffTest.UpdateLineProducesUpdatedEntry`,
  `MessageDiffTest.InsertLineProducesAddedEntry`,
  `MessageDiffTest.DeleteLineProducesRemovedEntry`, `./dev/check.sh`

Known limitations:
- Diff/apply keys messages by source line only; future modal/state-aware
  semantics may require richer identity keys.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R MessageDiffTest`
- `./dev/check.sh`

## 2026-02-23 (T-010 diagnostics policy + severity mapping)
- Defined and implemented severity mapping at lowering stage:
  - `error` for fail-fast rejecting diagnostics
  - `warning` for non-fatal unsupported arc-word lowering limits
- Added arc-lowering warning diagnostics for unsupported words
  (`AR`, `AP`, `RP`, `CIP`, `CT`, `I1`, `J1`, `K1`) while still emitting
  supported `G2/G3` message fields.
- Added test coverage validating warning severity and partial emission behavior.

SPEC sections / tests:
- SPEC: Section 6 (severity mapping policy), Section 7 (testing)
- Tests: `MessagesTest.ArcUnsupportedWordsEmitWarningsButKeepMessage`,
  `./dev/check.sh`

Known limitations:
- Warning list is currently rule-based and explicit; future parser/lowering
  extensions may adjust which words are considered supported.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R ArcUnsupportedWordsEmitWarningsButKeepMessage`
- `./dev/check.sh`

## 2026-02-23 (backlog status finalize)
- Updated `BACKLOG.md` status only: moved `T-010` to Done and cleared In Progress.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: not applicable (docs-only)

Known limitations:
- None (status bookkeeping update only).

How to reproduce locally (commands):
- `sed -n '1,220p' BACKLOG.md`

## 2026-02-24 (SPEC architecture rule for OO per-family modules)
- Added SPEC architecture requirement to avoid monolithic parser/lowering files.
- Added explicit requirement for per-family module/class structure (`G1`,
  `G2/G3`, and future families) in SPEC Section 8.
- Added backlog task `T-014` to track refactor work toward this architecture.

SPEC sections / tests:
- SPEC: Section 8 (Code Architecture Requirements)
- Tests: no behavior change (docs/backlog update only)

Known limitations:
- This change defines architecture policy only; full source refactor is tracked
  separately as `T-014`.

How to reproduce locally (commands):
- `sed -n '1,320p' SPEC.md`
- `sed -n '1,220p' BACKLOG.md`

## 2026-02-24 (T-014 OO per-family module refactor)
- Refactored parser semantic validation into rule classes in
  `src/semantic_rules.cpp` (motion exclusivity + G1 coordinate mode rules).
- Refactored message lowering into per-family lowerer classes/files:
  `G1Lowerer`, `G2Lowerer`, `G3Lowerer`, with factory wiring and common helpers.
- Kept public parse/lowering APIs stable while removing monolithic family logic
  from `src/gcode_parser.cpp` and `src/messages.cpp`.

SPEC sections / tests:
- SPEC: Section 8 (Code Architecture Requirements)
- Tests: full suite via `./dev/check.sh`

Known limitations:
- Refactor focuses on parser semantic rules + message lowering family modules;
  full AST builder extraction from `src/gcode_parser.cpp` can be a future step.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `./dev/check.sh`

## 2026-02-24 (T-015 per-class unit tests for OO family modules)
- Added dedicated GoogleTest targets for OO module files: `semantic_rules`,
  `lowering_family_factory`, `lowering_family_g1`, `lowering_family_g2`, and
  `lowering_family_g3`.
- Added focused unit tests per class/module file under `test/` validating
  motion code mapping, field lowering, semantic diagnostics, and warning
  behavior.
- Updated `BACKLOG.md` status bookkeeping: `T-014` moved to Done and `T-015`
  tracked as In Progress on this branch.

SPEC sections / tests:
- SPEC: Section 7 (Testing Strategy), Section 8 (Code Architecture Requirements)
- Tests: `SemanticRulesTest.*`, `MotionFamilyFactoryTest.*`, `G1LowererTest.*`,
  `G2LowererTest.*`, `G3LowererTest.*`, plus `./dev/check.sh`

Known limitations:
- Per-class coverage is focused on family lowerers and semantic rules introduced
  by the OO split; AST/parser internals remain covered primarily by existing
  parser/message/golden tests.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R "(SemanticRulesTest|MotionFamilyFactoryTest|G1LowererTest|G2LowererTest|G3LowererTest)"`
- `./dev/check.sh`

## 2026-02-24 (T-016 targeted regression edge-case tests)
- Added targeted regression coverage for duplicate same-motion words in one line
  (`G1 G1 X...`) to ensure no false exclusivity error and successful lowering.
- Added regression coverage for syntax-error location reporting on unsupported
  characters (line/column assertion).
- Added regression coverage confirming non-motion G-codes (e.g., `G4`) do not
  emit motion queue messages in v0.

SPEC sections / tests:
- SPEC: Section 7 (Testing Strategy)
- Tests: `RegressionTest.Regression_DuplicateSameMotionWord_NoExclusiveError`,
  `RegressionTest.Regression_UnsupportedChars_HasAccurateLocation`,
  `RegressionTest.Regression_NonMotionGCode_DoesNotEmitMessage`,
  `./dev/check.sh`

Known limitations:
- These tests lock current v0 behavior; future motion-family expansion may
  intentionally change non-motion lowering expectations.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R RegressionTest`
- `./dev/check.sh`

## 2026-02-24 (T-017 long-input stress smoke test)
- Added deterministic long-input stress smoke test in
  `test/fuzz_smoke_tests.cpp` that parses and lowers a large valid program.
- Added assertions for no diagnostics/rejections, expected message count, and
  execution-time envelope to catch crash/hang regressions.
- Updated backlog status: marked `T-016` done and tracked `T-017` in progress.

SPEC sections / tests:
- SPEC: Section 7 (Testing Strategy)
- Tests: `FuzzSmokeTest.LongProgramInputDoesNotCrashOrHang`,
  `./dev/check.sh`

Known limitations:
- Stress threshold is a smoke-level guard, not a formal benchmark budget;
  absolute timing may vary by machine.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R FuzzSmokeTest`
- `./dev/check.sh`

## 2026-02-24 (backlog task added: T-018 G4 dwell support)
- Added new backlog task `T-018` to implement `G4` dwell support with
  `G4 F...` (seconds) and `G4 S...` (spindle revolutions) syntax.
- Defined acceptance criteria for parser/lowering behavior, diagnostics,
  message JSON support, and test coverage expectations.
- Updated stale backlog status for `T-017` to `PR #23`.

SPEC sections / tests:
- SPEC impact planned by task: Sections 3, 5, 6, 7, 8
- Tests planned by task: parser/messages/messages_json/regression + message
  goldens for G4

Known limitations:
- This change is planning-only; no parser or lowering behavior changed yet.

How to reproduce locally (commands):
- `sed -n '1,240p' BACKLOG.md`
- `sed -n '1,520p' CHANGELOG_AGENT.md`

## 2026-02-24 (SPEC update for planned G4 dwell support)
- Updated `SPEC.md` to include `G4` dwell syntax with `G4 F...` (seconds) and
  `G4 S...` (master-spindle revolutions).
- Added diagnostics requirement that `G4` be programmed in a separate NC block
  with explicit correction-hint messaging.
- Extended message-lowering and JSON schema sections to define `G4Message`
  fields (`dwell_mode`, `dwell_value`, source metadata).

SPEC sections / tests:
- SPEC: Sections 1, 2, 3.5, 5, and 6
- Tests: planned under task `T-018` (no code/test behavior change in this docs-only commit)

Known limitations:
- This is a spec-definition update only; parser/lowering implementation for
  `G4Message` remains pending in `T-018`.

How to reproduce locally (commands):
- `sed -n '1,320p' SPEC.md`
- `sed -n '480,620p' CHANGELOG_AGENT.md`

## 2026-02-24 (program reference manual policy + initial manual)
- Added `SPEC.md` Section 9 defining a program-reference maintenance policy,
  including status labels (`Implemented`/`Partial`/`Planned`) and PR update
  requirements.
- Added `PROGRAM_REFERENCE.md` as the implementation snapshot manual for command
  syntax, lowering output, diagnostics, examples, and test references.
- Documented `G4` explicitly as `Planned` in the reference manual so product
  docs track current code behavior vs planned SPEC behavior.

SPEC sections / tests:
- SPEC: Section 9 (Program Reference Manual Policy)
- Tests: not applicable (docs-only change)

Known limitations:
- Manual is intentionally hand-maintained; no automatic doc generation from
  parser grammar/tests yet.

How to reproduce locally (commands):
- `sed -n '1,380p' SPEC.md`
- `sed -n '1,320p' PROGRAM_REFERENCE.md`
- `sed -n '520,700p' CHANGELOG_AGENT.md`

## 2026-02-24 (T-018 G4 dwell support)
- Implemented `G4` dwell support end-to-end with new `G4Message` type, OO
  lowerer module (`src/lowering_family_g4.h/.cpp`), and motion-family factory
  registration.
- Added semantic diagnostics for G4 block rules: separate-block enforcement,
  exactly-one dwell mode (`F` xor `S`), and numeric dwell value validation.
- Extended message JSON and diff/apply paths for `G4Message`, and added G4
  tests + golden fixtures (`testdata/messages/g4_*.ngc/.golden.json`).

SPEC sections / tests:
- SPEC: Section 3.5 (G4 syntax), Section 5 (diagnostics), Section 6 (message
  lowering/JSON), Section 7 (testing)
- Tests: `MessagesTest.G4ExtractionSecondsAndRevolutions`,
  `MessagesTest.G4MustBeSeparateBlockAndFailFast`,
  `MessagesJsonTest.RoundTripWithG4PreservesResult`,
  `SemanticRulesTest.ReportsG4NotSeparateBlock`,
  `SemanticRulesTest.ReportsG4RequiresExactlyOneModeWord`,
  `G4LowererTest.*`, `MessageDiffTest.G4LineUpdateIsTrackedAsUpdatedEntry`,
  `RegressionTest.Regression_G4RequiresSeparateBlock`,
  `MessagesJsonTest.GoldenMessageOutput` (new `g4_*` fixtures),
  `./dev/check.sh`

Known limitations:
- Dwell runtime conversion (e.g. spindle-revolution time conversion using
  spindle RPM/override state) is intentionally out of scope for parser/lowering
  v0 and not computed here.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R "(G4LowererTest|MessagesTest.G4|MessagesJsonTest.RoundTripWithG4|SemanticRulesTest.ReportsG4|RegressionTest.Regression_G4|MessageDiffTest.G4|MessagesJsonTest.GoldenMessageOutput)"`
- `./dev/check.sh`

## 2026-02-24 (add PRD for product/API requirements)
- Added `PRD.md` to define product goals, non-goals, command-family policy,
  quality gates, and release acceptance criteria for the parser library.
- Added explicit product API requirements, including recommended class facade,
  file-based parse/lower APIs, and output contracts (`ParseResult`/
  `MessageResult`).
- Linked SPEC documentation policy to `PRD.md` as the product-level source,
  while keeping syntax/behavior details in `SPEC.md`.

SPEC sections / tests:
- SPEC: Section 9 documentation policy note
- Tests: not applicable (docs-only)

Known limitations:
- PRD defines target API direction; class-facade/file-API implementation still
  requires a dedicated backlog task and code changes.

How to reproduce locally (commands):
- `sed -n '1,260p' PRD.md`
- `sed -n '200,360p' SPEC.md`
- `sed -n '1,260p' src/gcode_parser.h`
- `sed -n '1,260p' src/messages.h`

## 2026-02-24 (plan streaming API + benchmark baseline requirements)
- Added backlog tasks `T-019` (streaming parse/lower API) and `T-020`
  (benchmark harness + 10k-line baseline) with acceptance criteria.
- Updated `PRD.md` API/quality requirements to include streaming output support,
  cancellation/limits, and benchmark throughput visibility.
- Updated `SPEC.md` to document planned streaming output mode and benchmark
  expectations in testing requirements.

SPEC sections / tests:
- SPEC: Section 2.2 (planned streaming output mode), Section 7 (benchmark expectations)
- Tests: planning-only (implementation tracked by T-019/T-020)

Known limitations:
- No streaming runtime API or benchmark executable is implemented in this
  change; this is planning/spec alignment only.

How to reproduce locally (commands):
- `sed -n '1,260p' BACKLOG.md`
- `sed -n '1,260p' PRD.md`
- `sed -n '1,360p' SPEC.md`

## 2026-02-24 (T-019 streaming parse/lower API)
- Added callback-based streaming APIs: `lowerToMessagesStream`,
  `parseAndLowerStream`, and `parseAndLowerFileStream` in `src/messages.h/.cpp`.
- Added stream controls (`StreamOptions`) for early stop via max lines/messages/
  diagnostics and cancel hook.
- Added streaming tests (`test/streaming_tests.cpp`) covering 10k-line streaming,
  cancel behavior, rejected-line signaling, and file-based streaming input.

SPEC sections / tests:
- SPEC: Section 2.2 (streaming output mode), Section 6 (streaming API)
- Tests: `StreamingTest.*`, `./dev/check.sh`

Known limitations:
- Current streaming API still parses full input into AST before lowering stream;
  true incremental parsing is not part of this slice.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R StreamingTest`
- `./dev/check.sh`

## 2026-02-24 (T-020 benchmark harness + 10k-line baseline)
- Added benchmark executable `gcode_bench` (`bench/gcode_bench.cpp`) to measure
  parse-only and parse+lower performance on deterministic input.
- Added `dev/bench.sh` and benchmark output artifact path
  `output/bench/latest.json` with machine-readable metrics (time, lines/sec,
  bytes/sec).
- Added CI benchmark smoke job (`benchmark-smoke`) as soft-gating
  (`continue-on-error: true`) and integrated local benchmark smoke via ctest
  (`BenchmarkSmoke`).

SPEC sections / tests:
- SPEC: Section 7 (Performance benchmarking expectations)
- Tests: `BenchmarkSmoke`, `./dev/check.sh`, `./dev/bench.sh`

Known limitations:
- Benchmark currently uses a deterministic synthetic G1 corpus; broader mixed
  real-world corpora can be added in follow-up work.
- Metrics are wall-clock throughput; hardware-specific CPU cycles are not used
  as CI gating criteria.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R BenchmarkSmoke`
- `./dev/bench.sh`

## 2026-02-27 (T-021 detailed streaming callback message assertions)
- Added streaming unit test that captures callback-emitted messages into a
  vector and validates detailed field content for `G4` and `G1` payloads.
- Updated SPEC testing expectations to require field-level validation for
  streaming callback tests (not just count-based assertions).
- Updated backlog bookkeeping: `T-020` marked done and `T-021` tracked in progress.

SPEC sections / tests:
- SPEC: Section 7 (Testing Expectations)
- Tests: `StreamingTest.CallbackCanCaptureAndValidateDetailedMessages`,
  `./dev/check.sh`

Known limitations:
- This change strengthens tests only; no streaming API behavior changes.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R StreamingTest`
- `./dev/check.sh`
