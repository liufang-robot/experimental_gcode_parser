# Program Reference: Motion and Modal Commands

## Motion Packets

Status:

- `Implemented` for the motion subset

Rules:

- packetization consumes AIL and emits packets for `G0/G1/G2/G3/G4`
- `packet_id` is 1-based and deterministic per result order
- non-motion AIL instructions are skipped with warning diagnostics where
  appropriate
- `G0` and `G1` both use packet type `linear_move`; distinguish intent through
  `packet.modal.code`

Output fields:

- `packet_id`
- `type`
- `source`
- `modal`
- `payload`

Packet types:

- `linear_move`
- `arc_move`
- `dwell`

## G1

Syntax examples:

- `G1 X10 Y20 Z30 F100`
- `G1 AP=90 RP=10 F40`

Rules:

- case-insensitive words are accepted
- mixing Cartesian and polar words in one `G1` line is rejected

Output fields:

- source: filename, line, line_number
- modal: `group=GGroup1`, `code=G1`, `updates_state=true`
- target_pose: optional `x/y/z/a/b/c`
- feed: optional `F`

Execution call sequence:

1. a complete `G1` line is assembled
2. the line is parsed and lowered into a normalized linear-move command
3. the execution sink receives the normalized command
4. the runtime receives the same command via linear-move submission
5. the runtime returns `ready`, `pending`, or `error`
6. `pending` blocks execution until explicit resume

## G0

Syntax examples:

- `G0 X10 Y20 Z30`
- `G0 AP=90 RP=10`

Rules:

- case-insensitive words are accepted
- mixing Cartesian and polar words in one `G0` line is rejected

Output fields:

- source: filename, line, line_number
- modal: `group=GGroup1`, `code=G0`, `updates_state=true`
- target_pose: optional `x/y/z/a/b/c`
- feed: optional `F`

Current limitations:

- Siemens rapid interpolation override behavior is not fully implemented in the
  runtime or packet path

## RTLION / RTLIOF

Syntax examples:

- `RTLION`
- `RTLIOF`

Current behavior:

- lowering emits AIL `rapid_mode`
- JSON includes:
  - `opcode`
  - `mode`
- following `G0` AIL instructions include `rapid_mode_effective` when a
  rapid-mode command is active earlier in program order

Current limitations:

- runtime execution tracks rapid-mode state transitions, but full machine
  interpolation override behavior is not implemented yet
- packetization does not emit standalone packets for `rapid_mode`

## G40 / G41 / G42

Syntax examples:

- `G40`
- `G41`
- `G42`

Current behavior:

- lowering emits AIL `tool_radius_comp`
- JSON includes:
  - `opcode`
  - `mode`: `off`, `left`, `right`
- `AilExecutor` tracks `tool_radius_comp_current`

Current limitations:

- cutter-path geometric compensation is not implemented in v0
- packetization does not emit standalone packets for `tool_radius_comp`

## G17 / G18 / G19

Syntax examples:

- `G17`
- `G18`
- `G19`

Current behavior:

- lowering emits AIL `working_plane`
- JSON includes:
  - `opcode`
  - `plane`: `xy`, `zx`, `yz`
- `AilExecutor` tracks `working_plane_current`
- `G2/G3` lowering validates center words against the effective plane

Current limitations:

- deeper plane-dependent geometric behavior is not implemented in v0
- packetization does not emit standalone packets for `working_plane`

## G2 / G3

Syntax examples:

- `G2 X10 Y20 I1 J2 K3 R40 F100`
- `G3 X30 Y40 I4 J5 K6 CR=50 F200`

Output fields:

- source: filename, line, line_number
- modal: `group=GGroup1`, `code=G2|G3`, `updates_state=true`
- target_pose: optional `x/y/z/a/b/c`
- arc: optional `i/j/k/r`
- feed: optional `F`
- effective plane metadata derived from the active working plane

## G4

Syntax examples:

- `G4 F3`
- `G4 S30`

Rules:

- must be programmed in its own NC block
- exactly one of `F` or `S` must be present

Output fields:

- source: filename, line, line_number
- modal: `group=GGroup2`, `code=G4`, `updates_state=false`
- dwell_mode: `seconds` or `revolutions`
- dwell_value: numeric value
