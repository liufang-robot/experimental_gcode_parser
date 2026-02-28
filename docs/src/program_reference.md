# Program Reference

This section documents currently implemented parser/lowering behavior.

## Output APIs

- Intermediate IR:
  - `parseAndLowerAil(...)` -> `AilResult`
- Packet stage:
  - `parseLowerAndPacketize(...)` -> `PacketResult`
- Batch: `parseAndLower(...)` -> `MessageResult`
- Streaming callbacks:
  - `parseAndLowerStream(...)`
  - `parseAndLowerFileStream(...)`

## Modal Metadata

Every emitted message includes `modal` metadata:

- `group`: `GGroup1` (modal motion) or `GGroup2` (non-modal dwell)
- `code`: emitted function code (`G1`, `G2`, `G3`, `G4`)
- `updates_state`: whether this message updates modal state

Current Siemens-aligned baseline for supported functions:

- `G1/G2/G3` -> `GGroup1`, `updates_state=true`
- `G4` -> `GGroup2`, `updates_state=false`

## Command Status Matrix

| Command | Status | Notes |
|---|---|---|
| `G1` linear | Implemented | Emits `G1Message` with target pose + feed. |
| `G2` arc CW | Implemented | Emits `G2Message` with endpoint + arc fields + feed. |
| `G3` arc CCW | Implemented | Emits `G3Message` with endpoint + arc fields + feed. |
| `G4` dwell | Implemented | Emits `G4Message` with dwell mode/value. |
| `R... = expr` assignment | Partial | Parsed into AIL assignment instruction; not yet evaluated/runtime-applied. |

## Motion Packets

Status:
- `Implemented` (motion subset)

Rules:
- Packetization consumes AIL and emits packets for `G1/G2/G3/G4`.
- `packet_id` is 1-based and deterministic per result order.
- Non-motion AIL instructions (for example `assign`) are skipped with a
  warning diagnostic.

Output fields:
- packet: `packet_id`, `type`, `source`, `modal`, `payload`
- packet types: `linear_move`, `arc_move`, `dwell`

Test references:
- `test/packet_tests.cpp`
- `test/cli_tests.cpp`
- `testdata/packets/*.ngc`

## G1

Syntax examples:

- `G1 X10 Y20 Z30 F100`
- `G1 AP=90 RP=10 F40`

Rules:

- Case-insensitive words (`g1`, `x`, `f`) are accepted.
- Mixing Cartesian and polar words in one `G1` line is rejected.

Output fields:

- source: filename/line/line_number
- modal: `group=GGroup1`, `code=G1`, `updates_state=true`
- target_pose: optional `x/y/z/a/b/c`
- feed: optional `F`

## G2 / G3

Syntax examples:

- `G2 X10 Y20 I1 J2 K3 R40 F100`
- `G3 X30 Y40 I4 J5 K6 CR=50 F200`

Output fields:

- source: filename/line/line_number
- modal: `group=GGroup1`, `code=G2|G3`, `updates_state=true`
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
- modal: `group=GGroup2`, `code=G4`, `updates_state=false`
- dwell_mode: `Seconds` (F) or `SpindleRevolutions` (S)
- dwell_value: numeric value

## Test References

- `test/messages_tests.cpp`
- `test/streaming_tests.cpp`
- `test/messages_json_tests.cpp`
- `test/regression_tests.cpp`
- `test/semantic_rules_tests.cpp`
- `test/lowering_family_g4_tests.cpp`
