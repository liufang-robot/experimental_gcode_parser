# Execution Requirements: Motion and Timing

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
- `Pending` does not mean “retry the same motion submission later from the
  engine/session side”
- once runtime/external logic resolves the pending motion, `resume(...)`
  continues execution
- readiness for a pending motion token is determined by the embedding runtime
  or run manager, not by the library
- `resume(...)` must continue from the existing blocked execution state; it
  must not resubmit the original motion command
- `resume(...)` should be invoked by the thread that owns the execution
  session/engine
- for queue-backed runtimes, a valid readiness condition is that the held move
  was successfully pushed into the downstream queue
- the runtime boundary is a poor fit for long blocking inside motion-submission
  calls; the preferred design is to return `Pending` quickly and manage the
  wait asynchronously on the runtime side
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
- `Pending` does not mean “retry the same dwell/timing submission later from
  the engine/session side”
- once runtime/external logic resolves the pending dwell/timing action,
  `resume(...)` continues execution
- readiness for a pending dwell/timing token is determined by the embedding
  runtime or run manager, not by the library
- `resume(...)` must continue from the existing blocked execution state; it
  must not resubmit the original dwell/timing command
- `cancel()` should attempt runtime cancellation of in-flight dwell/timing wait
  and then move the engine to `Cancelled`

Review note:

- the remaining follow-up is mainly dwell-command field layout design, since
  the behavioral contract is now reviewed
