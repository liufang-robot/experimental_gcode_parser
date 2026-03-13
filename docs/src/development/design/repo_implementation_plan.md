# Repository Implementation Plan

This page mirrors the repo-root `IMPLEMENTATION_PLAN.md` so it is readable in
the published mdBook HTML. Keep this page in sync with the root document.

Status: summary/reference only.

The authoritative current plan is
[`implementation_plan_from_requirements.md`](implementation_plan_from_requirements.md).

# IMPLEMENTATION PLAN — Siemens-Compatible Parser and Runtime

## 1. Goal
Turn PRD Section 5 requirements into incremental, testable code changes while
keeping `main` stable and each PR small.

Inputs:
- `PRD.md`
- root `ARCHITECTURE.md`
- `BACKLOG.md` (`T-037`..`T-047`)
- acceptance fixture:
  - `testdata/integration/simple_integrated_case.ngc`

## 2. Delivery Strategy
Principles:
- Build shared infrastructure first (modal/config/policy scaffolding).
- Then add one feature family per slice.
- Keep strict test-first and `./dev/check.sh` green at each step.
- PRs are created only; merge after explicit user approval.
- Model every machine-carried behavior as an explicit executable instruction;
  decide per-instruction execution transport (motion packet vs runtime control).

## 3. Phase Plan

### Phase A — Foundation (architecture scaffolding)
Target outcomes:
- explicit modal registry with group IDs
- machine profile/config objects
- policy interfaces (tool/M-code/runtime hooks)

Planned tasks:
- `T-041` (feed model architecture; supplies modal/group scaffolding)
- `T-042` (dimensions/units architecture integration)

Expected code touch points:
- `src/` new files:
  - `modal_registry.h/.cpp`
  - `modal_engine.h/.cpp`
  - `machine_profile.h/.cpp`
  - `runtime_policy.h/.cpp`
- existing:
  - `src/ail.h`, `src/ail.cpp`
  - `src/ast.h` (state-carrying metadata expansions)

Tests:
- new `test/modal_engine_tests.cpp`
- new `test/machine_profile_tests.cpp`

### Phase B — Syntax and normalization extensions
Target outcomes:
- parser coverage for required syntax families and normalized statement schema.

Planned tasks:
- `T-037` comments (`(*...*)`, `;`, optional `//` compatibility mode)
- `T-046` M-code syntax + normalization baseline
- `T-045` rapid-traverse syntax/state (`G0`, `RTLION`, `RTLIOF`)

Expected code touch points:
- `grammar/GCode.g4`
- `src/gcode_parser.cpp`
- `src/semantic_rules.cpp`
- `src/semantic_normalizer.h/.cpp` (new)

Tests:
- `test/parser_tests.cpp` additions
- parser/CLI golden updates in `testdata/`

### Phase C — Modal behavior integration
Target outcomes:
- modal groups represented and validated centrally with deterministic conflict handling.

Planned tasks:
- `T-039` Group 7
- `T-040` working plane
- `T-044` Groups 10/11/12
- `T-043` Group 8 offsets/suppress contexts
- complete Group 14/13 and Group 15 behavior from `T-041/T-042`

Expected code touch points:
- `src/modal_engine.*`
- `src/ail.cpp`
- `src/messages*.cpp`
- `src/packet*.cpp`

Tests:
- `test/modal_engine_tests.cpp`
- `test/ail_tests.cpp`
- `test/ail_executor_tests.cpp`
- targeted new suites:
  - `test/work_offset_tests.cpp`
  - `test/feed_mode_tests.cpp`
  - `test/transition_mode_tests.cpp`

### Phase D — Tool/runtime behavior
Target outcomes:
- configurable Siemens-style tool behavior and runtime-executable semantics.

Planned tasks:
- `T-038` tool-change semantics with/without tool management
- `T-046` runtime-executable M-code subset mapping

Expected code touch points:
- `src/ail.cpp`, `src/ail.h`
- `src/runtime_policy.*`
- possible new `src/tool_policy.*`, `src/mcode_policy.*`

Tests:
- `test/tool_change_tests.cpp` (new)
- `test/mcode_tests.cpp` (new)
- executor integration tests

## 4. Backlog Dependency Order
Recommended execution order:
1. `T-041` feed model architecture (introduce modal/config scaffold)
2. `T-042` dimensions/units architecture (Group 13/14 integration)
3. `T-037` comments syntax completion
4. `T-046` M-code syntax/model baseline
5. `T-045` rapid traverse model
6. `T-039` Group 7 compensation state
7. `T-040` plane selection state
8. `T-044` exact-stop/continuous-path state
9. `T-043` work offsets and suppression contexts
10. `T-038` tool change + tool management policies
11. `T-047` incremental parse session API

Reasoning:
- Steps 1-2 create common state/config foundation.
- Steps 3-5 strengthen syntax/model inputs.
- Steps 6-9 implement grouped modal behavior.
- Step 10 applies the most machine-dependent runtime semantics last.

## 5. PR Slicing Policy
For each task:
1. one narrow behavior slice
2. tests first/with code
3. docs updated in same PR (`SPEC.md`, `CHANGELOG_AGENT.md`, optionally `PRD.md`)
4. evidence of `./dev/check.sh` pass

Suggested PR size:
- 200-500 LOC preferred
- avoid mixing unrelated modal groups in one PR

## 6. Validation Matrix (minimum per feature)
- Parser tests:
  - syntax acceptance/rejection
  - source-location diagnostics
- AIL tests:
  - normalized instruction shape
  - modal metadata correctness
- Executor tests:
  - runtime branch/state behavior where applicable
- CLI tests:
  - stable debug/json outputs if schemas change
- Integration fixture:
  - run parse + ail/lower on `testdata/integration/simple_integrated_case.ngc`
  - compare key invariants (no unintended diagnostics, stable line mapping,
    expected modal/motion/control items present)

## 7. Risk and Mitigation
Risk:
- Modal logic spread across parser/lowering/runtime causing regressions.
Mitigation:
- centralize modal transitions in modal engine; add unit tests before feature rollout.

Risk:
- machine-specific semantics hardcoded too early.
Mitigation:
- require `MachineProfile` + policy interfaces before advanced features.

Risk:
- golden fixture churn.
Mitigation:
- update goldens only in focused PRs and include change rationale.

## 8. Review Checklist (per PR)
- requirement trace to PRD subsection
- architecture alignment with root `ARCHITECTURE.md`
- tests added/updated and meaningful
- `./dev/check.sh` green
- no unintended schema break without doc updates
