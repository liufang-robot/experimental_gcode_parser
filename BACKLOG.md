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
### T-025 (P1) Add CLI lower mode for emitted messages
Why:
- Current CLI only exposes parse output; users cannot directly inspect lowered
  queue messages from a file.
Scope:
- Extend `gcode_parse` with `--mode parse|lower` (default `parse`).
- Implement `--mode lower --format json` output via `parseAndLower(...)`.
- Implement `--mode lower --format debug` as human-readable lowered summary:
  emitted message lines, rejections, diagnostics, and totals.
- Keep existing `--mode parse` behavior and output compatibility.
- Add CLI tests for lower mode outputs and argument validation.
Acceptance criteria:
- `gcode_parse --mode lower --format json <file>` outputs `MessageResult`
  JSON (`schema_version`, `messages`, `diagnostics`, `rejected_lines`).
- `gcode_parse --mode lower --format debug <file>` prints stable summary lines
  that include message type, source, modal metadata, and key payload fields.
- Existing parse mode tests remain green.
- `./dev/check.sh` passes.
Out of scope:
- Streaming CLI mode and full modal-state simulation engine.
SPEC Sections:
- Section 2.2 (CLI output modes), Section 6 (Message Lowering), Section 7 (testing).
Tests To Add/Update:
- `test/cli_tests.cpp` (new lower-mode coverage)
- Optional golden assets for lower debug output (if formatter is fixed text)

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
- T-025 (feature/t025-cli-lower-mode)

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
