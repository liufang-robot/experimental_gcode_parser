# Execution Requirements: Runtime Boundaries and Action Commands

## 3. Runtime Interface Boundaries

Status: `Reviewed` (2026-03-11 partial boundary review)

Current intended split to review:

- `IRuntime`
  - primitive machine/runtime operations
  - read variable
  - read system variable
  - write variable
  - submit motion/dwell
  - cancel wait
- `IExecutionRuntime`
  - richer language-level execution semantics
  - condition evaluation
  - richer expression evaluation

Reviewed decisions:

- the current split is kept:
  - `IRuntime` is the primitive machine/service boundary
  - `IExecutionRuntime` is the richer language-aware execution boundary
- plain `IRuntime` must not own parsing, expression interpretation, condition
  interpretation, or control-flow semantics
- plain `IRuntime` must remain limited to primitive runtime actions such as:
  - variable reads/writes
  - system-variable reads
  - motion/dwell submission
  - wait cancellation

Examples of logic that must not be added to plain `IRuntime`:

- full condition evaluation such as `IF (R1 == 1) AND (R2 > 10)`
- full expression interpretation such as `(R1 + 2) * ABS(R3)`
- control-flow target resolution
- AST- or parser-shape-specific execution entry points

Review note:

- richer execution semantics still rely on primitive runtime operations in many
  cases; `IExecutionRuntime` sits above `IRuntime`, not beside it as a fully
  independent machine layer

Reviewed decisions:

- all language-level evaluation that cannot be resolved as simple primitive
  runtime access should go through `IExecutionRuntime`
- plain `IRuntime` remains the primitive substrate used for operations such as:
  - variable reads/writes
  - system-variable reads
  - motion/dwell submission
  - wait cancellation
- `IExecutionRuntime` may use `IRuntime` operations underneath while performing
  richer evaluation or execution semantics

Examples of richer semantics that should go through `IExecutionRuntime`:

- full condition evaluation
- richer expression evaluation
- parser-limited or context-dependent expression forms
- execution semantics that depend on interpreter context rather than primitive
  machine/service access alone

General action-command pattern:

- action-type execution commands should use a runtime submission model rather
  than assuming synchronous completion
- runtime submission outcomes are:
  - `Ready`: accepted and no execution wait is required
  - `Pending`: accepted, but execution flow must wait for later resolution
  - `Error`: rejected or failed
- `Pending` means the action was accepted by the runtime boundary, not
  necessarily that the physical/device-side activity is already complete
- once the pending action is later resolved by runtime/external logic,
  `resume(...)` continues engine execution

This pattern applies broadly to action commands such as:

- motion
- dwell/timing
- tool change
- other runtime-managed machine/device actions added later
