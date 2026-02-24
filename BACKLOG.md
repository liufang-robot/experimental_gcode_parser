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

### T-014 (P2) Refactor parser/lowering into per-family OO modules
Why:
- SPEC now requires non-monolithic OO design per function family.
Acceptance criteria:
- Split family-specific parsing/lowering behavior into separate classes/files
  (at minimum: `G1`, `G2/G3`).
- Keep existing public APIs stable unless explicitly approved.
- Preserve all current tests/goldens and `./dev/check.sh` pass.
- Update architecture notes if module boundaries change.

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
- T-014 (feature/t014-oo-family-modules)

## Done
(Move completed tasks here with PR link)
- T-010 (PR #17)
- T-009 (PR #16)
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
