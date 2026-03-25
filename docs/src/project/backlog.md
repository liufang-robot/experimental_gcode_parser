# BACKLOG

## How To Use
- Agent picks by priority and dependency order.
- For the current architecture-planning queue (`T-037`..`T-047`), follow
  [Repository Implementation Plan](../development/design/implementation_plan_root.md)
  Section 4 execution order.
- Each task must include acceptance criteria before work starts.
- Completed tasks are moved to a done section in this file or linked from PR.

## Priority Legend
- `P0`: must fix now (blocker, broken CI, crash/data-loss/security risk)
- `P1`: high-value near-term
- `P2`: important but can wait
- `P3`: optional/enhancement

## Ready Queue
- Architecture planning queue `T-037`..`T-047` is completed and moved to
  `Done`.
- Next ready work should be selected from the remaining non-completed backlog
  entries by priority and dependency order.
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
  - runtime execution-path wiring for `tool_select`/`tool_change` as executable
    control instructions (non-motion path; no standalone motion packets)
- `Acceptance Criteria`:
  - executor behavior is deterministic across direct/deferred configurations
  - diagnostics/policy behavior is explicit for `M6` without pending tool
  - execution boundary is documented and test-covered:
    - instruction exists in AIL
    - runtime path executes it
    - packet stage remains motion-focused
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
- T-037 (merged to `main`; see `CHANGELOG_AGENT.md` slices on 2026-03-03)
- T-038 (merged to `main`; see `CHANGELOG_AGENT.md` entry on 2026-03-04)
- T-039 (merged to `main`; PR #131)
- T-040 (merged to `main`; PR #132)
- T-041 (merged to `main`; PR #130)
- T-042 (merged to `main`; see `CHANGELOG_AGENT.md` entry on 2026-03-04)
- T-043 (merged to `main`; see `CHANGELOG_AGENT.md` entry on 2026-03-04)
- T-044 (merged to `main`; see `CHANGELOG_AGENT.md` entry on 2026-03-04)
- T-045 (merged to `main`; see `CHANGELOG_AGENT.md` entry on 2026-03-04)
- T-046 (merged to `main`; see `CHANGELOG_AGENT.md` entry on 2026-03-04)
- T-047 (merged to `main`; see `CHANGELOG_AGENT.md` entry on 2026-03-04)
- T-050 (merged to `main`; see PR series #107-#129)
- T-051 (local, unmerged)
- T-052 (local, unmerged)
- T-053 (local, unmerged)
- T-054 (local, unmerged)
