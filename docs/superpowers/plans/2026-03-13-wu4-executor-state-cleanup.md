# WU-4 Executor State Cleanup Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make executor modal/runtime state easier to reason about by centralizing state seeding, snapshot construction, and state-transition handling without changing behavior.

**Architecture:** Keep `AilExecutor` as the single execution core, but reduce duplicated state shapes and duplicated state-transition code around blocking, initial-state seeding, and command-facing modal snapshots. The slice is internal-only: no new execution behavior and no public command-schema changes.

**Tech Stack:** C++17, CMake, GoogleTest, mdBook docs, existing `./dev/check.sh` gate

---

## Scope

In scope:
- executor-internal state cleanup
- reducing duplicate state-transition logic in `AilExecutor`
- reducing duplicate modal-state construction between executor and helper code
- updating docs/plan status

Out of scope:
- new modal groups
- tool-behavior changes
- diagnostics-policy behavior changes
- public API redesign

## File Structure

### Files to modify

- `include/gcode/ail.h`
  - tighten executor/internal state declarations only if needed for cleanup
- `src/ail.cpp`
  - main executor-state cleanup work
- `src/execution_modal_state.h`
  - central helper declarations for executor-facing modal snapshot/state creation
- `src/execution_modal_state.cpp`
  - central helper implementations
- `test/ail_executor_tests.cpp`
  - state-transition and seeding regression coverage
- `test/streaming_execution_tests.cpp`
  - verify streaming still inherits the unchanged executor behavior where relevant
- `docs/src/development/design/implementation_plan_from_requirements.md`
  - mark `WU-4` in progress/done when appropriate
- `CHANGELOG_AGENT.md`
  - required slice entry

### Files to read before implementation

- `include/gcode/execution_commands.h`
- `src/execution_instruction_dispatcher.cpp`
- `src/streaming_execution_engine.cpp`

## Chunk 1: Define the cleanup target

### Task 1: Document the current executor-state duplication

**Files:**
- Modify: `docs/src/development/design/implementation_plan_from_requirements.md`

- [ ] **Step 1: Write down the concrete duplication points**

Add a short note under `WU-4` describing:
- duplicated state seeding in `AilExecutor` constructor
- duplicated blocked-state resume logic across the two `step(...)` overloads
- duplicated modal snapshot construction split between executor fields and helper calls

- [ ] **Step 2: Verify the note is accurate against code**

Run:
```bash
rg -n "initial_state|blocked->|state_\\.status = ExecutorStatus::Ready|makeExecutionModalState" src/ail.cpp src/execution_modal_state.cpp
```

Expected:
- matches for constructor seeding
- matches for both blocked-state wake paths
- match for modal snapshot construction

- [ ] **Step 3: Commit**

```bash
git add docs/src/development/design/implementation_plan_from_requirements.md
git commit -m "docs: clarify WU-4 executor state cleanup target"
```

## Chunk 2: Centralize executor-state helpers

### Task 2: Add helper(s) for executor initial state and command-facing modal snapshot

**Files:**
- Modify: `src/execution_modal_state.h`
- Modify: `src/execution_modal_state.cpp`
- Test: `test/ail_executor_tests.cpp`

- [ ] **Step 1: Write a failing test for state seeding/snapshot equivalence**

Add a focused test in `test/ail_executor_tests.cpp` that:
- seeds `AilExecutorOptions.initial_state`
- executes one motion-capable instruction with sink/runtime
- asserts the emitted command reflects the seeded state via `effective`

If a similar test already exists, add a narrower one for the specific helper boundary instead of duplicating broad coverage.

- [ ] **Step 2: Run the targeted test and capture the current behavior**

Run:
```bash
cmake --build build -j --target ail_executor_tests
./build/ail_executor_tests --gtest_filter='AilExecutorTest.InitialModalStateAffectsDispatchedMotionCommands'
```

Expected:
- PASS before refactor, establishing behavior lock

- [ ] **Step 3: Add helper functions**

Implement helpers in `src/execution_modal_state.*` for:
- building executor state from `AilExecutorInitialState`
- building `ExecutionModalState` from current executor state

Design rule:
- no behavior change
- executor remains owner of canonical state

- [ ] **Step 4: Re-run the targeted test**

Run:
```bash
./build/ail_executor_tests --gtest_filter='AilExecutorTest.InitialModalStateAffectsDispatchedMotionCommands'
```

Expected:
- PASS

- [ ] **Step 5: Commit**

```bash
git add src/execution_modal_state.h src/execution_modal_state.cpp test/ail_executor_tests.cpp
git commit -m "refactor: centralize executor modal state helpers"
```

## Chunk 3: Remove duplicated blocked-state wake logic

### Task 3: Factor the blocked-to-ready transition into one executor helper

**Files:**
- Modify: `src/ail.cpp`
- Test: `test/ail_executor_tests.cpp`

- [ ] **Step 1: Write a focused regression test for blocked resume paths**

Add or tighten tests covering both:
- wait-token unblock
- time-based unblock

These should assert:
- blocked executor returns no progress while still blocked
- once ready, state returns to `Ready`
- `pc` resumes from `blocked.instruction_index`

- [ ] **Step 2: Run the focused tests**

Run:
```bash
./build/ail_executor_tests --gtest_filter='AilExecutorTest.MotionStepCanBlockAndResumeOnRuntimeWaitToken:AilExecutorTest.BranchCanBlockAndResumeOnEvent'
```

Expected:
- PASS before refactor

- [ ] **Step 3: Implement one shared wake-from-blocked helper in `src/ail.cpp`**

Target:
- both `step(...)` overloads call the same helper
- helper decides whether a blocked executor is now runnable
- helper owns:
  - pending-event lookup
  - retry-time check
  - transition to `Ready`
  - `pc` restore
  - blocked-state reset

- [ ] **Step 4: Re-run the focused tests**

Run:
```bash
./build/ail_executor_tests --gtest_filter='AilExecutorTest.MotionStepCanBlockAndResumeOnRuntimeWaitToken:AilExecutorTest.BranchCanBlockAndResumeOnEvent'
```

Expected:
- PASS

- [ ] **Step 5: Commit**

```bash
git add src/ail.cpp test/ail_executor_tests.cpp
git commit -m "refactor: deduplicate executor blocked-state wake logic"
```

## Chunk 4: Use canonical helper-based state in dispatch path

### Task 4: Make motion/dwell dispatch consume the centralized executor snapshot path

**Files:**
- Modify: `src/ail.cpp`
- Test: `test/ail_executor_tests.cpp`
- Test: `test/streaming_execution_tests.cpp`

- [ ] **Step 1: Write or tighten a dispatch-state regression**

Use existing tests or add one that proves:
- motion code updates correctly after dispatch
- working plane / rapid mode / tool comp / tool selection still propagate into emitted command `effective`

- [ ] **Step 2: Run the focused dispatch tests**

Run:
```bash
./build/ail_executor_tests --gtest_filter='AilExecutorTest.MotionStepWithSinkAndRuntimeDispatchesLinearMove:AilExecutorTest.InitialModalStateAffectsDispatchedMotionCommands:AilExecutorTest.DeferredToolModeUsesPendingSelectionUntilM6'
./build/streaming_execution_tests --gtest_filter='StreamingExecutionTest.FunctionExecutionRuntimeCanDriveStreamingEngine'
```

Expected:
- PASS before refactor

- [ ] **Step 3: Replace ad hoc state-to-snapshot code in `src/ail.cpp`**

Target:
- use the new helper for command-facing snapshot construction
- keep post-dispatch motion-code update behavior unchanged

- [ ] **Step 4: Re-run the focused dispatch tests**

Run the same commands from Step 2.

Expected:
- PASS

- [ ] **Step 5: Commit**

```bash
git add src/ail.cpp test/ail_executor_tests.cpp test/streaming_execution_tests.cpp
git commit -m "refactor: route executor dispatch through canonical modal snapshot helper"
```

## Chunk 5: Finish docs and full validation

### Task 5: Update docs and run the full gate

**Files:**
- Modify: `docs/src/development/design/implementation_plan_from_requirements.md`
- Modify: `CHANGELOG_AGENT.md`

- [ ] **Step 1: Mark `WU-4` status**

Update `implementation_plan_from_requirements.md`:
- `WU-4`: completed
- `WU-5`: next

- [ ] **Step 2: Add `CHANGELOG_AGENT.md` entry**

Include:
- what changed
- affected tests
- known limitations
- local repro commands

- [ ] **Step 3: Run focused build/tests**

Run:
```bash
cmake --build build -j --target ail_executor_tests streaming_execution_tests
./build/ail_executor_tests
./build/streaming_execution_tests
```

Expected:
- PASS

- [ ] **Step 4: Run the final gate**

Run:
```bash
./dev/check.sh
```

Expected:
- PASS

- [ ] **Step 5: Commit**

```bash
git add docs/src/development/design/implementation_plan_from_requirements.md CHANGELOG_AGENT.md
git commit -m "docs: record WU-4 executor state cleanup"
```

## Review Notes

- This work unit is intentionally internal-only.
- If the cleanup starts pushing public API changes, stop and split that work into `WU-7`.
- If tool-state cleanup starts changing `M6` semantics, stop and move that part to `WU-5`.

## Validation Checklist

- `AilExecutor` still owns canonical execution state
- emitted command payloads are unchanged
- no streaming behavior regressions
- no public header changes unless explicitly justified
- `./dev/check.sh` green

## Handoff

Plan complete and saved to `docs/superpowers/plans/2026-03-13-wu4-executor-state-cleanup.md`.
