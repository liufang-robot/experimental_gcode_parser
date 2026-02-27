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
### T-023 (P1) Rewrite README for accurate and comprehensive project guide
Why:
- Current `README.md` is outdated and does not fully reflect implemented
  parser/lowering APIs, docs flow, and CI/dev workflow.
Scope:
- Rewrite `README.md` to cover:
  - project purpose and current feature status (`G1/G2/G3/G4`, diagnostics,
    AST/lowering, streaming)
  - build/runtime prerequisites (ANTLR runtime/tool, CMake, gtest, json lib)
  - local build/test/benchmark/docs commands
  - library usage entry points (batch + streaming)
  - links to `SPEC.md`, `PRD.md`, and mdBook docs
- Keep examples aligned with currently implemented behavior.
Acceptance criteria:
- `README.md` sections are consistent with implementation and `SPEC.md`.
- Includes at least one parse/lower usage snippet and one streaming snippet.
- Includes mdBook docs build/serve instructions.
- `./dev/check.sh` passes after README update.
Out of scope:
- API behavior changes or new parser features.
SPEC Sections:
- Section 9 (Documentation Policy), Section 6 (Message Lowering summary references).
Tests To Add/Update:
- No new unit tests (docs task); verify by running `./dev/check.sh`.

## Icebox
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
- T-023 (feature/t023-readme-rewrite)

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
- T-014 (PR #20)
- T-015 (PR #21)
- T-016 (PR #22)
- T-017 (PR #23)
- T-018 (PR #25)
- T-019 (PR #26)
- T-020 (PR #27)
- T-021 (PR #28)
- T-022 (PR #31)
