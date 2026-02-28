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
### T-026 (P1) Introduce AIL intermediate representation layer
Why:
- Current pipeline is parse -> message lowering; we need a stable semantic IR
  between syntax and runtime packets.
Scope:
- Add `AIL` instruction model (motion + assignment + sync placeholder types).
- Add source mapping, diagnostics passthrough, and JSON serialization.
- Keep existing message lowering APIs working during transition.
Acceptance criteria:
- New IR types compile and can be produced from current motion AST subset.
- IR JSON schema is documented and unit-tested.
- No regression in existing parse/lower tests.
- `./dev/check.sh` passes.
Out of scope:
- Full expression evaluation runtime.
SPEC Sections:
- Section 2.2 (pipeline modes), Section 6 (lowering architecture).
Tests To Add/Update:
- New `test/ail_tests.cpp`
- CLI tests for `--mode ail --format json` (once mode is wired)

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

### T-028 (P1) Add AIL -> MotionPacket conversion
Why:
- Runtime and replay need packetized execution payloads.
Scope:
- Define `MotionPacket` envelope (`packet_id`, type, payload, source).
- Convert AIL motion instructions into packets.
- Add packet JSON serializer.
Acceptance criteria:
- Packet output is deterministic and stable for same input.
- Packet JSON goldens exist for core samples.
- `./dev/check.sh` passes.
Out of scope:
- Path planner execution.
SPEC Sections:
- Section 2.2 (output modes), Section 6 (packet stage).
Tests To Add/Update:
- `test/packet_tests.cpp`
- `testdata/packets/*.golden.json`

### T-029 (P1) Add expression/assignment AST + AIL instructions
Why:
- Need non-motion semantic instructions, e.g. `R1 = $P_ACT_X`.
Scope:
- Extend grammar/AST for assignment expressions.
- Lower to AIL assignment instruction nodes.
- Support system-variable references as symbolic operands.
Acceptance criteria:
- Parser accepts representative assignment syntax and reports diagnostics with
  line/column on invalid expressions.
- AIL output includes assignment instructions with source mapping.
- `./dev/check.sh` passes.
Out of scope:
- Runtime evaluation against live machine values.
SPEC Sections:
- Section 3 (syntax), Section 5 (diagnostics), Section 6 (AIL).
Tests To Add/Update:
- `test/parser_tests.cpp` (assignment coverage)
- `test/ail_lowering_tests.cpp` (assignment lowering)

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
- T-026 (feature/t026-ail-pipeline-planning)

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
