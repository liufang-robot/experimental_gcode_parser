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
