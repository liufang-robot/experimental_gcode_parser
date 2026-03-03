# PRD — CNC G-code Parser Library

## 0. Review Guide
Suggested review order:
1. Confirm scope and non-goals in Sections 1-3.
2. Confirm API constraints in Section 4.
3. Review Siemens functional requirements in Section 5 by domain:
   - comments
   - tool/tool-change
   - modal groups and dimensions/units
   - feed and motion transition behavior
   - rapid traverse
   - M-code behavior
4. Mark each subsection in Section 5 as:
   - `Approved`
   - `Needs Clarification`
   - `Out of Scope`

## 0.1 Requirement Index (Section 5)
- `5.3` Comments (`;`, `(*...*)`, optional `//`)
- `5.4` Tool change (with/without tool management)
- `5.5` Modal Group 7 (G40/G41/G42)
- `5.6` Working plane (G17/G18/G19)
- `5.7` Modal Group 15 feed semantics (G93/G94/G95/... + FGROUP/FL/FGREF)
- `5.8` Modal Group 14 dimensions + Group 13 units + DIAM family
- `5.9` Modal Group 8 settable work offsets + suppress commands
- `5.10` Modal Groups 10/11/12 exact-stop and continuous-path behavior
- `5.11` Rapid traverse (`G0`, `RTLION`, `RTLIOF`)
- `5.12` M-code syntax/semantics
- `5.13` Incremental parse/edit-resume API
- `5.14` System variables and user-defined variables

## 0.2 Acceptance Fixture (Integrated)
Reserved integrated scenario for end-to-end acceptance after implementation:
- `testdata/integration/simple_integrated_case.ngc`
- Purpose:
  - validate mixed modal/motion/program-flow syntax in one realistic file
  - verify parse/lower/runtime line mapping and deterministic outputs

## 1. Product Purpose
Provide a production-usable C++ library that converts G-code text/files into:
- syntax/semantic diagnostics with precise source locations
- typed queue-ready messages for downstream motion systems

Primary users:
- CNC application/backend developers integrating parsing into planners,
  simulators, editors, and execution pipelines.

## 2. Product Goals
- Deterministic parsing and lowering results for identical input.
- Actionable diagnostics (line/column + correction hints).
- Stable API surface for integrators.
- Test-first delivery: golden + regression + fuzz smoke.

## 3. Non-Goals
- Executing machine motion.
- Full controller runtime semantics (override, real-time spindle dynamics).
- Full macro/subprogram/control-flow implementation in v0.

## 4. API Product Requirements

### 4.1 Main API Shape
Recommendation:
- expose a class facade as the main public integration surface, while keeping
  free-function entry points for backward compatibility.

Required facade (target shape):
- `class GCodeParser`
  - `ParseResult parseText(std::string_view input, const ParseOptions&)`
  - `ParseResult parseFile(const std::string& path, const ParseOptions&)`
  - `MessageResult parseAndLowerText(std::string_view input, const LowerOptions&)`
  - `MessageResult parseAndLowerFile(const std::string& path, const LowerOptions&)`
  - streaming variants for large input (callback/event or pull API):
    - `parseAndLowerStream(...)` with diagnostic/message callbacks
    - cancellation/early-stop support

Compatibility policy:
- Existing free APIs remain supported unless a major-version change is approved.

### 4.2 File Input Requirement
- Library API must support reading directly from file path (not only string).
- File read failures must return diagnostics (not crashes).

### 4.3 Output Requirement
Parser output:
- `ParseResult`
  - AST (`Program -> Line -> Item`)
  - diagnostics (`severity`, `message`, `location`)

Lowering output:
- `MessageResult`
  - typed `messages` queue entries (`G1/G2/G3/G4` currently)
  - accumulated diagnostics
  - `rejected_lines` for fail-fast policy

Output access points:
- library return values from parse/lower APIs
- optional JSON via message JSON conversion (`toJsonString` / `fromJsonString`)
- CLI output via `gcode_parse --format debug|json`

## 5. Functional Scope

### 5.1 Current Scope (v0)
- `G1`, `G2`, `G3`, `G4` syntax + diagnostics + lowering to typed messages.
- fail-fast lowering at first error line.
- JSON round-trip support for message results.

### 5.2 Next Scope (v0.2+)
- prioritized extension of additional G/M families.
- improved syntax coverage and diagnostics detail.
- API facade hardening and file-based API convenience.

### 5.3 Siemens Comment Syntax Requirement (New)
Requirement intent:
- Match Siemens-style comment behavior used in Sinumerik-style programs.

Required comment forms:
- Single-line comment with semicolon:
  - `; ...` until end-of-line
  - may appear at line start or after code
- Block comment section with `(*` ... `*)`:
  - may span multiple lines
  - parser should treat enclosed text as comment content
- Optional environment-specific single-line comment with `// ...`:
  - keep as configurable/compatibility mode requirement
  - do not enable by default unless explicitly specified in SPEC/task

Examples:
- `N10 G91 G64 F100 ; Continuous-path mode`
- `(* This is a multi-line comment section *)`
- `// Optional single-line style in some Siemens environments`

Acceptance expectation:
- comment text is preserved in parse output
- comments do not alter executable token order/semantics outside comment span
- malformed/unclosed block comment returns actionable diagnostic

### 5.4 Siemens Tool-Change Semantics Requirement (New)
Requirement intent:
- Support Siemens-style tool-call and tool-change semantics across machine
  configurations, including with/without tool management.

#### 5.4.1 Without Tool Management
Behavior split depends on machine type/configuration.

Direct change mode (typically turning/circular magazine):
- `T...` performs immediate tool change and offset activation.
- Supported forms:
  - `T<number>`
  - `T=<number>`
  - `T<n>=<number>` (machine-dependent spindle extension)
- Deselect forms:
  - `T0`
  - `T0=<number>`
- Meaning:
  - `T` includes selection + change + offset activation.
  - `T0` deselects active tool.

Preselect + M6 mode (typically milling/chain/rotary/box magazine):
- `T...` only preselects.
- `M6` executes tool change and activates selected tool/offset.
- Supported forms:
  - tool select: `T<number>`, `T=<number>`, `T<n>=<number>`
  - tool change: `M6`
  - deselect: `T0`, `T0=<number>`
- Meaning:
  - `T` preselects only.
  - `M6` performs actual swap + activation.

#### 5.4.2 With Tool Management
Tool calls may be by location or name; controller may apply substitution rules.

Direct change mode (typically turning/circular magazine):
- `T=...` directly changes tool and activates offset.
- Supported forms:
  - `T=<location>`
  - `T=<name>`
  - `T<n>=<location>`
  - `T<n>=<name>`
  - deselect: `T0`
- Special behavior:
  - selecting an empty location behaves like `T0`.
  - controller may substitute valid equivalent tool by tool-group/search policy.

Preselect + M6 mode (typically milling/chain/box magazine):
- `T=...` preselects by location/name.
- `M6` executes change and activates selected tool/offset.
- Supported forms:
  - select: `T=<location>`, `T=<name>`, `T<n>=<location>`, `T<n>=<name>`
  - change: `M6`
  - deselect: `T0`
- Special behavior:
  - empty selected location treated as no-tool / `T0` behavior.

#### 5.4.3 Product-Level Constraints
- Tool-change semantics must be mode-driven by configuration, not hardcoded to
  one machine style.
- Parser and runtime model must preserve:
  - raw command form
  - resolved intent (`select`, `change`, `deselect`)
  - timing (`immediate` vs `deferred-until-M6`)
  - optional spindle extension (`n`)
  - selector kind (`number`, `location`, `name`)
- Tool-management substitution logic should be abstracted behind runtime policy
  interfaces (controller-specific behavior pluggable).

Acceptance expectation:
- PRD-level behavior is fully modeled in architecture and task plan before code.
- Tests must cover both direct-change and preselect+M6 flows, with and without
  tool management.

### 5.5 Siemens Modal Group Requirement: Tool Radius Compensation (New)
Requirement intent:
- Tool radius compensation commands must be represented with Siemens-compatible
  modal grouping in parser/lowering/runtime outputs.

Required modal group mapping:
- Modal Group 7: Tool radius compensation
  - `G40`: no tool radius compensation
  - `G41`: tool radius compensation left of contour
  - `G42`: tool radius compensation right of contour

Product-level constraints:
- Modal metadata model must support Group 7 explicitly (not overloaded into
  motion/non-modal placeholders).
- State transitions for Group 7 must be deterministic and independently tracked
  from motion group state.
- Parse/AIL/message outputs must preserve the original G-code and normalized
  modal-group interpretation.

Acceptance expectation:
- Architecture defines where Group 7 state lives and how it propagates through
  parse -> AIL -> packet/message stages.
- Tests cover Group 7 command recognition and modal-state transitions.

### 5.6 Siemens Working-Plane Selection Requirement (G17/G18/G19) (New)
Requirement intent:
- Support Siemens-style modal working-plane selection semantics and propagate
  selected plane impact across interpolation/compensation behavior.

Required commands:
- `G17`, `G18`, `G19` (modal plane-selection commands)

Required meanings:
- `G17`: working plane `XY`, infeed/tool-length direction `Z`
- `G18`: working plane `ZX`, infeed/tool-length direction `Y`
- `G19`: working plane `YZ`, infeed/tool-length direction `X`

Cross-feature coupling requirements:
- Selected working plane defines:
  - plane for tool radius compensation (`G40/G41/G42`, Group 7)
  - infeed direction for tool length compensation
  - plane used for circular interpolation (`G2/G3`)

Product-level constraints:
- Working-plane state must be modeled as modal state with deterministic updates.
- Arc interpretation (`G2/G3`) must be plane-aware.
- Radius-compensation interpretation must be plane-aware.
- Parse/AIL/message outputs must preserve active plane context and transitions.

Acceptance expectation:
- Architecture explicitly defines plane-state ownership and propagation across
  parse -> AIL -> runtime/message stages.
- Tests cover:
  - `G17/G18/G19` recognition and modal persistence
  - plane-dependent arc interpretation
  - plane-dependent tool radius compensation behavior mapping

### 5.7 Siemens Feedrate Semantics Requirement (Modal Group 15) (New)
Requirement intent:
- Support Siemens-style feedrate programming semantics for path axes,
  synchronized axes, and mixed linear/rotary motion.

Required commands/syntax:
- feed/modal-group-15 modes:
  - `G93`, `G94`, `G95`
  - `G96`, `G97`
  - `G931`, `G961`, `G971`
  - `G942`, `G952`, `G962`, `G972`, `G973`
- feed value: `F...` (including expression-based forms)
- path group: `FGROUP(<axis1>, <axis2>, ...)`
- rotary reference radius: `FGREF[<rotary_axis>] = <value>`
- axis speed limit: `FL[<axis>] = <value>`

Required meanings:
- `G93` inverse-time feed: feed value determines block execution time
  (larger `F` => shorter block duration).
- `G94` linear feed (mm/min, inch/min, or deg/min by context).
- `G95` feed per spindle revolution (mm/rev or inch/rev).
- `G96/G97` constant-cutting-rate related feed-mode switching (Siemens Group 15
  semantics, including spindle-speed-limitation interaction).
- `G931/G961/G971/G942/G952/G962/G972/G973` must be represented as distinct
  modal states under Group 15 with deterministic transitions.
- `F` applies to path axes in active `FGROUP`.
- axes outside active `FGROUP` are synchronized axes (same end time constraint).
- `FL[axis]` limits axis speed (commonly synchronized axes); path speed may be
  reduced to satisfy coordinated end-time behavior.
- `FGREF[rotary]` provides effective radius for rotary-axis feed conversion
  when rotary axes participate in path-feed grouping.

Modal/unit constraints:
- `G93/G94/G95` are modal; switching mode should require explicit `F`
  reprogramming in behavior contract.
- `FGROUP()` with empty args restores default grouping (machine-data initial
  setting).
- feed/limit units must follow Siemens unit context (`G700/G710` behavior in
  requirement model).

Cross-feature coupling requirements:
- arc/helical motion uses plane/path semantics from working-plane selection and
  path-group definitions.
- feed state must be represented in a way compatible with future tool/comp and
  runtime execution logic.

Acceptance expectation:
- architecture defines feed-mode state ownership and propagation across
  parse -> AIL -> runtime/message stages.
- tests must cover:
  - mode switching and `F` persistence/reprogramming
  - Group 15 state transitions across all supported mode codes
  - `FGROUP` path/synchronized axis partition behavior
  - `FL` speed-limit interaction with coordinated motion timing
  - `FGREF` effect for mixed linear/rotary feed interpretation.

### 5.8 Siemens Dimensions and Unit Semantics Requirement (Modal Group 14 + related) (New)
Requirement intent:
- Support Siemens dimension programming semantics for absolute/incremental
  input, unit modes, rotary absolute targeting, and turning diameter/radius
  interpretation.

Modal group mapping:
- Modal Group 14: workpiece measuring absolute/incremental
  - `G90`: absolute dimension
  - `G91`: incremental dimension

Modal group mapping (units):
- Modal Group 13: workpiece measuring inch/metric
  - `G70`: inch input system for geometric lengths
  - `G71`: metric input system for geometric lengths
  - `G700`: inch input system for lengths + technological length data
    (including velocity/feed-related length units and relevant system-variable context)
  - `G710`: metric input system for lengths + technological length data
    (including velocity/feed-related length units and relevant system-variable context)

Required commands/syntax:
- absolute/incremental modal:
  - `G90`, `G91`
- non-modal per-value overrides:
  - `AC(...)` absolute value override
  - `IC(...)` incremental value override
- rotary absolute targeting (non-modal, G90/G91 independent):
  - `DC(...)`, `ACP(...)`, `ACN(...)`
- geometry/extended units:
  - `G70`, `G71` (geometry-only unit switching)
  - `G700`, `G710` (geometry + technological length-unit switching)
- turning diameter/radius programming:
  - channel-level: `DIAMON`, `DIAM90`, `DIAMOF`, `DIAMCYCOF`
  - axis-level: `DIAMONA[...]`, `DIAM90A[...]`, `DIAMOFA[...]`,
    `DIACYCOFA[...]`, `DIAMCHANA[...]`, `DIAMCHAN`
  - non-modal axis-value commands:
    `DAC(...)`, `DIC(...)`, `RAC(...)`, `RIC(...)`

Required meanings:
- `G90/G91` define modal absolute vs incremental interpretation.
- `AC/IC` allow local non-modal override of one value while modal state stays.
- `DC/ACP/ACN` force rotary absolute target with path-direction constraints
  (shortest/positive/negative).
- `G70/G71` affect geometry data only (position data, arc data, pitch,
  programmable zero offset `TRANS`, polar radius `RP`).
- `G700/G710` affect both geometry and technological length data, including
  feed/velocity length units and related variable interpretation.
- Turning DIAM* behavior must be modeled as configurable interpretation on
  permitted axes (machine-data dependent axis eligibility).

Cross-feature coupling requirements:
- Group 14 state must couple with arc/center parameter interpretation (`I/J/K`)
  and with turning diameter/radius modes.
- Unit state must consistently couple with feed model requirements in Section 5.7.

Acceptance expectation:
- architecture defines state ownership and propagation for Group 14 + unit +
  diameter/radius modes across parse -> AIL -> runtime.
- tests must cover:
  - `G90/G91` persistence and `AC/IC` local override behavior
  - `DC/ACP/ACN` directional rotary targeting representation
  - `G70/G71/G700/G710` unit-state transitions
  - DIAM* and DAC/DIC/RAC/RIC mode/value interpretation contracts.

### 5.9 Siemens Settable Work Offset Requirement (Modal Group 8 + suppress commands) (New)
Requirement intent:
- Support Siemens settable work-offset selection and suppression semantics for
  multi-origin machining programs.

Modal group mapping:
- Modal Group 8: settable zero offset
  - `G500`: deactivate settable work offset / activate base-frame behavior
  - `G54`, `G55`, `G56`, `G57`: settable work offsets 1..4
  - `G505` .. `G599`: additional settable work offsets (machine-dependent range)

Related suppress commands (non-modal context controls):
- `G53`: suppress settable + programmable work offsets in block context
- `G153`: same suppression as `G53`, plus suppress basic frame
- `SUPA`: same suppression as `G153`, plus suppress DRF/overlaid movement/
  external zero offset/PRESET offset

Required meanings:
- `G54..G57/G505..G599` select active workpiece origin for subsequent motion.
- `G500` deactivates currently active settable work offset (with Siemens base
  frame behavior distinctions to be modeled by configuration/policy).
- `G53/G153/SUPA` act as suppression context controls and are not equivalent to
  persistent Group 8 selection state.

Platform/config notes:
- availability/range of offsets is machine-dependent (for example 828D-specific
  differences such as `G58/G59` usage vs `G505/G506`).
- default startup offset/state may be machine-data controlled and must be
  modeled as configuration, not hardcoded behavior.

Product-level constraints:
- parser/lowering/runtime must preserve:
  - selected offset code
  - offset selection persistence (modal Group 8)
  - per-block suppression commands and their scope
- state model must compose with existing coordinate/dimension/unit states.

Acceptance expectation:
- architecture defines ownership/propagation of Group 8 state and suppression
  context across parse -> AIL -> runtime/message stages.
- tests cover:
  - Group 8 modal transitions (`G500`, `G54..`, `G505..`)
  - suppression behavior representation (`G53`, `G153`, `SUPA`)
  - machine-configuration variants for available offset ranges.

### 5.10 Siemens Exact-Stop and Continuous-Path Requirement (Modal Groups 10/11/12) (New)
Requirement intent:
- Support Siemens motion-transition control semantics for exact stop and
  continuous-path smoothing, including exact-stop block-change criteria.

Modal group mappings:
- Modal Group 10: exact stop / continuous-path mode
  - `G60` exact stop (modal)
  - `G64` continuous path
  - `G641` continuous path with distance-based smoothing (`ADIS`/`ADISPOS`)
  - `G642` tolerance-based smoothing
  - `G643` tolerance-based block-internal smoothing
  - `G644` smoothing with max dynamic response (with transformation fallback behavior)
  - `G645` smoothing with tangential transition behavior
- Modal Group 11: non-modal exact stop
  - `G9` exact stop for current block only
- Modal Group 12: exact-stop block-change criterion (effective only with `G60`/`G9`)
  - `G601` exact stop fine
  - `G602` exact stop coarse
  - `G603` interpolator end criterion

Required parameter semantics:
- `ADIS=...` and `ADISPOS=...` with `G641`
  - `ADIS`: distance criterion for path motions (`G1/G2/G3...`)
  - `ADISPOS`: distance criterion for rapid (`G0`)
  - if omitted in `G641`, default behavior corresponds to Siemens-noted zero
    criterion effect (equivalent to baseline `G64` transition behavior contract).

Required meanings:
- `G60` activates modal exact-stop behavior until replaced by Group 10 mode
  (for example `G64...`).
- `G9` applies exact stop in current block only.
- `G601/G602/G603` define when block change is allowed in exact-stop context;
  they do not independently enable exact-stop mode.
- `G64...` modes prioritize velocity continuity and allow controlled contour
  rounding/smoothing according to selected mode and parameters.

Cross-feature coupling requirements:
- behavior must compose with feed model (Group 15), plane selection, and
  compensation/tool states without undefined precedence.
- transition-mode state must be represented in parse -> AIL -> runtime outputs.

Acceptance expectation:
- architecture defines Group 10/11/12 state ownership and precedence rules.
- tests must cover:
  - modal vs non-modal exact-stop behavior (`G60` vs `G9`)
  - Group 12 criterion applicability only under exact-stop context
  - `G64..G645` mode transitions and `ADIS/ADISPOS` parameter capture.

### 5.11 Siemens Rapid Traverse Requirement (G0, RTLION, RTLIOF) (New)
Requirement intent:
- Support Siemens rapid-traverse positioning behavior for non-cutting moves,
  including interpolation-mode control for `G0`.

Required commands/syntax:
- `G0 X... Y... Z...`
- `G0 AP=...`
- `G0 RP=...`
- `RTLION`
- `RTLIOF`

Required meanings:
- `G0` activates rapid traverse (modal) for positioning/retract/approach style
  moves; it is not a machining feed mode.
- `RTLION`: rapid-traverse linear interpolation behavior.
- `RTLIOF`: rapid-traverse nonlinear/positioning-axis behavior.
- `G0` must be explicit (`G` alone is not a valid replacement in behavior contract).

Operational constraints:
- rapid-traverse timing is constrained by axis rapid limits and coordinated
  multi-axis timing behavior.
- even with `RTLIOF`, linear interpolation may be forced under specific active
  modes (for example CP mode, compensation, compressor, transformation);
  architecture must model these as override conditions, not implicit side effects.

Cross-feature coupling requirements:
- interaction with Group 10 CP/exact-stop modes and compensation states must be
  explicitly defined.
- parse/AIL/runtime outputs must preserve active rapid-interpolation mode
  (`RTLION`/`RTLIOF`) and effective rapid behavior decisions.

Acceptance expectation:
- architecture defines `G0`/RTLI state ownership and override precedence.
- tests must cover:
  - `G0` endpoint parsing (Cartesian + polar forms)
  - `RTLION/RTLIOF` state transitions
  - representative override cases where linear rapid is forced.

### 5.12 Siemens M-Code Syntax and Semantics Requirement (General) (New)
Requirement intent:
- Support Siemens-style miscellaneous function commands (`M` codes) as
  first-class non-geometric control instructions in parser/lowering/runtime.

Required syntax:
- `M##`
- `M<address_extension>=<value>` for supported extended-function forms
- placement:
  - standalone block, or
  - alongside motion/other commands in the same block where valid

Address/data model constraints:
- `M` value type: integer
- value range: `0 .. 2147483647` (INT max)
- extended address notation allowed only for supported function families
  (not universally valid for all M-codes)

Predefined Siemens M-functions to model as baseline:
- stop/program flow:
  - `M0` programmed stop
  - `M1` optional stop
  - `M2` end of program (main; equivalent behavior context with `M30`)
  - `M17` end of subprogram
  - `M30` end of program (main; equivalent behavior context with `M2`)
- spindle:
  - `M3` spindle clockwise
  - `M4` spindle counter-clockwise
  - `M5` spindle stop
  - `M19` spindle positioning
  - `M70` spindle to axis mode
- tool:
  - `M6` tool change (default semantics)
- gear:
  - `M40` automatic gear change
  - `M41..M45` gear stages 1..5

Extended-address constraints:
- extended address notation is valid for supported spindle functions (e.g.
  spindle index selection such as `M2=3` for spindle-specific execution model).
- extended address notation is not valid for Siemens-marked core flow-control
  M-functions (`M0`, `M1`, `M2`, `M17`, `M30`).

Execution-order constraints:
- `M0`, `M1`, `M2`, `M17`, `M30` are triggered after traversing movement in
  their programmed block.

Machine-manufacturer extensibility:
- free M numbers are machine-specific and may map to different behavior across
  machines; architecture must support pluggable mapping/policy rather than
  hardcoded semantics for all non-predefined numbers.

Block-level constraints to capture:
- Siemens example allows up to five M functions in one block (machine/config
  dependent; treat as configurable validation limit in architecture).

Product-level constraints:
- M-code parsing must preserve:
  - raw code/value
  - normalized function identity
  - source location and block association
- architecture must define execution/lowering boundaries:
  - which M-codes are parse-only
  - which M-codes emit executable AIL/runtime instructions
  - interaction with tool/spindle/coolant/feed/modal state models

Acceptance expectation:
- architecture defines M-code classification and handling pipeline.
- tests cover:
  - syntax parsing (`M##`, extended forms)
  - integer range and invalid-form diagnostics
  - extended-address validity constraints per M-function family
  - predefined M-function semantic mapping coverage
  - mixed-block placement behavior
  - block-level M-function count policy behavior.

### 5.13 Incremental Parse/Edit-Resume Requirement (New)
Requirement intent:
- Support interactive editing workflows where parsing/lowering resumes from a
  specific line (typically first error line) without reprocessing unaffected
  prefix behavior.

Primary workflow:
1. Parse file text and stop on first error line under fail-fast policy.
2. User edits the failing line.
3. API reparses from that line and continues, preserving prior valid prefix.

Example:
- initial:
  - `G1 X10`
  - `G1 X12,30` (error)
- user fixes line 2 to `G1 X12 Y30`
- session resumes from line 2 and continues.

API expectations:
- session API exposes:
  - `firstErrorLine()`
  - `applyLineEdit(line, new_line)`
  - `reparseFromLine(line)`
  - `reparseFromFirstError()`
- results preserve deterministic line-to-message mapping and ordering.

Product-level constraints:
- unaffected lines before resume point remain stable (no unexpected reordering).
- diagnostics/messages for resumed suffix are line-correct and merged with kept
  prefix state in deterministic order.
- behavior works for text and file-backed editor workflows.

Acceptance expectation:
- architecture defines incremental session model and merge semantics for
  prefix/suffix results.
- tests cover:
  - resume from explicit line
  - resume from first error line
  - line-fix scenario transitions from error to valid output.

### 5.14 Variables: System Variables + User-Defined Variables
Requirement intent:
- Support Siemens-compatible variable usage for both user-defined variables and
  system variables in expressions, conditions, and control flow.

Required variable categories:
- User-defined variables:
  - baseline `R` variables (for example `R1`, `R2`)
- System variables:
  - Siemens-style `$...` variable names (for example `$P_ACT_X`)
  - structured forms with selectors (for example `$P_UIFR[1,X,TR]`,
    `$A_IN[1]`)

Required usage forms:
- assignment:
  - `R1 = $P_UIFR[1,X,TR]`
  - `R2 = $A_IN[1]`
- expressions:
  - `R3 = $P_ACT_X + 2*R2`
- conditions:
  - `IF $A_IN[1] > 0 GOTOF READY`

Behavior boundary (must be explicit in design/spec):
- Parse/lowering stage:
  - parse and preserve variable references structurally
  - validate syntax shape only
  - do not fetch live machine values
- Runtime stage:
  - resolve variable values through resolver/policy hooks
  - enforce access/write permissions
  - handle timing semantics (preprocessing-time vs main-run variables)

Product-level constraints:
- Variable access rules are machine/profile controlled (not hardcoded).
- Diagnostics must clearly separate:
  - parse-time syntax errors
  - runtime access/protection failures
  - runtime pending/unavailable-value outcomes

Acceptance expectation:
- SPEC defines variable reference syntax and parse/runtime responsibilities.
- Architecture/backlog include resolver and policy interfaces for runtime
  evaluation.
- Tests cover user-defined and system-variable references in assignment and
  condition contexts.

### 5.4 Siemens Fundamental NC Syntax Principles (Chapter 2 baseline)
Requirement intent:
- Add Siemens-compatible baseline grammar/format rules for NC program identity,
  block structure, value assignment, comments, and skip blocks.

Required coverage:
- Program naming:
  - parser accepts Siemens-style program identifiers and optional external
    transfer naming prefixes (for example `%...`) as compatibility syntax.
  - internal model preserves full raw program name text.
- Block structure:
  - block number form `N<positive_int>` at block start
  - block end by line feed
  - block length limit support (Siemens baseline 512 chars, including comment and
    LF) with deterministic diagnostics on overflow
  - repeated-address allowances where valid (for example multiple `G`, `M`, `H`)
- Value assignment rules:
  - support Siemens `=` requirement contract:
    - mandatory when multi-letter address or expression value is used
    - optional for single-letter + single-constant forms
  - support numeric extension disambiguation forms (for example `X1=10`)
- Comments:
  - semicolon end-of-block comments as baseline
  - preserve comment text for display/debug output
- Skip blocks:
  - support `/` block-skip marker and skip levels `/0.. /9`
  - preserve skip-level metadata in parse/AIL outputs
  - runtime may skip execution based on configured active levels

Behavior boundary:
- Parse stage:
  - recognize and validate syntax/shape
  - preserve raw text/metadata (name, skip level, comments)
- Runtime stage:
  - decide effective skip execution by active skip-level configuration

Acceptance expectation:
- SPEC defines syntax/diagnostics for naming/block/assignment/comment/skip forms.
- Backlog tasks split parser and runtime responsibilities for skip-level handling.
- Tests cover valid/invalid examples for each category.

### 5.5 Siemens Subprograms (MPF/SPF) and Call Semantics
Requirement intent:
- Support Siemens-style subprogram organization and invocation semantics, with
  Siemens-first behavior and optional ISO-compat mode.

Required syntax and behavior:
- Direct subprogram call by program name in main program (`.MPF`):
  - example: `POCKET_FINISH`
- Quoted-name call compatibility:
  - example: `"POCKET_FINISH"`
- Repeated call count with `P`:
  - examples: `DRILL_HOLE P5`, `P=5 DRILL_HOLE`
- Optional ISO-compat mode call form:
  - `M98 P<program_name_or_id>` (enabled only when ISO compatibility is on)
- Subprogram return/end forms:
  - `M17` is required Siemens baseline return from subprogram
  - `RET` may be supported as compatibility extension by profile
  - main program end remains `M2`/`M30` semantics

Program/file organization expectations:
- Main program commonly `.MPF`; subprogram commonly `.SPF`
- Program manager storage models (global SPF vs workpiece-local) are represented
  as resolver/search policy, not hardcoded paths in parser.
- Call target resolution policy:
  - if subprogram is in same folder/context, bare name resolution is allowed
  - otherwise full/qualified path resolution is required by policy configuration

Advanced parameter passing (planned):
- declaration-like form (for example `PROC MY_SUB(REAL _DEPTH, REAL _FEED)`)
- call with arguments (for example `MY_SUB(10.5, 200)`)
- parser must preserve signature/argument structure; runtime binding policy is
  resolved by subprogram execution engine.

Behavior boundary:
- Parse/lowering stage:
  - recognize call statements, repeat count, declaration/signature forms, and
    subprogram end statements
  - build structured call/declaration instructions with source location
  - do not perform file-system lookup or execute call stack
- Runtime stage:
  - resolve subprogram targets by configured search policy
  - manage call stack/return points
  - enforce recursion/depth/lookup error policies

Acceptance expectation:
- SPEC defines subprogram syntax contract and diagnostics.
- Backlog includes parser, AIL lowering, and runtime call-stack execution slices.
- Tests cover:
  - direct name call, repeated call (`P`)
  - `M17`/`RET` return behavior
  - unresolved target diagnostics and ISO-mode gating for `M98`.

## 6. Command Family Policy
- `G` codes:
  - Implement only command families explicitly listed in SPEC + backlog tasks.
- `M` codes:
  - parse visibility is allowed; lowering behavior must be explicitly specified
    before implementation.
- Vendor/extend codes:
  - require explicit spec entry + tests before treated as supported behavior.
- Variable families:
  - user-defined and system-variable handling must be explicitly specified in
    SPEC before runtime semantics are enabled.

## 7. Quality Requirements
- Must not crash/hang on malformed input.
- Must keep diagnostics and queue ordering deterministic.
- Must pass `./dev/check.sh` for merge.
- Must include regression tests for every bug fix.
- Must support large-input workflows through streaming APIs without requiring
  full output buffering in memory.
- Must provide benchmark visibility:
  - baseline scenario includes 10k-line input
  - report total parse/parse+lower time and throughput (lines/sec, bytes/sec)

## 8. Documentation and Traceability
- `PRD.md`: product goals, scope, API contract, prioritization.
- `SPEC.md`: exact syntax/diagnostics/lowering behavior contract.
- `PROGRAM_REFERENCE.md`: implementation snapshot (`Implemented/Partial/Planned`).
- `BACKLOG.md`: executable tasks and acceptance criteria.

Every feature PR must update all impacted docs in the same PR.

## 9. Release Acceptance (v0)
- API usable for text and file workflows.
- Supported function families documented and test-proven.
- No known P0/P1 correctness or crash issues open.
- CI and `./dev/check.sh` green.
