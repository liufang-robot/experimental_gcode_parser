# AGENTS.md — CNC G-code parser library

## Goal
Implement a C++ library that parses G-code text/file into an AST + diagnostics.

## Build Commands
- Configure/build: `cmake -S . -B build`
- Build: `cmake --build build -j`

## Test Commands
- Test: `ctest --test-dir build --output-on-failure`

## Code Style
- Use `clang-tidy` to check code style.
- Use `clang-format` to format code.

## Architecture
- implementation code in `src/`
- test code in `test/`
- test data sets in `testdata/`

## Rules
- C++ standard: C++17
- No new dependencies without asking.
- Every feature requires:
  1) tests in `test/`
  2) updates to `SPEC.md` if behavior changes
  3) a short entry in `CHANGELOG.md` (optional)
- Every change must include a `CHANGELOG_AGENT.md` entry with:
  - What changed (1–3 bullets)
  - Which SPEC sections / tests cover it
  - Any known limitations
  - How to reproduce locally (commands)
- Every feature PR must include evidence of passing `./dev/check.sh` and list
  new tests plus SPEC sections covered.
- Prefer small PRs / small diffs.
