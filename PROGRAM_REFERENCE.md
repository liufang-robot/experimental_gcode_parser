# Program Reference Manual (Implementation Snapshot)

This document is the implementation-facing reference for parser/lowering
behavior currently available in code. It must be updated in the same PR as any
behavior change.

Version scope:
- Parser library: current `main`
- Status legend: `Implemented`, `Partial`, `Planned`

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
