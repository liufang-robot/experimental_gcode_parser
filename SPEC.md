#SPEC — CNC G - code Parser Library(v0.1)

## 1. Purpose
Parse G-code text (string or file) into a simple, syntax-level JSON model
focused on G0/G1 (linear/rapid), G2/G3 (circular), and G4 (dwell) commands.

v0.1 is **syntax-focused** (no execution, no full multi-group modal
validation). It includes message-level modal metadata for supported functions.

## 2. Input / Output

### 2.1 Input
- UTF-8 text
- Line endings: `\n` or `\r\n`
- Tabs are whitespace
- Comments (v0 current):
  - Semicolon: `; ...` to end-of-line
  - Parentheses: `( ... )` with no nesting
- Siemens comment compatibility (v0 current subset):
  - Block section: `(* ... *)` (multi-line)
- Optional Siemens compatibility mode:
  - Single-line: `// ...` (disabled by default; enable via
    `ParseOptions.enable_double_slash_comments`)

### 2.2 Output (Library + CLI)
The parser returns:

- AST (program → lines → items)
- Diagnostics with line/column (1-based)

Optional downstream stage:
- AST can be lowered into typed AIL instructions, motion packets, and queue messages.
- v0 supports `G0Message`, `G1Message`, `G2Message`, `G3Message`, and
  `G4Message` emission.
- v0 supports motion packet emission from AIL motion instructions
  (`G0/G1/G2/G3/G4`).
- Normative execution-model contract:
  - machine-executable semantics must be represented as explicit typed AIL
    instructions
  - packet emission is an execution transport for motion families, not a
    requirement for every executable instruction
  - non-motion control instructions may execute through executor/policy paths
    without standalone motion packets
- Message results support JSON conversion (`toJson`/`fromJson`) for transport,
  fixtures, and debugging.
- Additive API: streaming parse/lower output mode for large-file workflows
  (callbacks/events instead of full-result buffering), with cancel/limit
  controls.

The `gcode_parse` CLI supports:
- `--mode parse|ail|packet|lower` (default: `parse`)
- parse mode:
  - `--format debug` (default): stable, line-oriented parse format used by
    parser golden tests.
  - `--format json`: machine-readable parse JSON with top-level keys:
    `schema_version`, `program`, and `diagnostics`.
- lower mode:
  - `--format json`: machine-readable lowered-message JSON with top-level keys:
    `schema_version`, `messages`, `diagnostics`, and `rejected_lines`.
  - `--format debug`: stable, line-oriented lowered summary including emitted
    messages, rejections, diagnostics, and totals.
- ail mode:
  - `--format json`: machine-readable AIL JSON with top-level keys:
    `schema_version`, `instructions`, `diagnostics`, and `rejected_lines`.
  - `--format debug`: stable line-oriented AIL summary including instruction
    kind/opcode, source, and totals.
- packet mode:
  - `--format json`: machine-readable packet JSON with top-level keys:
    `schema_version`, `packets`, `diagnostics`, and `rejected_lines`.
  - `--format debug`: stable line-oriented packet summary including packet id,
    type, source, and totals.

AST shape (v0.1):
- Program: ordered list of `Line`
- Line:
  - `block_delete` (optional `/`)
  - `line_number` (`N` + integer)
  - `items` (ordered `Word` or `Comment`)
- Word:
  - `text` (raw token)
  - `head` (uppercased word head, e.g. `G`, `X`, `AP`, `I1`, `CIP`)
  - `value` (optional string; numeric or `AC(...)`)
  - `has_equal` (whether `=` was present)
- Comment:
  - `text` (raw token)

## 3. Supported Syntax (from testdata)

### 3.1 Line structure
- Optional block delete: `/`
- Optional block delete with skip level: `/0`..`/9`
- Optional line number: `N` + integer
- One motion command per line (GGroup1)
- Word letters are case-insensitive (`x` == `X`, `g1` == `G1`).
- `N`-address block number must appear at block start (before statement words).
- Block-skip level constraints:
- only one skip level value is allowed after `/` -
    valid range is `/ 0` through `/ 9` - Block length
    validation:-parser reports error if a
                    block exceeds 512 characters(including LF)
                        .

                ## #3.2 Numbers
               -
      Integers: `0`, `10`, `- 3`, `+ 7` -
      Decimals: `1.0`, `- 2.5`, `.5`, `5.`, `+ .25` -
                                                No scientific notation in v0.1

                                                    ## #3.3 G0
                                                    / G1(rapid +
                                                         linear interpolation
                                                             baseline) -
                                                Motion
         words: `G0`, `G1` - Cartesian
      endpoint: `X`, `Y`, `Z`, optional `A` - Polar
      endpoint: `AP = ...` and `RP = ...` -
      Feedrate: `F...` - Mixed Cartesian + polar in one line is an error -
                Baseline v0
      behavior:
          - `G0` is parsed / lowered as a linear - motion family member
                                                   with `modal.code =
              G0` - `RTLION` / `RTLIOF` are parsed / lowered into AIL rapid -
              mode
              instructions(runtime machine -
                           actuation override semantics are still planned) -
              AIL /
                  packet `G0` output carries current effective rapid mode when
                  previously set by `RTLION`/`RTLIOF` - `AilExecutor` tracks
              current rapid
              -
              mode state as it executes
    `RTLION`/`RTLIOF` instructions

                  Examples(from `testdata / g1_samples.ngc`):
``` G0 X20 Y10 G1 Z
              - 2 F40 G1 X100 Y100 Z200 G1 AP = 90 RP = 10 F40 G1 G94 X100 Y20
          Z30 A40 F100 G1 X10 AP = 90 RP = 10; mixed modes should error
```

### 3.4 G2 / G3 (circular interpolation)
- Motion words: `G2` (CW), `G3` (CCW)
- Cartesian endpoint: `X`, `Y`, `Z`
- Center point: `I`, `J`, `K`
  - Optional `=` (e.g., `I-43` or `I=-43`)
  - Absolute form with `AC(...)` (e.g., `I=AC(90)`)
- Radius: `CR=...`
- Opening angle: `AR=...`
- Polar endpoint: `AP=... RP=...`
- Intermediate point: `CIP` with `I1/J1/K1`
  - Optional `=` and `AC(...)`
- Tangential circle: `CT` with `X/Y/Z`
- Effective plane metadata:
  - AIL arc instructions include `plane_effective` (`xy|zx|yz`)
  - packet arc payloads include `plane_effective` (`xy|zx|yz`)

Examples (from `testdata/g2g3_samples.ngc`):
```
N30 G2 X115 Y113.3 I=AC(90) J=AC(70)
N30 G2 X115 Y113.3 CR=-50
N30 G2 AR=269.31 I-43 J25.52
N30 G2 AR=269.31 X115 Y113.3
G2 AP=90 RP=10
G2 CIP X10 Y5 Z0 I1=AC(1) J1=AC(2) K1=AC(3)
G2 CT X10 Y5 Z0
```

### 3.5 G4 (dwell)
- Motion/control word: `G4`
- Dwell time words:
  - `F...` time in seconds
  - `S...` time in revolutions of master spindle
- `G4` must be programmed in a separate NC block.
- Exactly one dwell mode word must be provided (`F` xor `S`).
- Any previously programmed feed (`F`) and spindle speed (`S`) remain valid
  after dwell.

Examples:
```
N10 G1 F200 Z-5 S300 M3
N20 G4 F3
N30 X40 Y10
N40 G4 S30
N50 X60
```

### 3.6 Assignment Expressions (v0)
- Statement form:
  - `<lhs> = <expr>`
- Supported `lhs` currently:
  - `R`-style variable words (for example `R1`)
- Supported expression operands:
  - numeric literals
  - variable words (for example `R2`)
  - system variables (for example `$P_ACT_X`)
- Current system-variable scope in v0:
  - simple token form only (for example `$P_ACT_X`)
  - bracketed selector forms (for example `$P_UIFR[1,X,TR]`, `$A_IN[1]`) are
    reserved for Siemens-compatibility extension tasks
- Supported operators:
  - unary: `+`, `-`
  - binary: `+`, `-`, `*`, `/`
- Parenthesized subexpressions are not supported in v0.

Example:
```
R1 = $P_ACT_X + 2*R2
```

Implemented Siemens baseline checks:
- require `=` for multi-letter or numeric-extension address values
  (for example `AP=90`, `X1=10`; `AP90` is rejected)
- single-letter + single-constant omission remains accepted
  (for example `X10`, `F100`)

Planned Siemens compatibility extension:
- add structured variable references for:
  - user-defined variables (baseline `R...`)
  - system variables with selectors (`$...` + bracket arguments)
- preserve variable-reference structure in AIL for runtime resolver usage.
- extend assignment/value-shape support for full Siemens expression forms.

### 3.8 Program Naming and Metadata (planned Siemens compatibility)
- Planned syntax support:
  - Siemens-style program names (identifier constraints) for internal naming.
  - external transfer name forms that start with `%` for compatibility parsing.
- Parser responsibility:
  - preserve raw program-name text and source location.
  - report invalid name-shape diagnostics under Siemens-compatibility mode.

### 3.9 Subprogram Syntax (planned Siemens compatibility)
- Planned syntax support:
  - direct call by program name:
    - `<subprogram_name>`
    - `"<subprogram_name>"` (quoted form compatibility)
    - `<subprogram_name> P<count>`
    - `P=<count> <subprogram_name>`
  - optional ISO-compatibility call:
    - `M98 P<program_name_or_id>` (gated by ISO mode option)
  - subprogram return/end statements:
    - `M17` (baseline Siemens return)
    - `RET` (optional compatibility extension)
  - advanced/procedural forms (planned):
    - `PROC <name>(<typed_params>)`
    - call with arguments `<name>(<args...>)`
- Current baseline implemented in v0:
  - direct subprogram call forms lower to explicit AIL `subprogram_call`:
    - `<subprogram_name>`
    - `"<subprogram_name>"`
    - `<subprogram_name> P<count>`
    - `P=<count> <subprogram_name>`
  - standalone unquoted identifier targets (for example `ALIAS`) are accepted
    as direct call forms in v0 baseline.
  - ISO compatibility call `M98 P<id>` is supported when
    `enable_iso_m98_calls=true`; otherwise parser reports a deterministic
    diagnostic.
  - `RET` and `M17` lower to explicit AIL `return_boundary` instructions.
  - baseline declaration compatibility: `PROC <name>` lowers to a non-motion
    label marker (`AilLabelInstruction`) so subprogram calls can resolve to it.
    - quoted declaration target compatibility is supported:
      `PROC "<subprogram_name_or_path>"`
  - inline parenthesized suffixes on baseline procedural forms are currently
    treated as non-executable comments:
    - empty suffix `()` is accepted as a no-argument compatibility form
      without warnings
      - both contiguous and whitespace-separated forms are accepted
        (`MAIN()`, `MAIN ()`)
    - non-empty suffix `(...)` is ignored with deterministic AIL warnings:
      - `PROC <name>(...)`
      - `<subprogram_name>(...)`
  - packet stage does not emit standalone packets for `subprogram_call`.
  - packet stage does not emit standalone packets for `return_boundary`.
  - executor call baseline:
    - `subprogram_call` resolves target against in-program labels, pushes return
      PC, and transfers control to target label
    - `repeat_count` (`P<count>` / `P=<count>`) re-enters target until count is
      exhausted before resuming caller
    - `return_boundary` pops return PC and resumes caller
    - unresolved call target or return with empty call stack is a runtime fault
  - Parser/lowering responsibility:
  - preserve call target and repeat count structure
  - preserve declaration names and return statements
  - emit warning diagnostics when inline procedural signature/call argument
    suffixes are encountered but not modeled in baseline
  - emit deterministic error for malformed declaration keyword form `PROC`
    that does not match baseline declaration shape `PROC <name>`
  - emit deterministic diagnostics for malformed call/declaration syntax
- Runtime responsibility:
  - resolve target by configured search policy
    - same-context/bare-name resolution
    - qualified-path resolution where required
  - execute with call stack and return semantics
  - fault or warn by policy on unresolved calls/invalid returns

### 3.10 M Functions (baseline parse/validation)
- Supported baseline syntax:
  - `M<value>` (for example `M3`, `M30`)
  - `M<address_extension>=<value>` (for example `M2=3`)
- Baseline validation:
  - M-function value must be integer in range `0..2147483647`
  - extended address form must use `=` (`M<ext>=<value>`)
  - extended address is rejected for `M0`, `M1`, `M2`, `M17`, `M30`
- Current scope:
  - parse + diagnostics + AIL emission (`m_function`)
  - executor boundary policy:
    - known predefined Siemens M functions are accepted as executable no-op
      control instructions in v0
    - unknown M functions follow executor policy (`error|warning|ignore`)
  - no runtime machine action mapping/actuation in this slice

### 3.11 Tool Radius Compensation (Group 7 baseline)
- Supported syntax:
  - `G40` (compensation off)
  - `G41` (left of contour)
  - `G42` (right of contour)
- Current scope:
  - parse + AIL emission (`tool_radius_comp`)
  - executor state tracking (`tool_radius_comp_current`)
  - packet stage does not emit standalone packets for this modal instruction
- Current limitation:
  - no cutter-path geometric compensation is applied in v0

### 3.12 Working Plane Selection (Group 6 baseline)
- Supported syntax:
  - `G17` (XY plane)
  - `G18` (ZX plane)
  - `G19` (YZ plane)
- Current scope:
  - parse + AIL emission (`working_plane`)
  - executor state tracking (`working_plane_current`)
  - lowering validation for `G2/G3` arc-center words against effective plane:
    - `G17`: `I/J` allowed, `K` rejected
    - `G18`: `I/K` allowed, `J` rejected
    - `G19`: `J/K` allowed, `I` rejected
  - packet stage does not emit standalone packets for this modal instruction
- Current limitation:
  - no downstream geometric remapping of arc/tool-compensation behavior is
    applied in v0

### 3.13 Tool Selector Syntax (baseline, mode-aware parse validation)
- Machine mode flag:
  - parser option `tool_management` controls selector-form validation.
- When `tool_management=false` (default):
  - accepted forms:
    - `T<number>` (for example `T12`)
    - `T=<number>` (for example `T=7`)
    - `T<n>=<number>` (for example `T1=99`)
  - rejected examples:
    - `T=DRILL` (non-numeric value)
    - `T1` (indexed form missing `=` and value)
- When `tool_management=true`:
  - accepted forms:
    - `T=<location|name>` (for example `T=DRILL_10`)
    - `T<n>=<location|name>` (for example `T1=LOC_A5`)
  - rejected examples:
    - `T12` (legacy numeric shortcut without `=`)
- Current scope:
  - parse/semantic validation
  - AIL emits explicit tool instructions:
    - `tool_select` with `selector_index?`, `selector_value`, and
      `timing` (`immediate|deferred_until_m6`)
    - `tool_change` for `M6` with `timing`
  - packet stage does not emit standalone packets for tool instructions
    (tool instructions are executable control instructions, routed to runtime
    control path rather than motion packets).
  - executor runtime baseline:
    - deferred mode: `T...` updates `pending_tool_selection`, `M6` activates it
    - direct mode: `T...` immediately updates `active_tool_selection`
    - before `M6`, last valid deferred selection wins
    - `M6` without pending selection uses policy
      (`error|warning|ignore`)
    - tool resolution policy supports outcomes:
      - `resolved`
      - `unresolved`
      - `ambiguous`
      with configurable handling (`error|warning|ignore`) and optional fallback
      selection/substitution mapping

### 3.7 Control Flow Syntax (parse-only in v0)
- Jump directions:
  - `GOTOF <target>`: forward search direction
  - `GOTOB <target>`: backward search direction
  - `GOTO <target>`: global search
  - `GOTOC <target>`: non-faulting global search (controller runtime semantics
    remain out of scope in v0 parser)
- Jump targets:
  - label (`<label>:` declaration)
  - line number token (`N100`)
  - numeric block number (`200`)
  - system variable token (for indirection at runtime)
- Single-line conditional jump:
  - `IF <condition> GOTOx <target>`
  - optional `THEN`
  - optional `ELSE GOTOx <target>`
- Structured control flow statements:
  - `IF <condition>` ... `ELSE` ... `ENDIF`
  - `WHILE <condition>` ... `ENDWHILE`
  - `FOR <var> = <start> TO <end>` ... `ENDFOR`
    - v0 parser assumes implicit step `+1` syntax-only
  - `REPEAT` ... `UNTIL <condition>`
  - `LOOP` ... `ENDLOOP`
- Supported condition operators:
  - `==`, `>`, `<`, `>=`, `<=`, `<>`
- Supported logical composition (v0 parser):
  - `AND` across condition terms (for example `IF (R1 == 1) AND (R2 > 10)`)
  - Parenthesized terms are accepted in control-flow conditions and preserved
    as raw condition terms for runtime resolver handling.
- Label names follow word-style identifiers and may include underscores.
- Duplicate `N` line numbers are accepted with warning; jumps by line number may
  become ambiguous and runtime uses directional nearest-match policy.
- Parser behavior in v0 is syntax-only: labels are not resolved and control
  flow is not executed.

Example:
```
N100 G00 X10 Y10
GOTO END_CODE
N110 (This block is skipped)
N120 (This block is skipped)
END_CODE:
N130 G01 X20 Y20
```

## 4. Non-goals (v0.1)
- Full modal group validation beyond GGroup1 single-motion rule
- Units/distance-mode state
- Full RS274NGC expressions/macros
- Full subprogram execution semantics (call stack/lookup) in v0.1

## 5. Diagnostics (v0.1)
- Syntax errors reported by the lexer/parser with line/column.
  - Messages should be actionable and prefixed with `syntax error:`.
- Semantic errors:
  - Mixed Cartesian (`X/Y/Z/A`) + polar (`AP/RP`) words in a `G1` line.
  - Multiple motion commands (`G1/G2/G3`) in a single line.
  - `G4` combined with other words in the same block when a separate block is
    required.
  - `G4` with both `F` and `S`, with neither `F` nor `S`, or with non-numeric
    dwell values.
  - Semantic messages should include an explicit correction hint (for example,
    "choose one coordinate mode", "choose only one of G1/G2/G3", or
    "program G4 in a separate block").
- Planned Siemens compatibility diagnostics:
  - malformed skip-level marker (outside `/0.. /9`)
  - invalid assignment-shape for `=` requirement/omission rules

## 6. Message Lowering (v0.1)
- Pipeline note:
  - Current supported paths are `parse -> AIL` and `parse -> AIL -> packets`,
    with `parse -> messages` retained for queue-level compatibility.
- Standalone lowering stage: AST + parser diagnostics -> queue-ready messages +
  diagnostics.
- `G1Message` fields:
  - Source: optional filename, physical line, optional `N` line number
  - Target pose: `X/Y/Z/A/B/C` (optional numeric fields)
  - Feed: `F` (optional numeric field)
- `G2Message` / `G3Message` fields:
  - Source: optional filename, physical line, optional `N` line number
  - Target pose: `X/Y/Z/A/B/C` (optional numeric fields)
  - Arc parameters: `I/J/K/R` (optional numeric fields; `CR` maps to `R`)
  - Feed: `F` (optional numeric field)
- `G4Message` fields:
  - Source: optional filename, physical line, optional `N` line number
  - Modal metadata:
    - `group`: `GGroup2`
    - `code`: `G4`
    - `updates_state`: `false` (non-modal one-shot)
  - Dwell mode: `seconds` (from `F`) or `revolutions` (from `S`)
  - Dwell value: numeric duration/revolution count
- Modal metadata policy (Siemens-preferred baseline for supported set):
  - `G1` / `G2` / `G3` belong to `GGroup1` (modal motion group) and set
    `updates_state=true` with `code` equal to the emitted motion code.
  - `G4` belongs to `GGroup2` and sets `updates_state=false`.
  - This metadata is informational in v0.1; full modal-group conflict
    validation across all groups is out of scope.
- Error-line emission policy (v0):
  - If a line has error diagnostics, do not emit motion message for that line;
    the line is reported in `rejected_lines` with reasons.
  - Lowering is fail-fast: stop lowering when the first error line is encountered.
- Severity mapping policy (v0):
  - `error`: syntax/semantic violations that reject a line and trigger fail-fast stop.
  - `warning`: non-fatal lowering limits (for example unsupported arc words);
    the message is still emitted from supported fields.
- Resume session API (v0):
  - Provide session-level edit + resume from line API for interactive workflows.
  - After line edit, reparsing preserves deterministic message ordering and
    stable source line mapping.
  - Incremental resume contract:
    - API may reparse from an explicit line or from first error line.
    - Prefix before resume line is preserved; suffix is reparsed and merged.
    - Line mapping remains 1-based and deterministic after merge.
    - Example workflow:
      - parse `G1 X10` + `G1 X12,30` -> error at line 2
      - edit line 2 to `G1 X12 Y30`
      - resume from line 2 and continue lowering.
- Queue diff/apply API (v0):
  - Provide line-keyed message diff with `added`, `updated`, and
    `removed_lines`.
  - Provide apply helper that applies a diff to an existing message queue and
    preserves deterministic line order.
- Streaming API (v0):
  - Provide callback-based streaming variants for parse/lower output.
  - Stream diagnostics/messages/rejected-lines without requiring message-vector
    accumulation by default.
  - Support early-stop controls (max lines/messages/diagnostics, cancel hook).
- JSON schema notes:
  - Include top-level `schema_version` (current value: `1`).
  - Include `messages`, `diagnostics`, and `rejected_lines`.
  - All message JSON objects carry `modal` with:
    - `group` (`GGroup1` or `GGroup2`)
    - `code` (`G1`/`G2`/`G3`/`G4`)
    - `updates_state` (boolean)
  - `G1Message` JSON carries `type`, `source`, `target_pose`, and `feed`.
  - `G2Message` / `G3Message` JSON additionally carry `arc` with
    `i/j/k/r` optional numeric fields.
  - `G4Message` JSON carries `type`, `source`, `dwell_mode`, and
    `dwell_value`.
- Packet stage (v0):
  - Packet result shape:
    - `packets`: ordered `MotionPacket` envelope list
    - `diagnostics`: parse/semantic diagnostics, plus packetization warnings
    - `rejected_lines`: fail-fast rejected lines inherited from lowering
  - `MotionPacket` fields:
    - `packet_id` (1-based sequential ID within result)
    - `type`: `linear_move`, `arc_move`, or `dwell`
    - `source`: filename/line/N-line mapping
    - `modal`: modal metadata copied from originating instruction
    - `payload`: type-specific fields
  - v0 packetization only converts motion/dwell AIL instructions.
    Non-motion AIL instructions (for example `assign`, `m_function`) are skipped
    with a
    warning diagnostic.

### 6.1 Control-Flow AIL and Executor (v0)
- AIL may include parse-lowered control-flow instructions:
  - `label`
  - `goto` (`GOTOF/GOTOB/GOTO/GOTOC`) with target kind metadata
  - `branch_if` with condition + then/else goto branches
- Structured `IF ... ELSE ... ENDIF` is lowered into linear control flow
  (Lua-like block lowering pattern) using internal labels and gotos:
  - entry `branch_if` chooses `then_label` vs `else_label`/`end_label`
  - `then` body ends with `goto end_label` when `ELSE` exists
  - exactly one branch body executes at runtime
- Runtime executor API evaluates `branch_if` conditions via callback contract:
  - callback result states: `true`, `false`, `pending`, `error`
  - `pending` may include `wait_key` and `retry_at` metadata
- Variable evaluation boundary:
  - parser/lowering does not resolve live system-variable values
  - runtime resolver is responsible for evaluating user/system-variable
    references, including pending/unavailable outcomes
- Skip-level execution boundary (v0 Siemens baseline):
  - parser captures skip marker + optional level metadata (`/` => level `0`)
  - lowering/execution stage applies skip policy from
    `LowerOptions.active_skip_levels`
  - a block-delete line is skipped only when its level is active
- Executor state model:
  - `ready`
  - `blocked_on_condition`
  - `completed`
  - `fault`
- Branch wait/retry behavior:
  - on `pending`, executor blocks at the branch instruction
  - resume on matching event (`wait_key`) or retry deadline
- Target-resolution behavior (v0):
  - `GOTOF`: forward-only lookup
  - `GOTOB`: backward-only lookup
  - `GOTO`: forward first, then backward
  - `GOTOC`: same search behavior as `GOTO`, but unresolved target does not
    fault and execution continues
- For `N`/numeric targets with multiple candidates in the chosen search
  direction, runtime selects nearest match and emits warning diagnostic.
- M-function runtime boundary (v0):
  - `m_function` instructions are present in AIL and seen by executor
  - known predefined Siemens M values:
    - `M0/M1/M2/M3/M4/M5/M6/M19/M30/M40..M45/M70`
    are accepted without machine-actuation side effects in v0
  - unknown M values are handled by executor policy:
    - `error` => fault
    - `warning` => warning diagnostic and continue
    - `ignore` => continue silently
- Return boundary runtime boundary (v0 baseline):
  - `RET` and `M17` emit explicit `return_boundary` AIL instructions
  - packet stage does not emit standalone motion packets for
    `return_boundary`
  - executor pops and resumes from call stack
  - return with empty call stack is a runtime fault
- Subprogram call runtime boundary (v0 baseline):
  - direct call forms emit explicit `subprogram_call` AIL instructions with
    optional `repeat_count`
  - ISO call form `M98 P<id>` emits explicit `subprogram_call` when ISO mode is
    enabled
  - packet stage does not emit standalone motion packets for
    `subprogram_call`
  - executor resolves target by label in current instruction stream, pushes
    return PC, and transfers control to target label
  - subprogram target search policy is configurable:
    - `ExactOnly` (default): resolve only exact label match
    - `ExactThenBareName`: try exact target first, then bare-name fallback
      (substring after last `/` or `\`)
  - executor uses pluggable `SubprogramPolicy`; default policy implements the
    two search modes above, alias-map resolution (`requested -> label`), and
    unresolved-target policy (`error|warning|ignore`)
  - when `repeat_count > 1`, executor loops call/return on same target until
    repeat count is exhausted
  - `repeat_count <= 0` is ignored with warning
  - unresolved subprogram target is a runtime fault
  - unresolved subprogram target policy is configurable:
    - `error` => fault
    - `warning` => warning diagnostic and continue
    - `ignore` => continue silently
- Tool runtime boundary (current):
  - `tool_select` / `tool_change` instructions are explicit executable control
    instructions in AIL
  - packet stage intentionally does not emit standalone motion packets for them
  - executor tracks:
    - `active_tool_selection`
    - `pending_tool_selection`
  - execution baseline:
    - `tool_select(immediate)` updates `active_tool_selection`
    - `tool_select(deferred)` updates `pending_tool_selection`
    - `tool_change` consumes pending selection when present
    - `M6` without pending selection follows configurable policy
      (`error|warning|ignore`)
  - executor integrates pluggable tool policy (`ToolPolicy`) with default policy
    implementation (`DefaultToolPolicy`) for unresolved/ambiguous/substitution
    outcomes
- Unresolved non-`GOTOC` target is runtime fault.

## 7. Testing Expectations
- Golden tests for all examples in `SPEC.md`.
- CLI tests must cover parse mode and lower mode (`json` + `debug` formats).
- CLI tests must cover parse/ail/packet/lower modes (`json` + `debug` formats).
- Maintain stage-output CLI golden fixtures in `testdata/cli/` for
  parse/ail/packet/lower deterministic output checks.
- Unit test framework: GoogleTest (`gtest`) is the required framework for new
  unit tests.
- Add message-output JSON golden tests for representative queue outputs.
  - Message fixture naming convention:
    - input: `<name>.ngc`
    - golden: `<name>.golden.json`
    - for no-filename lowering cases, use `nofilename_<name>.ngc`.
- Add packet-output JSON golden tests for representative packet outputs.
  - Packet fixture naming convention mirrors message fixtures:
    - input: `<name>.ngc`
    - golden: `<name>.golden.json`
    - for no-filename cases, use `nofilename_<name>.ngc`.
- Add AIL-output JSON golden tests for representative AIL outputs.
  - AIL fixture naming convention:
    - input: `<name>.ngc`
    - golden: `<name>.golden.json`
    - for no-filename cases, use `nofilename_<name>.ngc`.
- Property/fuzz testing: parser must never crash, hang, or use unbounded memory.
  - Minimum smoke gate: deterministic corpus + fixed-seed generated inputs in
    `test/fuzz_smoke_tests.cpp`, bounded to max input length 256 and 3000
    generated cases.
  - Runtime budget for CI smoke gate: under 5 seconds per test invocation.
- Regression tests: every fixed bug must get a test that fails first, then passes.
  - Regression test naming convention:
    `Regression_<bug_or_issue_id>_<short_behavior>`.
- Streaming callback tests must validate message field content (type/source/
  payload), not only event counts.
- Performance benchmarking:
  - Maintain a benchmark harness for deterministic corpora.
  - Include at least one 10k-line baseline scenario.
  - Report parse and parse+lower throughput metrics (time, lines/sec, bytes/sec).

## 8. Code Architecture Requirements
- Use object-oriented module design for parser/lowering function families.
- Each major function family (`G1`, `G2/G3`, and future families) must be
  implemented in separate classes/files.
- Do not implement all parser/lowering family behavior in a single monolithic
  source file.
- New function families must be added as new modules rather than extending one
  large file with long branching chains.

## 9. Documentation Policy (mdBook + Reference Manual)
To keep product-facing behavior documentation aligned with code, maintain
mdBook documentation under:
- `docs/book.toml`
- `docs/src/`
- Program reference content in `docs/src/program_reference.md`
- Development reference content in `docs/src/development_reference.md`
- Product-level goals, scope, and public API expectations in `PRD.md`.

Requirements:
- Every feature/function PR must update the program reference in the same PR:
  - `PROGRAM_REFERENCE.md` (repo root reference)
  - and/or `docs/src/program_reference.md` (mdBook reference)
  If behavior is unchanged, explicitly state "no program-reference change" in PR.
- Architecture/design documentation must also be published under `docs/src/`
  (mdBook), not only as repo-root markdown files.
  - If design docs exist at root (for example `ARCHITECTURE.md`,
    `IMPLEMENTATION_PLAN.md`), keep matching mdBook pages in sync.
- If a documentation page becomes too long, split it into smaller topical files
  and link them through `docs/src/SUMMARY.md`.
  - Prefer one topic per page (for example parser pipeline, modal strategy,
    runtime executor, migration plan) rather than a single monolithic file.
- The program reference must describe currently implemented parser/lowering behavior,
  not only planned behavior.
- Every documented command/function must include a status:
  - `Implemented`
  - `Partial`
  - `Planned`
- The manual must include, per command/function:
  - accepted syntax forms
  - emitted message shape/fields (if any)
  - diagnostics/rejections rules
  - at least one example
  - test references (unit/golden/regression names or files)
- If code/API/function behavior changes, the same PR must update mdBook docs
  (`docs/src/program_reference.md` and/or `docs/src/development_reference.md`
  as appropriate).
- If `SPEC.md` adds planned behavior not yet implemented, docs must mark
  it as `Planned` until code/tests are merged.
- CI must build mdBook documentation and publish Pages content from `main`.

PR gate expectation:
- Feature PR description must list:
  - updated sections in `SPEC.md`
  - updated sections in `docs/src/program_reference.md` and/or
    `docs/src/development_reference.md`
  - tests proving the documented behavior
