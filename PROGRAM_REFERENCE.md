# Program Reference Manual (Implementation Snapshot)

Note:
- Canonical maintained program reference is now `docs/src/program_reference.md`
  (mdBook source). This file is retained as a compatibility snapshot.

This document is the implementation-facing reference for parser/lowering
behavior currently available in code. It must be updated in the same PR as any
behavior change.

Version scope:
- Parser library: current `main`
- Status legend: `Implemented`, `Partial`, `Planned`

## Output Access

Implemented API paths:
- batch output: `parseAndLower(...)` returns `MessageResult`
- streaming output: `parseAndLowerStream(...)` /
  `parseAndLowerFileStream(...)` emit callbacks for messages, diagnostics, and
  rejected lines

## Modal Metadata

All emitted messages include:
- `modal.group`: `GGroup1` for `G1/G2/G3`, `GGroup2` for `G4`
- `modal.code`: `G1`/`G2`/`G3`/`G4`
- `modal.updates_state`: `true` for `G1/G2/G3`, `false` for `G4`

## Command Matrix

| Command | Status | Notes |
|---|---|---|
| `G1` linear | Implemented | Lowers to `G1Message` with pose/feed. |
| `G2` arc CW | Implemented | Lowers to `G2Message` with pose/arc/feed. |
| `G3` arc CCW | Implemented | Lowers to `G3Message` with pose/arc/feed. |
| `G4` dwell | Implemented | Lowers to `G4Message` with mode/value. |

## `G1` Linear Interpolation

Status:
- `Implemented`

Syntax:
- `G1 X... Y... Z... [A...] [B...] [C...] [F...]`
- `G1 AP=... RP=... [F...]`
- Case-insensitive words (`g1`, `x`, etc. accepted).

Lowering output:
- `G1Message`
  - `source`: filename/line/line_number
  - `modal`: `group=GGroup1`, `code=G1`, `updates_state=true`
  - `target_pose`: optional `x/y/z/a/b/c`
  - `feed`: optional `F`

Diagnostics:
- Error when mixing Cartesian (`X/Y/Z/A`) and polar (`AP/RP`) in same `G1`
  line.

Examples:
- `G1 X10 Y20 Z30 F100`
- `G1 AP=90 RP=10 F40`

Test references:
- `test/messages_tests.cpp`
- `test/parser_tests.cpp`
- `test/regression_tests.cpp`
- `testdata/g1_samples.ngc`

## `G2` / `G3` Circular Interpolation

Status:
- `Implemented`

Syntax:
- `G2`/`G3` with endpoint words (`X/Y/Z`)
- Arc words: `I/J/K`, `R`/`CR`, plus optional feed `F`
- Parser accepts additional arc-related words; unsupported lowering words are
  currently warning-only.

Lowering output:
- `G2Message` / `G3Message`
  - `source`: filename/line/line_number
  - `modal`: `group=GGroup1`, `code=G2|G3`, `updates_state=true`
  - `target_pose`: optional `x/y/z/a/b/c`
  - `arc`: optional `i/j/k/r`
  - `feed`: optional `F`

Diagnostics:
- Warning for unsupported arc words in lowering, while still emitting supported
  fields.

Examples:
- `G2 X10 Y20 I1 J2 K3 CR=40 F100`
- `G3 X30 Y40 I4 J5 K6 R50 F200`

Test references:
- `test/messages_tests.cpp`
- `test/messages_json_tests.cpp`
- `testdata/g2g3_samples.ngc`

## `G4` Dwell

Status:
- `Implemented`

Syntax:
- `G4 F...` (seconds)
- `G4 S...` (master spindle revolutions)
- Program in separate NC block.

Lowering output:
- `G4Message` with `dwell_mode` and `dwell_value`.
- `G4Message.modal`: `group=GGroup2`, `code=G4`, `updates_state=false`.

Diagnostics:
- Error if `G4` is not in a separate block.
- Error if both `F` and `S` are present.
- Error if neither `F` nor `S` is present.

Examples:
- `G4 F3`
- `G4 S30`

Test references:
- `test/messages_tests.cpp`
- `test/messages_json_tests.cpp`
- `test/semantic_rules_tests.cpp`
- `test/regression_tests.cpp`
- `test/lowering_family_g4_tests.cpp`
- `testdata/messages/g4_dwell.ngc`
- `testdata/messages/g4_failfast.ngc`
