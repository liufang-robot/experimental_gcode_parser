# AGENTS.md — CNC G-code parser library

## Goal
Implement a C++ library that parses G-code text/file into an AST + diagnostics.

## Single Command Gate (authoritative)
- Run: `./dev/check.sh`
This script MUST run:
- configure/build
- unit tests + golden tests
- clang-format check
- clang-tidy (or a fast subset)
- sanitizer run (ASan/UBSan) if supported

If `./dev/check.sh` is green, CI should be green.

## Tooling Config
- clang-format config: `.clang-format`
- clang-tidy config: `.clang-tidy`
- compile database: `build/compile_commands.json` (CMake must export it)

## Code Style
- Use `clang-tidy` to check code style.
- Use `clang-format` to format code.

## Architecture
- public library headers in `include/gcode/`
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
- Before starting implementation, the agent must run the OODA flow:
  pick from `BACKLOG.md`, align priority with `ROADMAP.md`, and follow
  `OODA.md` (Observe/Orient/Decide/Act, merge policy, and definition of done).
- Prefer small PRs / small diffs.
- Public header rule:
  - only stable library entry points belong in `include/gcode/`
  - do not mirror `src/` into `include/gcode/`
  - if a header is internal/supporting-only, keep it in `src/` even if public
    headers include it transitively
  - if a type appears in a public signature or public struct field, its header
    is public too and must live in `include/gcode/`
  - when adding a new public header, update the public-header compile test
    under `test/public_headers_tests.cpp`
