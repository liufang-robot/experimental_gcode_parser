# G-code Execution Requirements

This document collects modal-state and runtime/execution requirements. It is
separate from syntax requirements on purpose.

For each execution family, review should answer:

- what state must exist
- when that state changes
- what must block or wait
- what runtime interface is responsible
- what deterministic tests must exist

## Review Status

Status values to use during review:

- `Wanted`
- `Reviewed`
- `Deferred`
- `Rejected`

Current state of this document:

- `Wanted`: initial consolidated checklist extracted from current execution
  notes and streaming refactor work
- not yet fully reviewed item-by-item

## 1. Modal State Model

Status: `Reviewed` (2026-03-10 review pass)

State families to review:

- motion mode (`G0/G1/G2/G3`)
- dwell/non-motion modal families where relevant
- working plane
- rapid interpolation mode
- tool radius compensation
- tool selection/change state
- feed mode / units / dimensions / work offsets if wanted

Reviewed decisions:

- every supported modal group should be explicit in the final execution model
- future modal groups should be added by extending the explicit modal-state
  model, not by ad hoc side channels
- if the parser/runtime accepts a modal family, the execution-state model
  should have a place to preserve it explicitly, even if runtime behavior is
  still partial
- emitted execution commands should carry the effective values of the
  execution-relevant modal groups needed for correct downstream execution

Review note:

- this decision sets the structural rule for modal modeling; the exact list of
  currently supported groups and which ones must be copied onto each command
  still need to be filled in explicitly

Reviewed decisions:

- emitted command families should carry all effective supported modal-group
  values, not just a selected subset
- no supported modal group should be treated as executor-internal-only by
  default; modal state is part of execution input and must remain visible in
  the modeled execution state
- all supported modal groups must be preserved in execution state even when
  current runtime behavior for some groups is still partial or deferred

Follow-up implementation note:

- the baseline field layout for emitted command snapshots is now established
  (`motion_code`, `working_plane`, `rapid_mode`, `tool_radius_comp`,
  `active_tool_selection`, `pending_tool_selection`), but future work still
  needs to extend that snapshot until it covers every supported modal group

## 2. Streaming Execution Model

Status: `Reviewed` (2026-03-11 review pass)

Requirements to review:

- chunk input
- line assembly
- `pushChunk(...)`
- `pump()`
- `finish()`
- `resume(...)`
- `cancel()`

Required engine states to review:

- `ReadyToExecute`
- `Blocked`
- `WaitingForInput`
- `Completed`
- `Cancelled`
- `Faulted`

Reviewed decisions:

- `pushChunk(...)` only buffers input and prepares internal buffered state; it
  does not autonomously drive execution
- `pump()` advances execution until a meaningful boundary, not just exactly one
  command
- `finish()`:
  - marks EOF
  - flushes a final unterminated line if it forms a valid block
  - converts unresolved forward-target waits into faults if still unresolved at
    EOF
- `resume(...)` is only for continuing from runtime/external blocking
- `cancel()`:
  - attempts to cancel any pending runtime wait/action
  - moves the engine to `Cancelled`
  - prevents further execution progress afterward
- `Blocked` and `WaitingForInput` are distinct states:
  - `Blocked` means runtime/external waiting
  - `WaitingForInput` means more G-code input/context is required
- unresolved forward targets should behave as:
  - before EOF: `WaitingForInput`
  - after `finish()`: `Faulted`

Review note:

- the high-level state-machine contract is now reviewed; the remaining follow-up
  work is mostly about per-feature execution semantics, not the streaming API
  shape itself

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

## 4. Motion Execution Requirements

Status: `Reviewed` (2026-03-11 review pass)

Per-command review:

- how `G0/G1/G2/G3` become normalized execution commands
- what source information must be carried
- what effective modal state must be attached
- when motion submission blocks
- what completion/cancel behavior is required

Reviewed decisions:

- `G0` and `G1` should normalize into explicit linear-move command structures
- `G2` and `G3` should normalize into explicit arc-move command structures
- normalized motion commands should be sent through the runtime interface
  boundary rather than exposing raw parser words downstream
- motion commands must carry source information including:
  - source file/path if known
  - physical input line number
  - block number `N...` if present
- motion commands must carry all effective supported modal-group values
- motion submission follows the general async action-command pattern:
  - `Ready`: accepted and execution can continue
  - `Pending`: accepted and engine enters `Blocked`
  - `Error`: execution faults
- in the async model, `Pending` means the motion command was accepted by the
  runtime boundary, not necessarily physically completed
- once runtime/external logic resolves the pending motion, `resume(...)`
  continues execution
- `cancel()` should attempt runtime cancellation of in-flight motion/wait and
  then move the engine to `Cancelled`

Review note:

- the remaining follow-up is mainly command-field layout design, since the
  behavioral contract is now reviewed

## 5. Dwell / Timing Execution Requirements

Status: `Reviewed` (2026-03-11 review pass)

Requirements to review:

- how `G4` becomes a normalized dwell/timing command
- what source information must be carried
- what effective modal/timing state must be attached, if any
- when dwell submission blocks
- what completion/cancel behavior is required

Reviewed decisions:

- `G4` should normalize into an explicit dwell/timing command structure
- normalized dwell/timing commands should be sent through the runtime interface
  boundary rather than exposing raw parser words downstream
- dwell/timing commands must carry source information including:
  - source file/path if known
  - physical input line number
  - block number `N...` if present
- dwell/timing commands must carry all effective supported modal-group values
  under the general execution-state rule
- dwell/timing submission follows the general async action-command pattern:
  - `Ready`: accepted and execution can continue
  - `Pending`: accepted and engine enters `Blocked`
  - `Error`: execution faults
- in the async model, `Pending` means the dwell/timing command was accepted by
  the runtime boundary, not necessarily fully completed
- once runtime/external logic resolves the pending dwell/timing action,
  `resume(...)` continues execution
- `cancel()` should attempt runtime cancellation of in-flight dwell/timing wait
  and then move the engine to `Cancelled`

Review note:

- the remaining follow-up is mainly dwell-command field layout design, since
  the behavioral contract is now reviewed

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

## 9. Tool Execution Requirements

Status: `Reviewed` (2026-03-12 review pass)

Requirements to review:

- `tool_select`
- `tool_change`
- deferred vs immediate timing
- pending tool state
- `M6` behavior with/without pending selection
- substitution/fallback policy model

Reviewed decisions:

- `tool_select` and `tool_change` are separate execution concepts
- tool selection prepares the tool state but does not mount the tool onto the
  spindle
- tool change performs the actual spindle-mounted tool change
- actual tool change should be triggered later by `M6` or the equivalent
  change-trigger command, rather than by selection alone
- tool-change execution should go through the runtime interface boundary
- tool-change execution should be modeled as asynchronous and normally use the
  general `Pending` action-command behavior
- pending/current/selected tool state should remain explicit in execution state
- if `M6` occurs with no pending tool selection, the executor should use the
  current active/default tool selection as the tool-change target
- if the active/default tool is already mounted, runtime may treat that tool
  change as a no-op action
- if `M6` occurs with neither a pending tool selection nor an active/default
  tool selection, execution should fault

Review note:

- the main tool-selection/tool-change execution split and `M6` no-pending
  policy are now reviewed; further controller-specific substitution/fallback
  details can still be refined later

## 10. Diagnostics and Failure Policy

Status: `Reviewed` (2026-03-12 review pass)

Requirements to review:

- syntax diagnostic vs runtime diagnostic
- rejected line vs executor fault
- warning/error/ignore policies
- deterministic source attribution
- duplicate or repeated diagnostics policy

Reviewed decisions:

- syntax/semantic problems should produce syntax/semantic diagnostics
- runtime/execution failures should produce runtime/execution diagnostics or
  faults
- the engine should support halting at a line with syntax/semantic failure
  while preserving enough buffered state that execution can continue once that
  line is corrected and reprocessed
- unsupported constructs should default to `Error`
- duplicate constructs that are still executable should default to `Warning`
- anything ambiguous or unsafe for execution should default to `Error`
- diagnostics and faults must carry deterministic source attribution including:
  - source file/path if known
  - physical input line number
  - block number `N...` if present
- diagnostics should not be globally deduplicated across the whole program
- diagnostics may be deduplicated within the same line/block/execution event to
  avoid repeated identical reports for the same underlying problem

Review note:

- the remaining follow-up is to attach exact warning/error choices to specific
  semantic families over time, but the default policy is now reviewed

## 11. Test and Validation Requirements

Status: `Reviewed` (2026-03-12 review pass)

Required test families to review:

- parser unit tests
- lowering/AIL tests
- executor tests
- streaming execution tests
- fake-log CLI traces
- golden fixtures
- fuzz/sanitizer gate via `./dev/check.sh`

Reviewed decisions:

- each work unit should declare which test layers it affects rather than
  forcing every work unit to add every possible test type
- golden files should be used for:
  - parser output
  - CLI output
  - fake-log/event traces
  - serialized structured output
- direct assertions should be used for:
  - executor/runtime API behavior
  - call sequence and state-machine behavior
  - focused semantic checks
- execution/runtime/streaming changes should include a reviewable fake-log CLI
  trace or equivalent event-trace coverage
- every work unit should record:
  - exact test names
  - exact commands
  - fixture files used
- `./dev/check.sh` remains the required final validation gate for
  implementation slices
- every work unit should map back to:
  - requirement section(s)
  - test file(s)
  - fixture(s)
- every feature/work unit should include invalid/failure cases where
  applicable
- streaming-related changes should explicitly cover:
  - chunk split cases
  - no trailing newline
  - `WaitingForInput`
  - `Blocked`
  - `resume(...)`
  - `cancel()`
  - EOF unresolved-target fault

Review note:

- the test policy is now reviewed; the remaining work is applying it
  consistently to future work units and implementation slices

## 12. Work-Unit Readiness

A work unit should not start implementation until the relevant requirement
subset is:

- reviewed
- assigned clear in-scope/out-of-scope boundaries
- paired with expected tests
- paired with unresolved-question list if decisions are still missing
