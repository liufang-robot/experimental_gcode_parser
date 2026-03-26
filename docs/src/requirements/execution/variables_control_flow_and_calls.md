# Execution Requirements: Variables, Control Flow, and Calls

## 6. Variable and Expression Runtime Requirements

Status: `Reviewed` (2026-03-11 review pass)

Requirements to review:

- user-variable reads
- system-variable reads
- variable writes
- simple numeric expression evaluation
- richer expression evaluation
- pending/error behavior

Explicit boundary to review:

- plain `IRuntime` should cover only primitive and simple runtime cases
- richer or parser-limited expression semantics should use
  `IExecutionRuntime`

Reviewed decisions:

- user variables are executor-internal by default and may be evaluated directly
  by the executor
- system-variable reads and writes must go through the runtime interface
- runtime reads and writes may return `Pending`
- executor should directly evaluate the supported baseline expression subset
  rather than routing all evaluation through runtime
- richer or unsupported expression semantics must go through
  `IExecutionRuntime`
- pending read/evaluation/write behavior should move the engine to `Blocked`
  until `resume(...)`
- read/evaluation/write errors should fault execution with deterministic source
  attribution
- assignment execution should:
  - evaluate the right-hand side first
  - then perform the write
  - if the write is pending, block after submission and continue on
    `resume(...)`

Review note:

- the remaining follow-up is to enumerate the exact baseline
  executor-direct expression subset versus the richer `IExecutionRuntime`
  subset

## 7. Control-Flow Execution Requirements

Status: `Reviewed` (2026-03-11 review pass)

Families to review:

- label resolution
- `GOTO/GOTOF/GOTOB/GOTOC`
- branch execution
- structured `IF/ELSE/ENDIF`
- loops
- unresolved forward target behavior
- `WaitingForInput` vs fault-at-EOF behavior

Review questions:

- which control-flow families must be fully supported in streaming mode?
- what should remain deferred?

Reviewed decisions:

- all supported control-flow families should also be supported in streaming
  mode; the project should not intentionally maintain separate batch-only
  versus streaming-only control-flow semantics
- labels and control-flow targets must be preserved in buffered execution state
  so they can be resolved incrementally
- branch conditions are evaluated at execution time, not at parse time
- unresolved forward jump/call targets behave as:
  - before EOF: `WaitingForInput`
  - after `finish()`: `Faulted`
- backward or already-known targets should resolve immediately from buffered
  execution state

Review note:

- the remaining follow-up is to confirm detailed behavior family by family
  (`IF/ELSE/ENDIF`, loops, and other structured forms) without changing the
  reviewed rule that streaming mode should support the full accepted
  control-flow set

## 8. Subprogram and Call-Stack Requirements

Status: `Reviewed` (2026-03-11 review pass)

Requirements to review:

- call stack model
- target resolution
- repeat count behavior
- return behavior
- unresolved target policy
- interaction with streaming incremental input

Reviewed decisions:

- subprogram calls must use a real nested call stack
- each subprogram call pushes a return frame
- each return pops a return frame and resumes at the saved return point
- nested subprogram calls must be supported naturally through the call stack
- subprogram targets should be resolved from buffered execution state
- unresolved forward subprogram targets behave as:
  - before EOF: `WaitingForInput`
  - after `finish()`: `Faulted`
- repeat count means repeated subprogram invocation
- each repeat uses normal call/return behavior
- a return with an empty call stack is invalid and must fault
- subprogram/call-stack behavior should be supported in streaming mode under
  the same buffered execution model as other control-flow features
- subprogram target resolution and call-stack control are executor behavior,
  not plain `IRuntime` behavior

Review note:

- the remaining follow-up is mostly detailed call-frame layout and any
  controller-specific subprogram-mode differences, not the core behavioral
  contract
