# BACKLOG

## How To Use
- Agent picks by priority and dependency order.
- For the current architecture-planning queue (`T-037`..`T-047`), follow
  [IMPLEMENTATION_PLAN.md](/home/liufang/optcnc/gcode/IMPLEMENTATION_PLAN.md)
  Section 4 execution order.
- Each task must include acceptance criteria before work starts.
- Completed tasks are moved to a done section in this file or linked from PR.

## Priority Legend
- `P0`: must fix now (blocker, broken CI, crash/data-loss/security risk)
- `P1`: high-value near-term
- `P2`: important but can wait
- `P3`: optional/enhancement

## Ready Queue
Architecture planning queue for PRD Section 5 requirements:
- `ID`: T-047
- `Priority`: P1
- `Title`: Architecture/design for incremental parse session API
- `Why`: PRD requires editor-style resume from explicit line or first error line
  with deterministic prefix preservation and suffix reparse behavior.
- `Scope`:
  - define session API contract for line edit and resume controls
  - define prefix/suffix merge semantics for diagnostics/messages
  - define behavior for fail-fast errors and first-error-line discovery
  - define compatibility with file-backed and text-backed workflows
- `Acceptance Criteria`:
  - architecture note maps PRD Section 5.13 end-to-end
  - merge semantics are explicit and testable
  - implementation task breakdown prepared with tests/docs per slice
- `Out of Scope`:
  - full parser-internal incremental parse-tree reuse optimizations
  - IDE protocol implementation details
- `SPEC Sections`:
  - Section 6 (resume session API / incremental contract)
- `Tests To Add/Update`:
  - planning-only task; implementation tests split into follow-up tasks
- `ID`: T-046
- `Priority`: P1
- `Title`: Architecture/design for Siemens M-code model and execution boundaries
- `Why`: PRD now requires structured M-code support (`M##` and extended forms)
  with clear parse/lowering/runtime behavior.
- `Scope`:
  - define syntax model for `M##` and `M<ext>=<value>` forms
  - define value-range validation model (`INT`, `0..2147483647`)
  - define extended-address validity rules by M-function family
  - define M-code classification (parse-only vs executable semantics)
  - define baseline mappings for predefined Siemens M-functions
    (`M0/M1/M2/M3/M4/M5/M6/M17/M19/M30/M40..M45/M70`)
  - define block-level M-count policy model (machine/config dependent)
  - define trigger timing model for stop/end functions after traversing movement
  - define interaction points with tool/spindle/coolant/control-flow subsystems
  - define output schema expectations for M-code instructions/events
- `Acceptance Criteria`:
  - architecture note maps PRD Section 5.12 end-to-end
  - clear boundaries between parser, AIL lowering, and runtime handling
  - implementation task breakdown prepared with tests/docs per slice
- `Out of Scope`:
  - vendor PLC custom M-function implementation details
  - full machine I/O stack integration
- `SPEC Sections`:
  - future: M-code syntax + runtime behavior sections
- `Tests To Add/Update`:
  - planning-only task; implementation tests split into follow-up tasks
- `ID`: T-045
- `Priority`: P1
- `Title`: Architecture/design for rapid traverse model (G0 + RTLION/RTLIOF)
- `Why`: PRD requires Siemens-compatible rapid positioning semantics and
  interpolation-mode control with explicit override precedence.
- `Scope`:
  - define `G0` representation and lowering/runtime behavior for rapid moves
  - define rapid interpolation state model for `RTLION` / `RTLIOF`
  - define override conditions that force linear rapid behavior
  - define interactions with Group 10 modes and compensation/transformation states
  - define output schema expectations for rapid-mode state and effective behavior
- `Acceptance Criteria`:
  - architecture note maps PRD Section 5.11 end-to-end
  - precedence/override rules are explicit and testable
  - implementation task breakdown prepared with tests/docs per slice
- `Out of Scope`:
  - machine-specific accel/jerk tuning internals
  - full servo trajectory generation details beyond interface contracts
- `SPEC Sections`:
  - future: rapid-traverse syntax + runtime behavior sections
- `Tests To Add/Update`:
  - planning-only task; implementation tests split into follow-up tasks
- `ID`: T-044
- `Priority`: P1
- `Title`: Architecture/design for exact-stop and continuous-path modes (Groups 10/11/12)
- `Why`: PRD requires Siemens-compatible transition behavior for exact-stop,
  continuous-path smoothing, and exact-stop block-change criteria.
- `Scope`:
  - define state model for Group 10 (`G60`, `G64..G645`)
  - define block-scope non-modal behavior model for Group 11 (`G9`)
  - define Group 12 criterion model (`G601/G602/G603`) with applicability rules
  - define representation for `ADIS`/`ADISPOS` and `G641` coupling
  - define precedence/coupling with feed and other modal states
- `Acceptance Criteria`:
  - architecture note maps PRD Section 5.10 end-to-end
  - explicit precedence and scope rules (modal/non-modal/criterion dependency)
  - implementation task breakdown prepared with tests/docs per slice
- `Out of Scope`:
  - full servo/dynamics control-law implementation
  - machine-specific LookAhead internals beyond interface contracts
- `SPEC Sections`:
  - future: transition mode syntax + runtime behavior sections
- `Tests To Add/Update`:
  - planning-only task; implementation tests split into follow-up tasks
- `ID`: T-043
- `Priority`: P1
- `Title`: Architecture/design for work-offset model (Group 8 + G53/G153/SUPA)
- `Why`: PRD requires Siemens-compatible settable work-offset state and
  suppression semantics for multi-origin programs.
- `Scope`:
  - define modal Group 8 state model (`G500`, `G54..G57`, `G505..G599`)
  - define non-modal suppression context model (`G53`, `G153`, `SUPA`)
  - define configuration model for machine-specific offset availability/ranges
    (including 828D-style variants)
  - define interaction with coordinate/frame/dimension state pipeline
- `Acceptance Criteria`:
  - architecture note maps PRD Section 5.9 end-to-end
  - explicit distinction between modal selection and block-scope suppression
  - implementation task breakdown prepared with tests/docs per slice
- `Out of Scope`:
  - full controller frame-chain internals beyond interface contracts
  - HMI zero-offset table editing workflows
- `SPEC Sections`:
  - future: work-offset syntax + modal/suppress behavior sections
- `Tests To Add/Update`:
  - planning-only task; implementation tests split into follow-up tasks
- `ID`: T-042
- `Priority`: P1
- `Title`: Architecture/design for dimensions model (Group 14 + Group 13 + AC/IC + DIAM*)
- `Why`: PRD requires Siemens-compatible absolute/incremental, units, rotary
  absolute targeting, and turning diameter/radius programming semantics.
- `Scope`:
  - define modal Group 14 state model (`G90/G91`) and per-value overrides (`AC/IC`)
  - define modal Group 13 unit-state model (`G70/G71/G700/G710`) with
    geometry-only vs geometry+technological-length scope
  - define rotary absolute targeting model (`DC/ACP/ACN`)
  - define channel/axis-specific diameter-radius state model (DIAM*/DAC/DIC/RAC/RIC)
  - define machine-data/policy hooks for axis eligibility and turning semantics
- `Acceptance Criteria`:
  - architecture note maps PRD Section 5.8 end-to-end across pipeline stages
  - migration plan produced with small implementation slices and tests/docs
  - coupling points to feed (5.7) and plane/compensation (5.5/5.6) are explicit
- `Out of Scope`:
  - full machine-data database implementation
  - controller-specific cycle internals beyond interface contracts
- `SPEC Sections`:
  - future: dimensions/units/modal behavior sections
- `Tests To Add/Update`:
  - planning-only task; implementation tests split into follow-up tasks
- `ID`: T-041
- `Priority`: P1
- `Title`: Architecture/design for Siemens feedrate model (Modal Group 15)
- `Why`: PRD now requires Siemens-style feed semantics across path and
  synchronized axes, including mixed linear/rotary conversion and axis limits.
- `Scope`:
  - define modal Group 15 state model (`G93/G94/G95/G96/G97/G931/G961/G971/G942/G952/G962/G972/G973`) and `F` handling
  - define `FGROUP` representation and path vs synchronized axis semantics
  - define `FL` axis-limit model and its effect on coordinated timing
  - define `FGREF` representation for rotary effective-radius conversion
  - define output schema expectations for feed-related state in AIL/runtime
- `Acceptance Criteria`:
  - architecture note maps PRD Section 5.7 behaviors end-to-end
  - migration plan from current feed model to Siemens-compatible model exists
  - implementation task breakdown prepared with tests/docs per slice
- `Out of Scope`:
  - full controller kinematic solver implementation
  - PLC/machine-data integration beyond modeled interfaces
- `SPEC Sections`:
  - future: supported syntax + modal/feed runtime behavior sections
- `Tests To Add/Update`:
  - planning-only task; implementation tests split into follow-up tasks
- `ID`: T-040
- `Priority`: P1
- `Title`: Architecture/design for working-plane modal semantics (G17/G18/G19)
- `Why`: PRD requires Siemens-compatible working-plane behavior and coupling to
  arc interpolation and compensation semantics.
- `Scope`:
  - define modal-state model for plane selection (`G17/G18/G19`)
  - define plane-state propagation through parse/lowering/runtime outputs
  - define interaction points:
    - arc interpolation plane (`G2/G3`)
    - tool radius compensation plane (Group 7)
    - tool length compensation infeed direction
  - define migration plan from current axis/arc model
- `Acceptance Criteria`:
  - architecture doc maps plane semantics end-to-end
  - implementation task breakdown prepared with test matrix
  - SPEC update plan identified for syntax + runtime behavior sections
- `Out of Scope`:
  - full geometric cutter compensation algorithm
  - machine kinematics beyond standard geometry-axis interpretation
- `SPEC Sections`:
  - future: supported syntax + modal behavior + lowering/runtime sections
- `Tests To Add/Update`:
  - planning-only task; implementation tests split into follow-up tasks
- `ID`: T-039
- `Priority`: P1
- `Title`: Architecture/design for Siemens modal Group 7 (G40/G41/G42)
- `Why`: PRD requires explicit modal-group representation for tool radius
  compensation with Siemens Group 7 semantics.
- `Scope`:
  - define data model updates for modal groups to include Group 7
  - define parse/lowering/runtime behavior for `G40/G41/G42`
  - define modal transition rules and interactions with current motion state
  - define output schema expectations (AIL/message/packet modal metadata)
- `Acceptance Criteria`:
  - architecture note maps Group 7 semantics end-to-end across pipeline stages
  - implementation task breakdown is produced (small PR slices with tests)
  - SPEC update plan identified for behavior contract once implemented
- `Out of Scope`:
  - full cutter-compensation geometry/path correction algorithm
  - controller-specific contour-entry/exit heuristics
- `SPEC Sections`:
  - future: modal metadata and supported G-code sections
- `Tests To Add/Update`:
  - planning-only task; implementation tests split into follow-up tasks
- `ID`: T-038
- `Priority`: P1
- `Title`: PRD architecture slice for Siemens tool-change semantics
- `Why`: PRD now requires configurable Siemens tool-change behavior
  (direct `T` change vs deferred `T`+`M6`, with/without tool management,
  location/name selectors, and substitution policy hooks).
- `Scope`:
  - define architecture for tool command representation in AST/AIL/runtime
  - define configuration model for machine mode:
    - direct-change vs deferred-change
    - tool-management on/off
  - define policy interfaces for tool-management resolution/substitution
  - produce staged implementation plan and test matrix
- `Acceptance Criteria`:
  - architecture proposal maps each PRD behavior in Section 5.4
  - clear boundaries between parser, lowering, executor, and policy layer
  - explicit migration plan from current architecture with small PR slices
  - backlog tasks generated from design with test/doc requirements
- `Out of Scope`:
  - full implementation of tool database/life management
  - machine-specific PLC integration
- `SPEC Sections`:
  - future update required after architecture approval
- `Tests To Add/Update`:
  - planning-only task; implementation tests split into follow-up tasks
- `ID`: T-037
- `Priority`: P1
- `Title`: Siemens-compatible comment syntax (`;`, `(* ... *)`, optional `//`)
- `Why`: Incoming PRD requires Siemens-style comment forms including
  multi-line block comments for real-world NC source compatibility.
- `Scope`:
  - support semicolon end-of-line comments after code (retain current behavior)
  - add block-comment tokenization/parsing for `(* ... *)` across lines
  - define optional compatibility mode for `//` single-line comments
  - preserve comment text in parse outputs (debug/json) with stable locations
  - add actionable diagnostics for unclosed block comment
- `Acceptance Criteria`:
  - parser accepts `;` and `(* ... *)` comments per PRD requirement
  - multi-line block comments are represented in AST/comment output
  - unclosed `(*` comment reports clear syntax diagnostic with location
  - existing golden tests remain stable unless intentionally updated
  - `./dev/check.sh` passes with new tests
- `Out of Scope`:
  - semantic meaning/execution of comment directives
  - IDE-specific comment metadata formats
- `SPEC Sections`:
  - Section 2.1 (Input comments)
  - Section 3.x syntax examples (as needed)
- `Tests To Add/Update`:
  - `test/parser_tests.cpp` (comment parsing + diagnostics)
  - parser/CLI golden fixtures affected by comment syntax updates
- `ID`: T-050
- `Priority`: P1
- `Title`: Siemens subprogram model (MPF/SPF, direct call, P-repeat, M17/RET, ISO M98)
- `Why`: Subprogram organization and call semantics are required for practical
  Siemens compatibility and reusable NC structure.
- `Scope`:
  - parser/AST support for:
    - direct call by subprogram name
    - quoted-name call form
    - repeat count `P`
    - `P=<count> <name>` and `<name> P<count>` forms
    - `M17` and `RET`
    - ISO-gated `M98 P...` compatibility syntax
  - (planned extension) parse model for `PROC` signatures and call arguments
  - AIL/runtime execution model for call stack, return, and unresolved-target
    diagnostics/policies
  - search-path policy model (same-folder bare name vs qualified/full path)
- `Acceptance Criteria`:
  - SPEC documents syntax + parse/runtime responsibilities
  - parser tests cover call/return syntax and ISO-mode gating
  - executor tests cover call stack return flow and unresolved call behavior
- `Out of Scope`:
  - full program-manager filesystem replication
  - controller-specific PLC integration details
- `SPEC Sections`:
  - Section 3.9
  - Section 6.1 (runtime control-flow execution notes)
- `Tests To Add/Update`:
  - `test/parser_tests.cpp`
  - `test/ail_tests.cpp`
  - `test/ail_executor_tests.cpp`

- `ID`: T-049
- `Priority`: P1
- `Title`: Siemens Chapter-2 syntax baseline (naming/blocks/assignment/comments/skip levels)
- `Why`: We need explicit Siemens fundamental grammar coverage for NC program
  structure and skip behavior before higher-level runtime features.
- `Scope`:
  - add parser support/validation for:
    - Siemens-compatible program-name forms (including `%...` compatibility mode)
    - block length limit diagnostics (Siemens baseline 512 chars)
    - assignment-shape `=` rules and numeric-extension disambiguation
    - skip-level markers `/0.. /9` metadata capture
  - define runtime boundary for skip-level execution policy
- `Acceptance Criteria`:
  - SPEC sections updated with concrete syntax/diagnostics behavior
  - parser tests cover valid/invalid chapter-2 baseline examples
  - runtime tests cover skip-level on/off behavior once execution hook is added
- `Out of Scope`:
  - full subprogram file-management implementation
  - HMI-specific UI configuration behavior for skip levels
- `SPEC Sections`:
  - Section 3.1, Section 3.6, Section 3.8, Section 5, Section 6.1
- `Tests To Add/Update`:
  - `test/parser_tests.cpp`
  - `test/ail_tests.cpp`
  - `test/ail_executor_tests.cpp`

- `ID`: T-048
- `Priority`: P1
- `Title`: System variables and user-defined variables model (Siemens-compatible)
- `Why`: PRD now requires explicit handling for both user-defined variables
  (`R...`) and Siemens-style system variables (`$...`) with clear parse/runtime
  responsibility boundaries.
- `Scope`:
  - extend variable reference model to support structured system-variable forms
    (including selector syntax like `[1,X,TR]`)
  - define parser diagnostics for malformed variable selectors
  - define runtime resolver contract for variable read/eval outcomes
    (`value|pending|error`) in expressions/conditions
  - define policy hooks for access/write restrictions and timing classes
    (preprocessing vs main-run variables)
- `Acceptance Criteria`:
  - SPEC documents syntax and boundary behavior for variable evaluation
  - parser/AIL tests cover user-defined + system-variable references
  - executor tests cover resolver `pending/error` behavior with variable-backed
    conditions
- `Out of Scope`:
  - full Siemens parameter-manual variable catalog implementation
  - PLC-side variable transport internals
- `SPEC Sections`:
  - Section 3.6 (assignment expressions / variable references)
  - Section 6.1 (runtime resolver behavior)
- `Tests To Add/Update`:
  - `test/parser_tests.cpp`
  - `test/ail_tests.cpp`
  - `test/ail_executor_tests.cpp`

- `ID`: T-051
- `Priority`: P1
- `Title`: Tool selector normalization for Siemens `T...` forms
- `Why`: `T-038` requires one normalized selector model across number/location/
  name forms with mode-aware validation.
- `Scope`:
  - parse/AST normalization for:
    - management-off forms: `T<number>`, `T=<number>`, `T<n>=<number>`
    - management-on forms: `T=<location|name>`, `T<n>=<location|name>`
  - mode-aware diagnostics for invalid selector forms
- `Acceptance Criteria`:
  - parser emits normalized selector representation with stable locations
  - invalid-by-mode forms produce clear actionable diagnostics
- `Out of Scope`:
  - runtime activation semantics (`M6` / pending state)
- `SPEC Sections`:
  - future: tool command syntax + selector rules
- `Tests To Add/Update`:
  - `test/parser_tests.cpp`
  - golden fixtures affected by selector output shape

- `ID`: T-052
- `Priority`: P1
- `Title`: AIL instruction split for tool select/change
- `Why`: `T-038` requires explicit `tool_select` vs `tool_change` boundaries
  and timing metadata (`immediate` vs `deferred_until_m6`).
- `Scope`:
  - AIL schema + lowering updates for `tool_select` and `tool_change`
  - JSON/debug output updates for new instruction shapes
- `Acceptance Criteria`:
  - AIL outputs preserve timing metadata and source attribution
  - CLI JSON/debug outputs remain deterministic
- `Out of Scope`:
  - policy-based resolver/substitution behavior
- `SPEC Sections`:
  - future: AIL/tool instruction schema
- `Tests To Add/Update`:
  - `test/ail_tests.cpp`
  - `test/cli_tests.cpp`

- `ID`: T-053
- `Priority`: P1
- `Title`: Executor pending-tool state and direct/deferred semantics
- `Why`: `T-038` requires runtime behavior for both direct `T` activation and
  deferred `T`+`M6` execution.
- `Scope`:
  - executor state for `active_tool` and `pending_tool_selection`
  - behavior rules for direct mode, deferred mode, and last-selection-wins
  - `M6` without pending selection policy hook
- `Acceptance Criteria`:
  - executor behavior is deterministic across direct/deferred configurations
  - diagnostics/policy behavior is explicit for `M6` without pending tool
- `Out of Scope`:
  - tool database substitution resolution internals
- `SPEC Sections`:
  - future: runtime tool-change behavior
- `Tests To Add/Update`:
  - `test/ail_executor_tests.cpp`
  - `test/ail_tests.cpp` (if metadata coupling changes)

- `ID`: T-054
- `Priority`: P1
- `Title`: Tool resolver/substitution policy integration
- `Why`: `T-038` requires machine-policy control for unresolved/ambiguous tool
  selectors and substitution behavior.
- `Scope`:
  - tool policy interface and default policy implementation
  - unresolved/ambiguous selector outcomes (`error|warning|fallback`)
  - substitution-enabled vs substitution-disabled behavior
- `Acceptance Criteria`:
  - policy outcomes are pluggable and test-covered
  - diagnostics clearly identify unresolved/ambiguous selectors
- `Out of Scope`:
  - full tool life/wear optimization algorithms
- `SPEC Sections`:
  - future: policy and diagnostics behavior for tool management
- `Tests To Add/Update`:
  - `test/machine_profile_tests.cpp`
  - `test/ail_executor_tests.cpp`

## Icebox
- Coverage threshold policy and badge.
- Multi-file include/subprogram parsing (future SPEC).

## Task Template
Use this template for new backlog items:

- `ID`: T-XXX
- `Priority`: P0/P1/P2/P3
- `Title`:
- `Why`:
- `Scope`:
- `Acceptance Criteria`:
- `Out of Scope`:
- `SPEC Sections`:
- `Tests To Add/Update`:

## In Progress
(List tasks currently being worked on; only one assignee/task per PR)
- (none)

## Done
(Move completed tasks here with PR link)
- T-010 (PR #17)
- T-009 (PR #16)
- T-008 (PR #15)
- T-013 (PR #14)
- T-012 (PR #13)
- T-007 (PR #3)
- T-006 (PR #3)
- T-005 (PR #12)
- T-004 (PR #11)
- T-003 (PR #10)
- T-002 (PR #9)
- T-001 (PR #8)
- T-011 (PR #6)
- T-014 (PR #20)
- T-015 (PR #21)
- T-016 (PR #22)
- T-017 (PR #23)
- T-018 (PR #25)
- T-019 (PR #26)
- T-020 (PR #27)
- T-021 (PR #28)
- T-022 (PR #31)
- T-023 (PR #33)
- T-024 (PR #35)
- T-025 (PR #36)
- T-026 (PR #37)
- T-029 (PR #38)
- T-028 (PR #39)
- T-030 (PR #40)
- T-027 (PR #42)
- T-031 (local, unmerged)
- T-032 (local, unmerged)
- T-033 (local, unmerged)
- T-034 (local, unmerged)
- T-035 (local, unmerged)
- T-036 (local, unmerged)
- T-044 (local, unmerged)
- T-046 (local, unmerged)
- T-045 (local, unmerged)
- T-047 (local, unmerged)
- T-043 (local, unmerged)
- T-042 (local, unmerged)
- T-038 (local, unmerged)
- T-051 (local, unmerged)
