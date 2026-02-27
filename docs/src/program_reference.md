# Program Reference

This section documents currently implemented parser/lowering behavior.

## Output APIs

- Batch: `parseAndLower(...)` -> `MessageResult`
- Streaming callbacks:
  - `parseAndLowerStream(...)`
  - `parseAndLowerFileStream(...)`

## Command Status Matrix

| Command | Status | Notes |
|---|---|---|
| `G1` linear | Implemented | Emits `G1Message` with target pose + feed. |
| `G2` arc CW | Implemented | Emits `G2Message` with endpoint + arc fields + feed. |
| `G3` arc CCW | Implemented | Emits `G3Message` with endpoint + arc fields + feed. |
| `G4` dwell | Implemented | Emits `G4Message` with dwell mode/value. |

## G1

Syntax examples:

- `G1 X10 Y20 Z30 F100`
- `G1 AP=90 RP=10 F40`

Rules:

- Case-insensitive words (`g1`, `x`, `f`) are accepted.
- Mixing Cartesian and polar words in one `G1` line is rejected.

Output fields:

- source: filename/line/line_number
- target_pose: optional `x/y/z/a/b/c`
- feed: optional `F`

## G2 / G3

Syntax examples:

- `G2 X10 Y20 I1 J2 K3 R40 F100`
- `G3 X30 Y40 I4 J5 K6 CR=50 F200`

Output fields:

- source: filename/line/line_number
- target_pose: optional `x/y/z/a/b/c`
- arc: optional `i/j/k/r`
- feed: optional `F`

## G4

Syntax examples:

- `G4 F3`
- `G4 S30`

Rules:

- Must be programmed in its own NC block.
- Exactly one of `F` or `S` must be present.

Output fields:

- source: filename/line/line_number
- dwell_mode: `Seconds` (F) or `SpindleRevolutions` (S)
- dwell_value: numeric value

## Test References

- `test/messages_tests.cpp`
- `test/streaming_tests.cpp`
- `test/messages_json_tests.cpp`
- `test/regression_tests.cpp`
- `test/semantic_rules_tests.cpp`
- `test/lowering_family_g4_tests.cpp`
