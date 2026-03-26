# Execution Requirements: Modal State and Streaming

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
