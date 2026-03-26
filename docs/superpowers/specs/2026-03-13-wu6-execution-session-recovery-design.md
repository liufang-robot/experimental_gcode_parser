# WU-6 Execution Session Recovery Design

## Goal

Define the public recovery API for halt-fix-continue execution after a rejected
line, while keeping the existing streaming executor focused on buffered
execution.

## Problem

The reviewed requirements say:

- a syntax/semantic line failure should halt at that line
- enough buffered state should be preserved to continue after the line is
  corrected and reprocessed
- the user interface should allow edits from the rejected line onward, while
  keeping the earlier prefix locked

The current implementation does not satisfy that contract. Today
`StreamingExecutionEngine` emits a diagnostic and `rejected_line`, then moves
to `Faulted`. There is no API for editing the rejected suffix and continuing
execution.

## Chosen Design

Introduce a new public session type:

- `ExecutionSession`

Keep the current `StreamingExecutionEngine` as the execution core, but do not
make it own document-editing semantics.

Responsibilities split:

- `StreamingExecutionEngine`
  - chunk buffering
  - buffered execution
  - runtime blocking / waiting / cancellation
  - execution over accepted text
- `ExecutionSession`
  - editable text/suffix buffer
  - rejected-boundary tracking
  - recovery-oriented control flow
  - owning and rebuilding the streaming engine when retry is required

This preserves a clean separation:

- execution engine = runtime-oriented state machine
- execution session = editable document + recovery workflow

## Why This Design

### Rejected alternative: edit APIs on `StreamingExecutionEngine`

Pros:
- one object for everything

Cons:
- mixes runtime execution with document editing
- tangles input buffering, blocked waits, and text replacement
- makes the engine harder to reason about and test

### Rejected alternative: evolve `ParseSession`

Pros:
- reuse existing line-buffer/edit concept

Cons:
- `ParseSession` is batch/message-oriented today
- high risk of dragging batch assumptions into the execution path

### Selected approach: brand-new `ExecutionSession`

Pros:
- clear boundary
- matches the reviewed requirements
- keeps the recovery workflow explicit
- lets execution remain AIL/executor-driven

## Public API Direction

First-slice public surface:

```cpp
class ExecutionSession {
public:
  ExecutionSession(IExecutionSink &sink,
                   IRuntime &runtime,
                   ICancellation &cancellation,
                   const LowerOptions &options = {});
  ExecutionSession(IExecutionSink &sink,
                   IExecutionRuntime &runtime,
                   ICancellation &cancellation,
                   const LowerOptions &options = {});

  bool pushChunk(std::string_view chunk);
  StepResult pump();
  StepResult finish();
  StepResult resume(const WaitToken &token);
  void cancel();

  bool replaceEditableSuffix(std::string_view replacement_text);

  EngineState state() const;
  std::optional<int> rejectedLine() const;
};
```

Notes:

- `ExecutionSession` owns execution methods because it is an execution-facing
  abstraction, not just an editable document helper.
- `replaceEditableSuffix(...)` is the first-slice edit API.
- The editable region starts at the rejected line and includes all later text.
- The prefix before the rejected line is locked in normal recovery flow.

## Recovery State Model

Add a recoverable execution/session state:

- `Rejected`

Keep `Faulted` for unrecoverable execution failures.

Meaning:

- `Rejected`
  - syntax/semantic line rejection
  - recoverable by editing the suffix and retrying
- `Faulted`
  - unrecoverable runtime/execution failure
  - requires a new session or explicit higher-level reset

`Rejected` should carry:

- rejected physical line number
- rejected-line diagnostics/reasons
- stored retry boundary

## Edit Model

The user-approved edit model is:

- the rejected line and any later lines are editable
- lines before the rejected line are not editable in the common recovery path

For WU-6 first slice, the edit API should be suffix-oriented:

- `replaceEditableSuffix(text_from_rejected_line_onward)`

This avoids over-designing line insert/delete primitives in the first slice,
while still supporting all edits the user wants in the remaining suffix text.

If earlier-line edits are needed later, they should be modeled as an explicit
broader-boundary reparse/rebuild feature, not hidden behind the rejected-line
workflow.

## Retry Model

The session stores the rejected boundary internally.

That means the caller should not need to supply `retryFromLine(...)` in the
common case.

Normal recovery loop:

1. execution reaches a rejected line
2. session enters `Rejected`
3. caller replaces editable suffix
4. caller calls `pump()` again
5. session rebuilds execution from the stored rejected boundary and continues

Advanced explicit boundary APIs can be added later if needed, but are not
required for WU-6.

## Interaction With Runtime Blocking

`Rejected` is distinct from `Blocked`:

- `Blocked`
  - accepted action is pending in runtime
- `Rejected`
  - line text is invalid for execution

Recovery rules:

- `resume(...)` is only for `Blocked`
- `replaceEditableSuffix(...)` is only for `Rejected`
- `cancel()` remains valid in either state

## Engine Ownership

`ExecutionSession` should own the `StreamingExecutionEngine` internally.

Reason:

- the session owns the editable text and recovery boundary
- rebuilding the engine after suffix replacement is session behavior
- callers should not have to coordinate a separate engine object manually

## Compatibility

Keep the current public `StreamingExecutionEngine` API for direct execution
use-cases.

`ExecutionSession` is an additive public layer for editable recovery, not an
immediate replacement.

This keeps WU-6 bounded and avoids coupling recovery work to broader public API
removal decisions that belong to WU-7.

## Initial Test Expectations

The first implementation slice should prove:

- rejected line yields `Rejected`, not `Faulted`
- rejected suffix can be replaced
- execution continues from the stored rejected boundary
- prefix before the rejected line remains unchanged
- runtime-blocked behavior still works and stays distinct from rejection
- CLI/fake-log traces remain deterministic for the new recovery flow where
  applicable

## Out Of Scope For WU-6

- arbitrary earlier-line edits in the execution recovery API
- parser-internal incremental reuse optimizations
- removing `ParseSession`
- removing `StreamingExecutionEngine`
- final public API cleanup/deprecation work

## Traceability

- Requirements:
  - `docs/src/requirements/execution/index.md`
    - Diagnostics and Failure Policy
    - Streaming Execution Model
    - Test and Validation Requirements
- Authoritative plan:
  - `docs/src/development/design/implementation_plan_from_requirements.md`
