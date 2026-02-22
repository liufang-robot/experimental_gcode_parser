# BACKLOG

## How To Use
- Agent picks from top to bottom by priority.
- Each task must include acceptance criteria before work starts.
- Completed tasks are moved to a done section in this file or linked from PR.

## Priority Legend
- `P0`: must fix now (blocker, broken CI, crash/data-loss/security risk)
- `P1`: high-value near-term
- `P2`: important but can wait
- `P3`: optional/enhancement

## Ready Queue

### T-001 (P0) Add parser fuzz smoke test target
Why:
- SPEC requires crash/hang resilience.
Acceptance criteria:
- Add fuzz/property test executable under `test/`.
- Add deterministic smoke input corpus under `testdata/`.
- Integrate smoke run in `./dev/check.sh` and CI.
- Document known limits and runtime budget.

### T-002 (P1) Regression-test protocol for parser bugs
Why:
- Every bug should become a failing test first.
Acceptance criteria:
- Add regression test naming convention to test docs.
- Add at least one concrete regression example test.
- Update `OODA.md` Definition of Done to require bug-test linkage.

### T-003 (P1) Improve diagnostic messages
Why:
- Current diagnostics are structurally correct but terse.
Acceptance criteria:
- Diagnostics include actionable text for syntax and semantic errors.
- Add/refresh golden outputs where message text changed.
- Update SPEC diagnostics section if behavior changes.

### T-004 (P2) JSON output mode for CLI
Why:
- Easier machine integration than debug-text format.
Acceptance criteria:
- Add `--format json|debug` with stable JSON schema.
- Keep existing debug format for golden tests.
- Add tests for both output modes.

### T-005 (P2) Grammar cleanup for optional empty tail warning
Why:
- ANTLR warning indicates nullable optional rule in grammar.
Acceptance criteria:
- Remove warning during parser generation without behavior regression.
- Golden tests unchanged unless behavior intentionally changes.

## Icebox
- Performance benchmarking harness.
- Coverage threshold policy and badge.
- Multi-file include/subprogram parsing (future SPEC).

## Task Template
Use this template for new backlog items:

- `ID`: T-XXX
- `Priority`: P0/P1/P2/P3
- `Title`:
- `Why`:
- `Scope`:
- `Acceptance Criteria`:
- `Out of Scope`:
- `SPEC Sections`:
- `Tests To Add/Update`:
