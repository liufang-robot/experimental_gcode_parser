# Program Reference

This section documents currently implemented parser/lowering behavior and the
planned streaming-first execution boundary.

## Output APIs

- Primary public execution APIs:
  - `StreamingExecutionEngine`
  - `ExecutionSession`
  - injected interfaces:
    - execution sink
    - runtime
    - cancellation
- Additive executable IR API:
  - `AilExecutor::step(now_ms, sink, runtime)`
  - uses the same `IExecutionSink` / `IExecutionRuntime` motion contract for
    motion-capable AIL instructions
  - `AilExecutorOptions.initial_state` can seed modal context when an executor
    instance needs to inherit prior plane/rapid/tool-comp state
  - `StreamingExecutionEngine` now seeds a per-line executor from its carried
    modal state for the supported control/runtime subset
- Public parser/lowering APIs:
  - `parse(...)` -> `ParseResult`
  - `parseAndLowerAil(...)` -> `AilResult`

Current limitations:
- `StreamingExecutionEngine` currently covers motion/dwell lines plus
  line-level diagnostics/rejections; variable/control-flow execution remains
  follow-up work.
- `AilExecutor::step(now_ms, sink, runtime)` currently dispatches motion/dwell
  instructions through the shared runtime path.

## Modal Metadata

Every emitted message includes `modal` metadata:

- `group`: `GGroup1` (modal motion) or `GGroup2` (non-modal dwell)
- `code`: emitted function code (`G0`, `G1`, `G2`, `G3`, `G4`)
- `updates_state`: whether this message updates modal state

Current Siemens-aligned baseline for supported functions:

- `G0/G1/G2/G3` -> `GGroup1`, `updates_state=true`
- `G4` -> `GGroup2`, `updates_state=false`

## Command Status Matrix

| Command | Status | Notes |
|---|---|---|
| `G0` rapid baseline | Implemented | Emits `G0Message` with target pose + feed. |
| `RTLION` / `RTLIOF` rapid interpolation mode | Partial | Lowered to AIL `rapid_mode`; packet/runtime interpolation semantics pending. |
| `G40` / `G41` / `G42` tool radius compensation | Partial | Lowered to AIL `tool_radius_comp`; executor tracks modal state only. |
| `G17` / `G18` / `G19` working plane | Partial | Lowered to AIL `working_plane`; executor tracks active plane state only. |
| `G1` linear | Implemented | Compatibility surfaces emit `G1Message`; streaming engine and executor runtime-step overload both emit normalized linear-move commands to sink/runtime interfaces. |
| `G2` arc CW | Implemented | Emits `G2Message`; streaming engine and executor runtime-step overload can dispatch normalized arc commands. |
| `G3` arc CCW | Implemented | Emits `G3Message`; streaming engine and executor runtime-step overload can dispatch normalized arc commands. |
| `G4` dwell | Implemented | Emits `G4Message`; streaming engine and executor runtime-step overload can dispatch normalized dwell commands. |
| `M` functions (`M<value>`, `M<ext>=<value>`) | Partial | Parse + validation only in current slice; runtime machine actions not yet mapped. |
| `R... = expr` assignment | Partial | Parsed and lowered to AIL `assign`; runtime evaluation/store policy not implemented. |
| `N...` line number at block start | Implemented | Parsed into source metadata; duplicate-warning support for line-number jumps. |
| Block delete `/` and `/0.. /9` | Implemented | Parsed with skip level metadata; skip policy applied by `LowerOptions.active_skip_levels`. |
| `GOTO/GOTOF/GOTOB/GOTOC` + labels | Implemented | Parsed, lowered to AIL, and executable in `AilExecutor`. |
| `IF cond GOTO ... [ELSE GOTO ...]` | Implemented | Parsed/lowered to AIL `branch_if`; runtime resolver decides branch. |
| Structured `IF/ELSE/ENDIF` | Implemented | Parsed and lowered into label/goto control-flow pattern; runtime executes one branch. |
| `WHILE/ENDWHILE`, `FOR/ENDFOR`, `REPEAT/UNTIL`, `LOOP/ENDLOOP` | Partial | Parse-only in current implementation (no lowering/executor semantics yet). |
| Comments `;...`, `( ... )`, `(* ... *)` | Implemented | Preserved as comment items in parse output. |
| Comments `// ...` | Partial | Supported only when `ParseOptions.enable_double_slash_comments=true`; error by default. |
| Subprogram call by name (`THE_SHAPE`, `"THE_SHAPE"`) | Planned | Siemens-style call model planned; parser/runtime integration pending. |
| Subprogram repeat (`P=<n> NAME`, `NAME P<n>`) | Planned | Repeat-count call semantics planned. |
| Subprogram return (`M17` baseline, `RET` optional) | Planned | Return-to-caller runtime semantics planned. |
| ISO compat call (`M98 P...`) | Planned | Enabled only by ISO-compatibility profile option. |

## Parse/Lower Options

- Parse options:
  - `ParseOptions.enable_double_slash_comments`
    - `false` (default): `// ...` emits diagnostic
    - `true`: `// ...` is accepted as comment
- Lower options:
  - `LowerOptions.active_skip_levels`
    - block-delete lines (`/` => level `0`, `/n` => level `n`) are skipped when
      level is active

## Motion Packets

Status:
- `Implemented` (motion subset)

Rules:
- Packetization consumes AIL and emits packets for `G0/G1/G2/G3/G4`.
- `packet_id` is 1-based and deterministic per result order.
- Non-motion AIL instructions (for example `assign`) are skipped with a
  warning diagnostic.
- `G0` and `G1` both use packet type `linear_move`; distinguish intent via
  `packet.modal.code` (`G0` vs `G1`).

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

Planned streaming execution call sequence:

1. engine assembles one complete `G1` line
2. line is parsed and lowered into a normalized linear-move command
3. execution sink receives the normalized command
4. runtime receives the same normalized command via linear-move submission
5. runtime returns `ready`, `pending`, or `error`
6. `pending` blocks the engine until explicit resume

## G0

Syntax examples:

- `G0 X10 Y20 Z30`
- `G0 AP=90 RP=10`

Rules:

- Case-insensitive words (`g0`, `x`, `f`) are accepted.
- Mixing Cartesian and polar words in one `G0` line is rejected.

Output fields:

- source: filename/line/line_number
- modal: `group=GGroup1`, `code=G0`, `updates_state=true`
- target_pose: optional `x/y/z/a/b/c`
- feed: optional `F` (baseline parser/lowering acceptance)

Current limitations:

- Siemens rapid interpolation mode machine-actuation override behavior is not
  fully implemented in current runtime/packet behavior.

## RTLION / RTLIOF

Syntax examples:

- `RTLION`
- `RTLIOF`

Current behavior:

- Lowering to AIL emits `kind=rapid_mode` instructions.
- JSON shape includes:
  - `opcode`: `RTLION` or `RTLIOF`
  - `mode`: `linear` or `nonlinear`
- Following `G0` AIL linear instructions include `rapid_mode_effective` when a
  rapid-mode command is active in prior program order.

Current limitations:

- Runtime execution now tracks rapid-mode state transitions, but interpolation
  override behavior for machine actuation is not implemented yet.
- Packetization does not emit standalone packets for `rapid_mode`, while
  preserving `rapid_mode_effective` on `G0` linear payloads.

## G40 / G41 / G42 (Group 7 baseline)

Syntax examples:

- `G40`
- `G41`
- `G42`

Current behavior:

- Lowering emits AIL `kind=tool_radius_comp`.
- JSON shape includes:
  - `opcode`: `G40` or `G41` or `G42`
  - `mode`: `off` / `left` / `right`
- `AilExecutor` tracks the active mode in `tool_radius_comp_current`.

Current limitations:

- Cutter-path geometric compensation is not implemented in v0.
- Packetization does not emit standalone packets for `tool_radius_comp`.

## G17 / G18 / G19 (Group 6 baseline)

Syntax examples:

- `G17`
- `G18`
- `G19`

Current behavior:

- Lowering emits AIL `kind=working_plane`.
- JSON shape includes:
  - `opcode`: `G17` or `G18` or `G19`
  - `plane`: `xy` / `zx` / `yz`
- `AilExecutor` tracks the active value in `working_plane_current`.
- `G2/G3` lowering validates center words against effective plane:
  - `G17` -> `I/J`
  - `G18` -> `I/K`
  - `G19` -> `J/K`
  Invalid center words are rejected with line diagnostics.

Current limitations:

- Plane-dependent geometric behavior changes (arc interpretation/tool-comp
  coupling) are not implemented in v0.
- Packetization does not emit standalone packets for `working_plane`.

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
- AIL/packet arc output carries `plane_effective` (`xy|zx|yz`) derived from
  active working plane.

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
- dwell_mode: `seconds` (F) or `revolutions` (S)
- dwell_value: numeric value

## M Functions (Baseline)

Status:
- `Partial` (parse + validation + AIL emission + executor boundary policy)

Supported syntax:
- `M<value>` (for example `M3`, `M30`)
- `M<address_extension>=<value>` (for example `M2=3`)

Validation:
- M value must be integer `0..2147483647`
- Extended form must use equals syntax
- Extended form is rejected for predefined stop/end families:
  - `M0`, `M1`, `M2`, `M17`, `M30`

Current limitation:
- Runtime machine-function execution mapping is not implemented in this slice.

AIL output:
- Emits `m_function` instructions with:
  - source
  - value
  - optional address extension

Executor boundary behavior:
- Known predefined Siemens M values (`M0/M1/M2/M3/M4/M5/M6/M17/M19/M30/M40..M45/M70`) are accepted and advanced without machine actuation in v0.
- Unknown M values follow executor policy:
  - `error` => fault
  - `warning` => warning diagnostic and continue
  - `ignore` => continue

## Control Flow and AIL Runtime

Implemented behavior:

- `GOTO` target search:
  - `GOTOF`: forward only
  - `GOTOB`: backward only
  - `GOTO`: forward then backward
  - `GOTOC`: same search as `GOTO`, unresolved target does not fault
- Target kinds:
  - label, `N` line-number, numeric line-number, system-variable token target
- `AilExecutor` branch resolver contract:
  - injected resolver interface returns `true`, `false`, `pending`, or `error`
  - `pending` supports `wait_token` and retry timestamp
  - lambda/function overload remains as a compatibility adapter
  - shared types live in [condition_runtime.h](/home/liufang/optcnc/gcode/src/condition_runtime.h)
- Structured `IF/ELSE/ENDIF` lowering:
  - lowered to `branch_if` + internal labels + gotos (Lua-style chunk lowering)

Current limitations:

- Loop-family statements (`WHILE/FOR/REPEAT/LOOP`) are parse-only for now.
- System-variable live evaluation is runtime-resolver responsibility and not
  implemented inside parser/lowering.

## Parser Diagnostics Highlights

Implemented checks include:

- block length limit: 512 chars including LF
- invalid skip-level value (`/0.. /9` enforced)
- invalid/misplaced `N` address words
- duplicate `N` warning when line-number jumps are present
- `G4` block-shape checks
- assignment-shape baseline (`AP90` rejected; `AP=90`, `X1=10` accepted)

## Test References

- `test/messages_tests.cpp`
- `test/streaming_tests.cpp`
- `test/messages_json_tests.cpp`
- `test/regression_tests.cpp`
- `test/semantic_rules_tests.cpp`
- `test/lowering_family_g4_tests.cpp`
