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
### T-018 (P1) Add G4 dwell support (`F` seconds / `S` spindle revolutions)
Why:
- Dwell is a core NC operation and currently unsupported in lowering output.
- Product requirements need explicit queue messages for dwell with stable
  source metadata.
Scope:
- Add syntax + lowering support for `G4` in its own NC block.
- Support dwell time words:
  - `G4 F...` where `F` is dwell time in seconds
  - `G4 S...` where `S` is dwell time in master-spindle revolutions
- Add typed queue message for dwell with source info and dwell mode/value.
Acceptance criteria:
- `G4 F...` lowers to one dwell message with mode=`seconds`.
- `G4 S...` lowers to one dwell message with mode=`revolutions`.
- `G4` must be programmed in a separate NC block; violations produce explicit
  error diagnostics with line/column.
- Existing feed (`F`) and spindle speed (`S`) semantics for adjacent non-`G4`
  blocks remain unchanged (no modal overwrite by dwell message).
- JSON conversion (`toJson`/`fromJson`) supports dwell messages.
- Golden tests and unit tests added for valid/invalid dwell examples.
- `./dev/check.sh` passes.
Out of scope:
- Time-to-real-seconds conversion from spindle RPM/override runtime state.
- Controller execution semantics beyond parse/lower output contracts.
SPEC Sections:
- Section 3 (Supported Syntax), Section 5 (Diagnostics), Section 6 (Message
  Lowering), Section 7 (Testing Strategy), Section 8 (architecture split).
Tests To Add/Update:
- `test/parser_tests.cpp`
- `test/messages_tests.cpp`
- `test/messages_json_tests.cpp`
- `test/regression_tests.cpp`
- `testdata/messages/*g4*`

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
- None.

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
