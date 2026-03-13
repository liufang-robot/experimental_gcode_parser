#Implementation Plan From Requirements

This note converts the reviewed requirements into a concrete architecture plan.
It is intentionally more specific than the older phase summary in
[`implementation_plan.md`](implementation_plan.md).

This is the authoritative implementation-planning document for the current
requirements-reviewed architecture.

The following documents are summary/reference companions only:

- [`implementation_plan.md`](implementation_plan.md)
- [`repo_implementation_plan.md`](repo_implementation_plan.md)

Use this document to answer:

- what the target architecture should be
- what the current code already does correctly
- what the main gaps are
- what order the remaining work should happen in

## Current Work-Unit Status

- `WU-1 Modal Snapshot Design`: completed and merged
- `WU-2 Command Schema Redesign`: completed and merged
- `WU-3 Runtime Dispatch Cleanup`: completed and merged
- `WU-4 Executor State Cleanup`: completed and merged
- `WU-5 Tool Execution Completion`: completed and merged
- `WU-6 Diagnostics/Recovery Alignment`: pending
- `WU-7 Final Public API Cleanup`: pending

## Goal

Move the project to one clear execution-oriented architecture that is
requirements-driven:

```text
chunk input -> buffered parse/lower -> buffered executor state ->
normalized execution commands -> runtime interfaces -> resume/cancel
```

The parser, semantic validation, execution/runtime behavior, and test policy
are already reviewed in the requirements documents. This plan maps those
decisions onto code and work units.

## Target Architecture

### 1. Syntax/Semantic Front End

Responsibilities:

- parse text into structured form
- preserve source information
- emit syntax/semantic diagnostics
- lower accepted text into AIL instructions

Key rule:

- syntax/semantic layers must not perform runtime execution decisions

### 2. Buffered Execution Core

Primary execution engine:

- `StreamingExecutionEngine`
- buffered incremental input
- `pushChunk(...)`, `pump()`, `finish()`, `resume(...)`, `cancel()`
- `Blocked` vs `WaitingForInput` kept distinct

Execution semantics:

- executor owns control flow, call stack, and incremental target resolution
- forward targets wait before EOF and fault after EOF
- execution-related state remains explicit

### 3. Execution IR

`AilInstruction` remains the executable IR.

Direction:

- parser/lowering produces AIL
- streaming engine buffers AIL
- executor advances through AIL
- normalized runtime-facing command data is emitted from execution

### 4. Runtime Boundary

Two-level split:

- `IRuntime`
  - primitive machine/service operations only
- `IExecutionRuntime`
  - richer language-level evaluation built on top of `IRuntime`

General action-command rule:

- action submission returns `Ready`, `Pending`, or `Error`
- `Pending` means accepted by runtime, not necessarily physically complete

### 5. Explicit Modal State

Requirements already fixed:

- every supported modal group is explicit in execution state
- all supported modal groups are preserved
- emitted commands carry effective modal values for all supported groups

This implies a full modal snapshot model, not a partial ad hoc set of fields.

## Current Architecture Assessment

### What Already Matches The Target

- public streaming execution API already exists in
  [`streaming_execution_engine.h`](/home/liufang/optcnc/gcode/include/gcode/streaming_execution_engine.h)
- buffered streaming state already exists:
  - pending lines
  - buffered instructions
  - executor reuse
  - `Blocked` / `WaitingForInput`
- `AilExecutor` already owns:
  - control-flow execution
  - call stack
  - unresolved-target deferral
  - runtime-aware stepping
- runtime split already exists:
  - [`execution_interfaces.h`](/home/liufang/optcnc/gcode/include/gcode/execution_interfaces.h)
  - [`execution_runtime.h`](/home/liufang/optcnc/gcode/include/gcode/execution_runtime.h)
- motion/dwell normalization and dispatch already exist
- reviewed fake-log/testing pattern already exists

### Main Architectural Gaps

#### 1. Command schema is still partial

Current `execution_commands.h` now carries a baseline explicit modal subset:

- motion code
- working plane
- rapid mode
- tool radius compensation
- active tool selection
- pending tool selection

That is a useful first step, but it still does not satisfy the reviewed
requirement that emitted commands carry all supported modal-group values.

#### 2. Modal state is modeled in multiple shapes

Today modal state is split across:

- `ModalState`
- `ExecutorState`
- `EffectiveModalSnapshot`
- AIL per-instruction effective fields

This is workable, but it is not yet one clean source of truth.

#### 3. Public execution command model is not finalized

We now have a normalized baseline for motion payloads:

- one `target` object for motion endpoints
- one `effective` snapshot for modal state
- no duplicated emitted `opcode` / `modal` fields on execution commands

But the model is still not final for:

- future tool/non-motion action payloads
- full effective modal snapshot

#### 4. Runtime boundary is correct in principle but not fully normalized

The split is right, but the exact inventory of:

- executor-direct evaluation
- `IExecutionRuntime`-only semantics
- command submission types

still needs one consolidated design pass.

#### 5. Tool execution is still policy-light

Tool execution requirements are reviewed, but current behavior still needs:

- explicit command schema
- explicit current/pending/selected tool snapshot model
- final `M6` no-pending-selection policy

#### 6. Diagnostics/recovery contract is reviewed but not yet formalized in API shape

The requirements now include halt-fix-continue behavior for line failures, but
the exact edit/reprocess session mechanics are not yet tied cleanly to the
streaming/executor API surface.

## Design Decisions To Apply Next

### A. Introduce a full effective modal snapshot type

Add one explicit execution snapshot type, for example:

- `EffectiveModalSnapshot`

It should become the single command-facing modal payload.

It should eventually include all supported groups, not just current motion ones.

### B. Reduce duplicated modal state shapes

Target:

- executor keeps canonical modal state internally
- normalized commands expose a copied `EffectiveModalSnapshot`
- ad hoc per-command modal subsets are retired where possible

### C. Treat AIL as the only execution IR

Do not add a second interpreter path.

Direction:

- syntax/semantic -> AIL
- buffered streaming engine -> AIL executor
- executor -> normalized command builder -> runtime interfaces

### D. Keep `IRuntime` primitive

Do not grow `IRuntime` into a language interpreter.

Keep these on `IExecutionRuntime`:

- richer expression evaluation
- richer condition evaluation
- parser-limited/context-heavy execution semantics

### E. Keep one streaming execution model

Do not intentionally split semantics into:

- batch-only execution behavior
- streaming-only execution behavior

The buffered streaming engine should be the primary execution path.

## Work Graph

```text
R1 Full modal snapshot design
  -> R2 Command schema redesign
  -> R3 Command builder/runtime dispatch cleanup

R1 Full modal snapshot design
  -> R4 Executor state cleanup

R2 Command schema redesign
  -> R5 Tool execution schema/policy completion

R3 Command builder/runtime dispatch cleanup
  -> R6 Diagnostics/recovery API alignment

R4 Executor state cleanup
  -> R6 Diagnostics/recovery API alignment

R5 Tool execution schema/policy completion
  -> R7 Final public execution API cleanup

R6 Diagnostics/recovery API alignment
  -> R7 Final public execution API cleanup
```

## Recommended Work Units

### WU-1 Modal Snapshot Design

Status: `Completed`

Deliver:

- one explicit full modal snapshot type
- mapping from executor state to command snapshot
- requirements/doc updates if field inventory changes

Tests:

- executor tests
- streaming execution tests
- CLI/event trace updates if payload changes

### WU-2 Command Schema Redesign

Status: `Completed`

Deliver:

- finalized command structs for:
  - motion
  - dwell
  - tool/action commands where needed
- normalized field layout for source + modal snapshot

Tests:

- executor tests
- CLI/event trace goldens
- public header tests

### WU-3 Runtime Dispatch Cleanup

Status: `Completed`

Deliver:

- consistent command building from executor state
- less duplication between executor, command builder, and dispatcher

Tests:

- executor runtime tests
- streaming tests
- fake-log trace tests

### WU-4 Executor State Cleanup

Status: `Completed`

Deliver:

- one clearer canonical modal/runtime state model inside executor
- reduced duplicate state between executor and command payload helpers

Tests:

- executor tests
- state-transition tests

### WU-5 Tool Execution Completion

Deliver:

- explicit tool command schema
- pending/current/selected tool state contract
- reviewed `M6`-without-pending-selection policy

Tests:

- executor tests
- streaming tests
- trace tests

### WU-6 Diagnostics/Recovery Alignment

Deliver:

- clear halt-fix-continue path for line failures
- consistent rejected-line vs fault behavior through streaming execution

Tests:

- streaming tests
- session/incremental tests
- trace tests

### WU-7 Final Public API Cleanup

Deliver:

- align public headers with final execution architecture
- deprecate or adapt legacy surfaces that conflict with the primary model

Tests:

- public header tests
- integration tests
- `./dev/check.sh`

## Remaining Implementation Order

Recommended order:

1. WU-6 Diagnostics/Recovery Alignment
2. WU-7 Final Public API Cleanup

## Definition Of Ready For Code Work

A work unit is ready only when:

- the relevant requirement sections are already reviewed
- current-vs-target gap is stated
- in-scope and out-of-scope are explicit
- test layers are declared
- fixtures/trace expectations are known

## Definition Of Done

For each work unit:

- code matches the reviewed requirements
- tests cover affected layers
- fake-log/trace coverage exists for execution changes
- public docs are updated if behavior/API changes
- `CHANGELOG_AGENT.md` is updated
- `./dev/check.sh` is green
