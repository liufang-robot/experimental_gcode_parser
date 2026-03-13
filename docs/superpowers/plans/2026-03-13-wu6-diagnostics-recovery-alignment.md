# WU-6 Diagnostics/Recovery Alignment Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a recoverable execution-session API for rejected-line halt-fix-continue behavior without breaking the existing streaming execution semantics.

**Architecture:** Keep `StreamingExecutionEngine` as the execution core and add a new public `ExecutionSession` that owns editable suffix state, rejected-boundary recovery, and engine rebuilds after suffix edits. Rejected-line failures become a distinct recoverable state rather than an unrecoverable fault in the session workflow.

**Tech Stack:** C++17, CMake, GoogleTest, existing `StreamingExecutionEngine` / `AilExecutor`, mdBook/docs, `./dev/check.sh`

---

## Scope

In scope:

- new public `ExecutionSession` API
- recoverable `Rejected` state for execution/session flow
- suffix-based recovery editing from the rejected line onward
- consistent rejected-line vs fault behavior in execution-oriented APIs
- tests for halt-fix-continue behavior
- docs/spec/changelog updates

Out of scope:

- arbitrary earlier-line editing in execution recovery
- parser-internal incremental parse reuse
- removing `StreamingExecutionEngine`
- removing `ParseSession`
- final public API cleanup/deprecation work

## File Structure

### Files to create

- `include/gcode/execution_session.h`
  - public `ExecutionSession` API
- `src/execution_session.cpp`
  - session implementation
- `test/execution_session_tests.cpp`
  - primary halt-fix-continue coverage

### Files to modify

- `include/gcode/execution_commands.h`
  - add `Rejected` engine/session state and the rejected step payload
- `include/gcode/streaming_execution_engine.h`
  - if needed, expose a non-faulting rejected result path used by the session
- `src/streaming_execution_engine.cpp`
  - stop collapsing rejected lines into unconditional faults for the reusable
    engine path
- `test/streaming_execution_tests.cpp`
  - preserve and tighten direct engine behavior expectations
- `test/public_headers_tests.cpp`
  - compile the new public header
- `SPEC.md`
- `docs/src/development/design/implementation_plan_from_requirements.md`
- `CHANGELOG_AGENT.md`

### Files to read before implementation

- `docs/superpowers/specs/2026-03-13-wu6-execution-session-recovery-design.md`
- `docs/src/development/design/incremental_session.md`
- `include/gcode/session.h`
- `src/session.cpp`

## Chunk 1: Define public rejected-state data

### Task 1: Add the recoverable rejected-state contract

**Files:**
- Modify: `include/gcode/execution_commands.h`
- Test: `test/public_headers_tests.cpp`

- [ ] **Step 1: Write the failing public-header compile test**

Add a compile-time usage check for:
- `EngineState::Rejected`
- `StepStatus::Rejected`
- rejected payload fields exposed through `StepResult`

- [ ] **Step 2: Run the public-header test and verify it fails**

Run:
```bash
cmake --build build -j --target public_headers_tests
./build/public_headers_tests
```

Expected:
- fail because the rejected state/payload is not present yet

- [ ] **Step 3: Add the minimal public data model**

Add to `include/gcode/execution_commands.h`:
- `EngineState::Rejected`
- `StepStatus::Rejected`
- `RejectedState` (line/source/reasons)
- `StepResult.rejected`

Keep the model focused and additive.

- [ ] **Step 4: Re-run the public-header test**

Run the same commands from Step 2.

Expected:
- PASS

- [ ] **Step 5: Commit**

```bash
git add include/gcode/execution_commands.h test/public_headers_tests.cpp
git commit -m "feat: add recoverable rejected state contract"
```

## Chunk 2: Make streaming execution surface rejection distinctly

### Task 2: Stop collapsing rejected lines into unconditional faults

**Files:**
- Modify: `src/streaming_execution_engine.cpp`
- Modify: `include/gcode/streaming_execution_engine.h` (if needed)
- Test: `test/streaming_execution_tests.cpp`
- Test: `test/streaming_execution_gmock_tests.cpp`

- [ ] **Step 1: Write the failing engine tests**

Add or update tests proving:
- invalid line emits diagnostic and `rejected_line`
- engine/session path returns `Rejected`, not `Faulted`
- runtime failure paths still return `Faulted`

- [ ] **Step 2: Run the focused tests and verify the current failure**

Run:
```bash
cmake --build build -j --target streaming_execution_tests streaming_execution_gmock_tests
./build/streaming_execution_tests
./build/streaming_execution_gmock_tests --gtest_filter='StreamingExecutionGmockTest.InvalidLineEmitsDiagnosticAndRejectedLine'
```

Expected:
- at least one failure because rejected lines still fault the engine

- [ ] **Step 3: Implement the distinct rejected result**

Update `src/streaming_execution_engine.cpp` so rejected lines:
- emit diagnostics
- emit `rejected_line`
- return `Rejected` with source/reasons
- do not reuse the fault path for ordinary line rejection

Keep unrecoverable runtime/executor faults as `Faulted`.

- [ ] **Step 4: Re-run the focused tests**

Run the same commands from Step 2.

Expected:
- PASS

- [ ] **Step 5: Commit**

```bash
git add include/gcode/streaming_execution_engine.h src/streaming_execution_engine.cpp test/streaming_execution_tests.cpp test/streaming_execution_gmock_tests.cpp
git commit -m "refactor: distinguish rejected lines from faults in streaming execution"
```

## Chunk 3: Add the new execution session

### Task 3: Implement `ExecutionSession` with suffix replacement recovery

**Files:**
- Create: `include/gcode/execution_session.h`
- Create: `src/execution_session.cpp`
- Test: `test/execution_session_tests.cpp`

- [ ] **Step 1: Write the failing execution-session tests**

Cover:
- rejected line enters `Rejected`
- `replaceEditableSuffix(...)` updates the rejected suffix
- subsequent `pump()` continues from the stored rejected boundary
- unchanged prefix remains stable

- [ ] **Step 2: Run the focused test and verify it fails**

Run:
```bash
cmake --build build -j --target execution_session_tests
./build/execution_session_tests
```

Expected:
- fail because `ExecutionSession` does not exist yet

- [ ] **Step 3: Implement minimal `ExecutionSession`**

Implementation rules:
- own the editable text lines
- own an internal `StreamingExecutionEngine`
- remember the rejected boundary and rejected reasons
- allow `replaceEditableSuffix(...)` only in `Rejected` state
- rebuild execution from the stored rejected boundary on next `pump()`

Do not add earlier-line edit support in this slice.

- [ ] **Step 4: Re-run the focused execution-session tests**

Run the same commands from Step 2.

Expected:
- PASS

- [ ] **Step 5: Commit**

```bash
git add include/gcode/execution_session.h src/execution_session.cpp test/execution_session_tests.cpp
git commit -m "feat: add execution session recovery API"
```

## Chunk 4: Verify interaction with runtime blocking and completion

### Task 4: Lock the state-machine boundaries

**Files:**
- Modify: `test/execution_session_tests.cpp`
- Modify: `test/streaming_execution_tests.cpp`

- [ ] **Step 1: Add boundary tests**

Add tests proving:
- `resume(...)` is valid only for `Blocked`
- `replaceEditableSuffix(...)` is valid only for `Rejected`
- `cancel()` still works from `Rejected`
- direct `StreamingExecutionEngine` behavior remains deterministic

- [ ] **Step 2: Run the focused boundary tests**

Run:
```bash
./build/execution_session_tests
./build/streaming_execution_tests
```

Expected:
- PASS after implementation

- [ ] **Step 3: Commit**

```bash
git add test/execution_session_tests.cpp test/streaming_execution_tests.cpp
git commit -m "test: lock execution session recovery state boundaries"
```

## Chunk 5: Docs, plan status, and final gate

### Task 5: Update documentation and run the full gate

**Files:**
- Modify: `SPEC.md`
- Modify: `docs/src/development/design/implementation_plan_from_requirements.md`
- Modify: `CHANGELOG_AGENT.md`

- [ ] **Step 1: Update docs**

Document:
- `ExecutionSession`
- recoverable `Rejected` state
- suffix-based recovery editing
- distinction between `Rejected` and `Faulted`

Update work-unit status:
- `WU-6`: completed
- `WU-7`: next

- [ ] **Step 2: Run focused tests once more**

Run:
```bash
./build/execution_session_tests
./build/streaming_execution_tests
./build/streaming_execution_gmock_tests
```

Expected:
- PASS

- [ ] **Step 3: Run the full gate**

Run:
```bash
./dev/check.sh
```

Expected:
- PASS

- [ ] **Step 4: Commit**

```bash
git add SPEC.md docs/src/development/design/implementation_plan_from_requirements.md CHANGELOG_AGENT.md
git commit -m "docs: record execution session recovery flow"
```

## Definition Of Done

`WU-6` is complete when:

- rejected-line recovery is no longer modeled as an unconditional execution
  fault
- a public `ExecutionSession` supports halt-fix-continue from the rejected
  suffix
- direct runtime blocking/fault behavior remains intact
- docs/spec/changelog are updated
- `./dev/check.sh` is green
