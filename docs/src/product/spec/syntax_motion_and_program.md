# SPEC: Motion and Program Syntax

## 3. Supported Syntax

### 3.1 Line structure

- optional block delete: `/`
- optional skip level: `/0` through `/9`
- optional line number: `N<integer>`
- one motion command per line in `GGroup1`
- word letters are case-insensitive
- `N` block number must appear at block start
- block length overflow is rejected deterministically

### 3.2 Numbers

- integers: `0`, `10`, `-3`, `+7`
- decimals: `1.0`, `-2.5`, `.5`, `5.`, `+.25`
- scientific notation is not supported in v0

### 3.3 G0 / G1

Supported words:

- `G0`
- `G1`
- Cartesian endpoints: `X/Y/Z/A/B/C`
- polar endpoints: `AP=...`, `RP=...`
- feed: `F...`

Rules:

- mixed Cartesian and polar endpoints in one line are rejected
- `G0` is represented as a linear-motion family member with modal code `G0`
- `RTLION` and `RTLIOF` control rapid interpolation mode in the execution
  model
- current runtime-backed axis support is limited to `X/Y/Z/A/B/C`
- supported runtime-facing variable names in this slice:
  - simple `$NAME`
  - single-selector `$NAME[part]`
- deferred in this slice:
  - multi-selector forms like `$P_UIFR[1,X,TR]`
  - feed expressions
  - arc-word expressions

Examples:

```gcode
G0 X20 Y10
G1 Z-2 F40
G1 X100 Y100 Z200
G1 AP=90 RP=10 F40
G1 G94 X100 Y20 Z30 A40 F100
G1 X=$P_ACT_X
G1 X=$AA_IM[X]
```

### 3.4 G2 / G3

Supported words:

- `G2` clockwise arc
- `G3` counterclockwise arc
- endpoints: `X/Y/Z`
- center words: `I/J/K`
- radius words: `CR`, `AR`
- polar endpoints: `AP`, `RP`
- intermediate-point form: `CIP` with `I1/J1/K1`
- tangential-circle form: `CT`

Plane behavior:

- AIL arc instructions carry `plane_effective`
- packet arc payloads carry `plane_effective`
- working-plane state constrains legal center words

### 3.5 G4

- `G4` must be programmed in a separate NC block
- exactly one dwell value is required:
  - `F...` seconds
  - `S...` spindle revolutions
- prior feed and spindle state remain valid after dwell

### 3.8 Program naming and metadata

Current baseline:

- leading `%...` metadata line is accepted as external program metadata
- normalized name must remain non-empty and alphanumeric-led after
  normalization
- inline trailing comments are excluded from normalized metadata name
- the metadata line is preserved but not lowered as an executable block

Planned follow-up:

- stricter Siemens naming rules
- richer invalid-name diagnostics under compatibility modes

### 3.9 Subprogram syntax

Current baseline:

- direct call by bare or quoted name
- repeat count forms:
  - `<name> P<count>`
  - `P=<count> <name>`
- optional ISO call:
  - `M98 P<id>` when ISO mode is enabled
- returns:
  - `RET`
  - `M17`
- baseline declarations:
  - `PROC <name>`
  - `PROC "<name>"`

Execution/runtime baseline:

- `subprogram_call` resolves against labels
- repeat count loops the same target before resuming caller
- unresolved target or empty-stack return faults

### 3.10 M functions

Baseline syntax:

- `M<value>`
- `M<address_extension>=<value>`

Baseline rules:

- value range: `0..2147483647`
- extended-address form requires `=`
- core flow M codes reject extended-address form
- known predefined Siemens M functions execute as accepted no-op control
  instructions in v0
- unknown M functions follow executor policy:
  - `error`
  - `warning`
  - `ignore`

### 3.11 Tool radius compensation

Supported words:

- `G40`
- `G41`
- `G42`

Current scope:

- parse
- AIL emission
- executor state tracking

Current limitation:

- no geometric cutter compensation is applied in v0

### 3.12 Working plane selection

Supported words:

- `G17`
- `G18`
- `G19`

Current scope:

- parse
- AIL emission
- executor state tracking
- arc-center validation against active plane

### 3.13 Tool selector syntax

Machine mode flag:

- `tool_management`

When `tool_management=false`:

- accepted:
  - `T<number>`
  - `T=<number>`
  - `T<n>=<number>`
- rejected:
  - non-numeric selector values
  - indexed form without `=`

When `tool_management=true`:

- accepted:
  - `T=<location_or_name>`
  - `T<n>=<location_or_name>`
- rejected:
  - legacy numeric shortcut without `=`

Runtime baseline:

- deferred mode stores pending tool selection until `M6`
- direct mode submits immediately
- `M6` without pending selection falls back to active selection when present
- runtime tool acceptance follows the same `Ready|Pending|Error` contract as
  motion and dwell
