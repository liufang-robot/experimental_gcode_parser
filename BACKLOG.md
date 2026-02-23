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

### T-009 (P2) Queue diff/apply API
Why:
- Downstream consumers need incremental updates, not full queue rebuild each edit.
Acceptance criteria:
- Add diff result with added/updated/removed messages keyed by source line.
- Add tests for line edit insert/delete/update scenarios.

### T-010 (P2) Message diagnostics policy and severity mapping
Why:
- Need explicit policy for parser/lowering errors at message stage.
Acceptance criteria:
- Define and implement policy (skip/partial message emission) in SPEC + code.
- Add tests that validate diagnostic severity and emission policy.

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

## In Progress
(List tasks currently being worked on; only one assignee/task per PR)
- T-009 (feature/t009-queue-diff-apply)

## Done
(Move completed tasks here with PR link)
- T-008 (PR #15)
- T-013 (PR #14)
- T-012 (PR #13)
- T-007 (PR #3)
- T-006 (PR #3)
- T-005 (PR #12)
- T-004 (PR #11)
- T-003 (PR #10)
- T-002 (PR #9)
- T-001 (PR #8)
- T-011 (PR #6)
