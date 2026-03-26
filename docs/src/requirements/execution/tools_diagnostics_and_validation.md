# Execution Requirements: Tools, Diagnostics, and Validation

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
