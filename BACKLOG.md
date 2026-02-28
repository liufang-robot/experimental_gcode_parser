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
### T-024 (P1) Add modal-group metadata to lowered messages
Why:
- Downstream queue consumers need explicit modal-group metadata per emitted
  message.
- We want Siemens-preferred modal semantics for current supported functions.
Scope:
- Add modal metadata fields to `G1/G2/G3/G4` message models.
- Populate lowering output:
  - `G1/G2/G3` -> group 1 (`GGroup1`), updates modal state.
  - `G4` -> group 2 (`GGroup2`), non-modal one-shot (does not update state).
- Include modal metadata in JSON serialization/deserialization.
- Add/adjust tests and JSON goldens for the new fields.
- Update SPEC + program reference docs.
Acceptance criteria:
- Every emitted message includes modal metadata in memory and JSON output.
- Tests validate modal metadata for message families and streaming callbacks.
- Message JSON golden fixtures include modal metadata.
- `./dev/check.sh` passes.
Out of scope:
- Full multi-group modal validation or machine-configuration-specific behavior.
SPEC Sections:
- Section 6 (Message Lowering), Section 9 (Documentation Policy).
Tests To Add/Update:
- `test/messages_tests.cpp`
- `test/streaming_tests.cpp`
- `test/lowering_family_g1_tests.cpp`
- `test/lowering_family_g2_tests.cpp`
- `test/lowering_family_g3_tests.cpp`
- `test/lowering_family_g4_tests.cpp`
- `test/messages_json_tests.cpp`
- `testdata/messages/*.golden.json`

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
- T-024 (feature/t024-modal-group-metadata)

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
