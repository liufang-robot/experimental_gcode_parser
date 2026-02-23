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

### T-006 (P1) Add polymorphic queue message model (start with G1)
Why:
- Product output should be typed messages, not only generic AST.
Acceptance criteria:
- Add message model with polymorphism for queue payloads.
- Add `G1Message` containing source info, line number, pose (`xyzabc`), and feed.
- Source info supports optional filename and line.
- No behavior regression in existing parse/golden tests.

### T-007 (P1) Add AST-to-message lowering pipeline (G1)
Why:
- Need standalone conversion from parsed AST into queue-ready messages.
Acceptance criteria:
- Add lowering API that maps AST lines into message list and diagnostics.
- Error lines (for example `G1 G2 X10`) do not emit motion message.
- Valid later lines still emit messages (resume-style behavior).
- Add tests for basic G1 extraction and skip-error-line behavior.

### T-008 (P1) Incremental resume-from-line session API
Why:
- Product needs practical re-entry after user edits near error lines.
Acceptance criteria:
- Add session API to reparse from changed line and rebuild affected messages.
- Preserve stable source line mapping and deterministic message ordering.
- Add tests for edit-and-resume flow.

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

### T-011 (P1) Message JSON serialization/deserialization
Why:
- Need stable machine-readable message payloads for queue fixtures, transport, and tooling.
Acceptance criteria:
- Add `toJson`/`fromJson` support for `MessageResult`/`G1Message`.
- Add schema versioning in serialized output.
- Add round-trip tests and data-asset golden tests for message JSON output.

### T-012 (P1) Implement G2/G3 message lowering (arc moves)
Why:
- Motion queue needs typed arc commands, not only linear `G1`.
Acceptance criteria:
- Add `G2Message` and `G3Message` typed payloads with source info and optional feed.
- Lower AST lines containing `G2`/`G3` into arc messages with target pose (`xyzabc`) and arc parameters (`I`,`J`,`K`,`R`) per current SPEC scope.
- Keep motion-family exclusivity: multiple motion commands on one line stay diagnostics errors and emit no message for that line.
- Preserve fail-fast behavior (`StopAtFirstError`) and rejected-line reporting.
- Add golden message-output JSON assets that include valid `G2`/`G3` and one invalid mixed-motion line.
- No regressions in existing parser/message tests.

### T-013 (P1) Expand message golden test matrix
Why:
- Current `testdata/messages` has only a fail-fast fixture; coverage should include representative valid and mixed inputs.
Acceptance criteria:
- Add message goldens for: valid multi-line `G1`, lowercase/uppercase equivalence, optional filename/line-number presence, comments/blank lines, and mixed valid+invalid lines.
- Keep one golden explicitly validating fail-fast stop behavior and `rejected_lines`.
- Update `test/messages_json_tests.cpp` to run all fixtures from `testdata/messages/` as a table-driven golden suite.
- Document fixture naming conventions in test code comments or SPEC testing section.
- No regressions in parser/message unit tests and existing goldens.

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
- T-013 (feature/t013-message-golden-matrix)

## Done
(Move completed tasks here with PR link)
- T-012 (PR #13)
- T-005 (PR #12)
- T-004 (PR #11)
- T-003 (PR #10)
- T-002 (PR #9)
- T-001 (PR #8)
- T-011 (PR #6)
