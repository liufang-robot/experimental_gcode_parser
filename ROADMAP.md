# ROADMAP

## Purpose
Define where the parser project is heading in ordered milestones. This is the single source of truth for direction and sequencing.

## Current State
- v0 parser exists with AST + diagnostics.
- Golden tests and CI/dev checks are in place.
- ANTLR-based pipeline is operational.

## Milestones

### M1: Harden v0 (Current)
Goal: make v0 reliable and maintainable.
- Stabilize grammar behavior and diagnostics quality.
- Add targeted regression tests for parser edge cases.
- Add baseline fuzz/property testing and CI gate.
Exit criteria:
- `./dev/check.sh` passes.
- No open P0/P1 parser crash bugs.
- Fuzz smoke runs in CI without crashes.

### M2: v0.2 Syntax Coverage
Goal: expand syntax support while preserving AST/diagnostic quality.
- Add additional common word/address patterns per SPEC updates.
- Improve diagnostics (clearer messages, context hints).
- Keep compatibility with existing golden tests.
Exit criteria:
- SPEC updated with new syntax and examples.
- New golden tests for all added syntax.
- Regression tests for all bug fixes merged during milestone.

### M3: API and Tooling Usability
Goal: make parser easier to consume by downstream tools.
- Introduce stable library API boundaries (parse options/result schema).
- Add machine-friendly output mode (JSON) in CLI.
- Improve docs for embedding parser in other C++ projects.
Exit criteria:
- Documented API contract.
- Backward compatibility notes in SPEC/CHANGELOG.
- Example integration in README.

### M4: Quality and Scale
Goal: increase confidence under heavy/invalid input.
- Extend fuzz corpus and long-input stress tests.
- Add performance baseline and regression budget.
- Add coverage trend visibility in CI.
Exit criteria:
- Fuzz and stress suite runs regularly.
- Performance baseline captured and compared.
- Coverage threshold policy agreed and enforced.

## Prioritization Rules
- Reliability over feature breadth.
- Parser correctness over CLI convenience.
- SPEC alignment over ad-hoc behavior.

## Roadmap Update Policy
- Update ROADMAP when milestone scope or order changes.
- Every roadmap change must reference one or more BACKLOG items.
