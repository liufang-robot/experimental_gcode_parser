# Program Reference: APIs and Status Matrix

## Output APIs

Primary public APIs:

- `ExecutionSession`
- injected interfaces:
  - execution sink
  - runtime
  - cancellation

Additive executable IR API:

- `AilExecutor::step(now_ms, sink, runtime)`
- uses the same `IExecutionSink` / `IExecutionRuntime` motion contract for
  motion-capable AIL instructions
- `AilExecutorOptions.initial_state` can seed modal context when an executor
  instance needs to inherit prior plane, rapid, or tool-comp state

Public parser and lowering APIs:

- `parse(...) -> ParseResult`
- `parseAndLowerAil(...) -> AilResult`

Current limitations:

- the public execution path is `ExecutionSession`; lower-level engine control is
  internal
- `AilExecutor::step(...)` currently dispatches motion and dwell instructions
  through the shared runtime path
- loops remain parse-only; the public execution path currently implements
  buffered `GOTO`/label flow and structured `IF/ELSE/ENDIF`, but not loop
  families

## Modal Metadata

Every emitted message includes `modal` metadata:

- `group`: `GGroup1` or `GGroup2`
- `code`: emitted function code such as `G0`, `G1`, `G2`, `G3`, `G4`
- `updates_state`: whether this message updates modal state

Current supported baseline:

- `G0/G1/G2/G3` -> `GGroup1`, `updates_state=true`
- `G4` -> `GGroup2`, `updates_state=false`

## Command Status Matrix

| Command | Status | Notes |
|---|---|---|
| `G0` rapid baseline | Implemented | Emits `G0Message` with target pose and feed. |
| `RTLION` / `RTLIOF` rapid interpolation mode | Partial | Lowered to AIL `rapid_mode`; full machine-actuation semantics still pending. |
| `G40` / `G41` / `G42` tool radius compensation | Partial | Lowered to AIL `tool_radius_comp`; executor tracks modal state only. |
| `G17` / `G18` / `G19` working plane | Partial | Lowered to AIL `working_plane`; executor tracks active plane state only. |
| `G1` linear | Implemented | Compatibility surfaces emit `G1Message`; execution emits normalized linear-move commands. |
| `G2` arc CW | Implemented | Emits `G2Message`; execution can dispatch normalized arc commands. |
| `G3` arc CCW | Implemented | Emits `G3Message`; execution can dispatch normalized arc commands. |
| `G4` dwell | Implemented | Emits `G4Message`; execution can dispatch normalized dwell commands. |
| `M` functions (`M<value>`, `M<ext>=<value>`) | Partial | Parse + validation + AIL boundary only; runtime machine actions are not fully mapped. |
| `R... = expr` assignment | Partial | Parsed and lowered to AIL `assign`; richer expression and write semantics are deferred. |
| `N...` line number at block start | Implemented | Parsed into source metadata with duplicate-warning support. |
| Block delete `/` and `/0.. /9` | Implemented | Parsed with skip-level metadata; lowering applies `active_skip_levels`. |
| `GOTO/GOTOF/GOTOB/GOTOC` + labels | Implemented | Parsed, lowered, and executable through public `ExecutionSession`. |
| `IF cond GOTO ... [ELSE GOTO ...]` | Partial | Parsed/lowered to `branch_if`; public execution handles the baseline expression subset. |
| Structured `IF/ELSE/ENDIF` | Partial | Lowered into label/goto control flow; public execution supports the baseline branch subset. |
| `WHILE/ENDWHILE`, `FOR/ENDFOR`, `REPEAT/UNTIL`, `LOOP/ENDLOOP` | Partial | Parse-only in current implementation. |
| Comments `;...`, `( ... )`, `(* ... *)` | Implemented | Preserved as comment items in parse output. |
| Comments `// ...` | Partial | Supported only when `ParseOptions.enable_double_slash_comments=true`. |
| Subprogram call by name (`THE_SHAPE`, `"THE_SHAPE"`) | Planned | Siemens-style call model planned; parser/runtime integration pending. |
| Subprogram repeat (`P=<n> NAME`, `NAME P<n>`) | Planned | Repeat-count call semantics planned. |
| Subprogram return (`M17` baseline, `RET` optional) | Planned | Return-to-caller runtime semantics planned. |
| ISO compat call (`M98 P...`) | Planned | Enabled only by ISO-compatibility profile option. |

## Parse and Lower Options

Parse options:

- `ParseOptions.enable_double_slash_comments`
  - `false` (default): `// ...` emits a diagnostic
  - `true`: `// ...` is accepted as a comment

Lower options:

- `LowerOptions.active_skip_levels`
  - block-delete lines (`/` => level `0`, `/n` => level `n`) are skipped when
    the level is active
