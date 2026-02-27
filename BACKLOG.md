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
### T-019 (P1) Add streaming parse/lower API for large input safety
Why:
- Current APIs return full in-memory results, which is less suitable for very
  large files and long-running parse sessions.
- Product requirement needs controlled processing with cancellation/limits.
Scope:
- Add additive streaming API for parse/lower output delivery.
- Support event/callback-style delivery for diagnostics and messages.
- Add safety controls (for example: max lines/diagnostics and cancel hook).
Acceptance criteria:
- Existing non-streaming APIs remain backward compatible.
- Streaming API can process at least a 10k-line file without accumulating all
  output in memory by default.
- Streaming API exposes diagnostics and message outputs with source location.
- Unit/integration tests cover normal flow and early-stop/cancel behavior.
- `./dev/check.sh` passes.
Out of scope:
- Full incremental parsing engine rewrite.
- ABI freeze beyond current project policy.
SPEC Sections:
- Section 2 (Input/Output), Section 6 (Message Lowering), Section 7 (Testing).
Tests To Add/Update:
- `test/` streaming API tests
- large-input fixture in `testdata/`

### T-020 (P1) Add benchmark harness and 10k-line performance baseline
Why:
- Need measurable parsing efficiency data and regression visibility.
Scope:
- Add benchmark executable/scripts for parser and lowering throughput.
- Measure parse-only and parse+lower on deterministic corpora.
Acceptance criteria:
- Benchmark command is documented and runnable locally.
- Includes baseline scenario for 10k lines and reports at least:
  total time, lines/sec, bytes/sec.
- Produces stable machine-readable output artifact under `output/bench/`.
- CI can run a lightweight benchmark smoke (non-gating or soft-gating).
- `./dev/check.sh` passes.
Out of scope:
- Hardware-specific CPU cycle accounting as a hard CI gate.
SPEC Sections:
- Section 7 (Testing), quality/performance notes.
Tests To Add/Update:
- `bench/` harness and smoke check entry
- optional CI benchmark job/update

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
- T-018 (feature/t018-g4-dwell-support, pending PR)
