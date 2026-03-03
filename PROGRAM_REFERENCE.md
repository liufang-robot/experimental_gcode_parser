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
- intermediate IR: `parseAndLowerAil(...)` returns `AilResult`
- packet stage: `parseLowerAndPacketize(...)` returns `PacketResult`
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
| `M` functions (`M<value>`, `M<ext>=<value>`) | Partial | Parse + validation only; runtime mapping pending. |
| `R... = expr` assignment | Partial | Parsed/lowered into AIL `assign`; runtime evaluator/store not implemented. |
| `N...` line number at block start | Implemented | Parsed into source metadata; duplicate-warning support for line-number jumps. |
| Block delete `/` and `/0.. /9` | Implemented | Parsed with level metadata; lowering skip policy uses `LowerOptions.active_skip_levels`. |
| `GOTO/GOTOF/GOTOB/GOTOC` + labels | Implemented | Parsed, lowered to AIL, executable in `AilExecutor`. |
| `IF cond GOTO ... [ELSE GOTO ...]` | Implemented | Lowered to AIL `branch_if`; runtime resolver selects branch. |
| Structured `IF/ELSE/ENDIF` | Implemented | Lowered to `branch_if` + internal labels + gotos. |
| `WHILE/FOR/REPEAT/LOOP` families | Partial | Parse-only in current implementation. |
| Comments `;...`, `( ... )`, `(* ... *)` | Implemented | Comment text preserved in parse output. |
| Comments `// ...` | Partial | Supported only when `ParseOptions.enable_double_slash_comments=true`. |
| Subprogram call by name (`THE_SHAPE`, `"THE_SHAPE"`) | Planned | Siemens-style subprogram invocation; parser + runtime call stack pending. |
| Subprogram repeat (`P=<n> NAME`, `NAME P<n>`) | Planned | Repeat-call semantics and looped call execution pending. |
| Subprogram return (`M17` baseline, `RET` optional) | Planned | Return-to-caller execution semantics pending runtime integration. |
| ISO compat call (`M98 P...`) | Planned | Gated by ISO compatibility mode policy. |

## Motion Packet Stage

Status:
- `Implemented` (motion subset)

Behavior:
- Converts motion/dwell AIL instructions into ordered packets.
- Packet envelope fields:
  - `packet_id` (1-based sequence)
  - `type` (`linear_move`, `arc_move`, `dwell`)
  - `source` + `modal`
  - type-specific `payload`
- Non-motion AIL instructions (such as assignment) are not packetized and emit
  warning diagnostics.

Test references:
- `test/packet_tests.cpp`
- `test/cli_tests.cpp`
- `testdata/packets/*.golden.json`

## Parse/Lower Options

Implemented options:
- `ParseOptions.enable_double_slash_comments`
  - default `false`: `// ...` diagnostics
  - `true`: `// ...` accepted as comments
- `LowerOptions.active_skip_levels`
  - block-delete lines are skipped when their level is active (`/` => level `0`)

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

## `M` Functions (Baseline)

Status:
- `Partial` (parse + diagnostics + AIL emission + executor boundary policy)

Supported syntax:
- `M<value>`
- `M<address_extension>=<value>`

Validation:
- integer range: `0..2147483647`
- extension form requires `=` syntax
- extension form is rejected for `M0/M1/M2/M17/M30`

Current limitation:
- runtime machine action mapping is not implemented yet.

AIL output:
- emits `m_function` instructions with:
  - `source`
  - `value`
  - optional `address_extension`

Executor boundary behavior:
- known predefined Siemens M values (`M0/M1/M2/M3/M4/M5/M6/M17/M19/M30/M40..M45/M70`) are accepted and advanced without machine actuation in v0
- unknown M values are handled by executor policy:
  - `error` -> fault
  - `warning` -> warning + continue
  - `ignore` -> continue

## Control Flow Runtime Notes

Implemented target search behavior:
- `GOTOF`: forward-only
- `GOTOB`: backward-only
- `GOTO`: forward then backward
- `GOTOC`: unresolved target does not fault

Branch runtime behavior:
- condition resolver returns `true|false|pending|error`
- `pending` supports event wait/retry metadata

Current limitation:
- `WHILE/FOR/REPEAT/LOOP` parse but do not yet lower to executable runtime flow.

Test references:
- `test/messages_tests.cpp`
- `test/messages_json_tests.cpp`
- `test/semantic_rules_tests.cpp`
- `test/regression_tests.cpp`
- `test/lowering_family_g4_tests.cpp`
- `testdata/messages/g4_dwell.ngc`
- `testdata/messages/g4_failfast.ngc`
