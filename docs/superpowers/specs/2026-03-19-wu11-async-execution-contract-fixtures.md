# WU-11 Async Execution Contract Fixtures

## Purpose

Extend the execution-contract fixture system from synchronous end states into
reviewable asynchronous public execution behavior.

This work unit is about the contract-review system, not about inventing new
runtime semantics. The public `ExecutionSession` API already supports:

- `Blocked`
- `resume(...)`
- `cancel()`

The missing part is fixture-level expression of those interactions and exact
reviewable traces for them.

## First Slice

The first slice is intentionally narrow:

- one async motion case
- explicit fixture driver steps
- `blocked` + `resume`
- no `cancelled` yet

This keeps the first async contract extension small enough to review and debug
without mixing multiple runtime behaviors at once.

## Goal

For an input like:

```gcode
G1 X10
G1 Y10
```

the fixture system should be able to declare:

- first move emits a `linear_move` event
- runtime returns `Pending`
- session enters `Blocked`
- driver calls `resume_blocked`
- second move emits a second `linear_move`
- session reaches `Completed`

The resulting ordered event trace becomes a reviewed public execution contract.

## Current Limitation

Today the execution-contract runner only does one straight-line interaction:

1. create `ExecutionSession`
2. `pushChunk(...)`
3. `finish()`
4. `pump()` until status is not `Progress`
5. record the terminal status

That allows:

- `completed`
- `rejected`
- `faulted`
- `blocked` as a terminal event

But it does not allow:

- continuing after `Blocked`
- expressing `resume(...)`
- recording a full multi-step async interaction

## Core Design

Add an explicit fixture-level `driver` section to the reference trace.

Initial shape:

```yaml
driver:
  - action: finish
  - action: resume_blocked
```

Rules for the first slice:

- `finish`
  - call `ExecutionSession::finish()`
- `resume_blocked`
  - requires the current session step to be `Blocked`
  - uses the blocked token returned by the session
  - calls `ExecutionSession::resume(token)`
  - continues the driver loop

This is intentionally explicit. The fixture should say how the review runner
drives the public session, instead of the runner inferring policy silently.

## Fixture Example

Reference trace shape for the first async case:

```yaml
name: linear_move_block_resume
description: First move blocks, then resume continues with the next move.

initial_state:
  modal:
    motion_code: G1
    working_plane: xy
    rapid_mode: linear
    tool_radius_comp: off
    active_tool_selection: null
    pending_tool_selection: null
    selected_tool_selection: null

options:
  active_skip_levels: []
  tool_change_mode: deferred_m6
  enable_iso_m98_calls: false

driver:
  - action: finish
  - action: resume_blocked

expected_events:
  - type: linear_move
    source:
      filename: null
      line: 1
      line_number: null
    target:
      x: 10.0
      y: null
      z: null
      a: null
      b: null
      c: null
    feed: null
    effective:
      motion_code: G1
      working_plane: xy
      rapid_mode: linear
      tool_radius_comp: off
      active_tool_selection: null
      pending_tool_selection: null
      selected_tool_selection: null
  - type: blocked
    line: 1
    token:
      kind: motion
      id: move-001
    reason: instruction execution in progress
  - type: linear_move
    source:
      filename: null
      line: 2
      line_number: null
    target:
      x: null
      y: 10.0
      z: null
      a: null
      b: null
      c: null
    feed: null
    effective:
      motion_code: G1
      working_plane: xy
      rapid_mode: linear
      tool_radius_comp: off
      active_tool_selection: null
      pending_tool_selection: null
      selected_tool_selection: null
  - type: completed
```

## Runtime Behavior In The Runner

The contract runner needs an async-capable test runtime for the first slice.

Initial behavior:

- first submitted linear move:
  - returns `Pending`
  - token: `{"kind":"motion","id":"move-001"}`
- later submitted linear moves:
  - return `Ready`

This runtime behavior should be fixture-driven or case-driven inside the runner,
but still deterministic and human-reviewable.

The first slice does not require a general async scripting engine. It only
needs enough control to drive one reviewed blocking contract cleanly.

## Event Model

No new public event kinds are required for the first slice.

Existing event model already includes:

- `linear_move`
- `blocked`
- `completed`

The missing piece is multi-step driver execution that can produce them in one
reviewed trace.

## Out Of Scope

This first async slice does not include:

- `cancelled`
- token mismatch error contracts
- invalid `resume(...)` contracts
- async dwell/tool-change fixtures
- arbitrary runtime event scripting

Those can follow once the first async motion case proves the fixture model.

## Acceptance Criteria

- execution-contract fixtures can declare an explicit `driver`
- the review runner can execute at least:
  - `finish`
  - `resume_blocked`
- one async linear-motion core case is enforced
- the generated HTML review output shows the driver alongside the case context
- exact equality remains the comparison rule
- `./dev/check.sh` passes once implemented
