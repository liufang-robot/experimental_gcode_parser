# SPEC: Testing, Architecture, and Documentation Policy

## 7. Testing Expectations

- golden tests cover representative behavior documented in the product docs
- CLI tests cover parse, ail, packet, and lower modes in debug and JSON forms
- unit test framework for new tests is GoogleTest
- regression rule:
  - every fixed bug gets a failing-first test
- parser fuzz/property smoke tests must not crash, hang, or grow without bound
- streaming execution tests validate:
  - chunk assembly
  - sink/runtime call order
  - normalized command arguments
  - blocked/resume/cancel state transitions
  - deterministic event logs
- maintain benchmark visibility for large deterministic corpora

## 8. Code Architecture Requirements

- use modular object-oriented design
- major function families live in separate classes/files
- do not accumulate all family behavior in one monolithic source file
- new families should be added as modules, not long branching chains

## 9. Documentation Policy

Canonical docs live under `docs/src/`.

Key documentation surfaces:

- `docs/src/product/program_reference/index.md`
- `docs/src/product/prd/index.md`
- `docs/src/product/spec/index.md`
- `docs/src/development/`

Documentation rules:

- every feature PR updates impacted docs in the same PR
- the program reference describes implemented behavior, not only plans
- every documented command/function includes status:
  - `Implemented`
  - `Partial`
  - `Planned`
- if a page gets too long, split it and link the subtree through
  `docs/src/SUMMARY.md`
- planned behavior must be marked as planned until code and tests land
- CI must build the mdBook and publish it from `main`

PR gate expectation:

- list updated product sections
- list updated development/reference sections
- list tests proving the documented behavior
