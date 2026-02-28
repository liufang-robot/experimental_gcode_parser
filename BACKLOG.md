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
### T-027 (P1) Add AST -> AIL lowering for G1/G2/G3/G4
Why:
- Need deterministic semantic instructions before packetization.
Scope:
- Map supported motion commands to AIL instruction variants.
- Preserve source line/N-line mapping and modal metadata.
Acceptance criteria:
- Existing supported motion commands lower to AIL with expected fields.
- Golden fixtures for representative AIL output.
- `./dev/check.sh` passes.
Out of scope:
- Packet transport/execution.
SPEC Sections:
- Section 3 (syntax), Section 6 (lowering), Section 7 (goldens).
Tests To Add/Update:
- `test/ail_lowering_tests.cpp`
- `testdata/ail/*.golden.json`

### T-030 (P2) Add stage-by-stage CLI modes and artifacts
Why:
- Users need visibility into each pipeline stage.
Scope:
- Extend CLI to support `--mode parse|ail|packet|lower`.
- Add stable debug/json output per stage.
- Add stage-specific golden tests.
Acceptance criteria:
- Each mode produces output for same file with deterministic schema/text.
- Existing parse/lower compatibility maintained.
- `./dev/check.sh` passes.
Out of scope:
- Streaming packet emit mode.
SPEC Sections:
- Section 2.2 (CLI), Section 7 (testing).
Tests To Add/Update:
- `test/cli_tests.cpp`
- `testdata/cli/*.golden.txt` and/or json fixtures

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
- T-030 (feature/t030-cli-stage-goldens)

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
- T-023 (PR #33)
- T-024 (PR #35)
- T-025 (PR #36)
- T-026 (PR #37)
- T-029 (PR #38)
- T-028 (PR #39)
