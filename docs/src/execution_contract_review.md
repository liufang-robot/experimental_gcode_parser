# Execution Contract Review

This page documents the human-reviewable execution contract fixture workflow.

Direct entry:

- [Open the generated execution contract review site](generated/execution-contract-review/index.html)

The fixture system validates the public `ExecutionSession` behavior with:

- persistent `.ngc` input files
- persistent approved `.events.yaml` reference traces
- generated `.actual.yaml` traces from the current code
- a generated static HTML comparison site

## Source Of Truth

Reference fixtures live in:

- `testdata/execution_contract/core/`

Each supported case uses:

- `<case>.ngc`
- `<case>.events.yaml`

Fixtures also declare explicit execution/lowering inputs under:

- `options`

Fixtures may also declare an explicit session driver for multi-step public
interactions:

- `driver`

Fixtures may also declare deterministic runtime inputs in the reference trace:

- `runtime.system_variables`
- `runtime.system_variable_reads`
- `runtime.linear_move_results`

The current fixture-level options mirror the public `LowerOptions` fields that
can affect observable execution behavior:

- `filename`
- `active_skip_levels`
- `tool_change_mode`
- `enable_iso_m98_calls`

For simple fixed-value cases, fixtures may keep using:

- `runtime.system_variables`

When trace timing matters, fixtures may declare an ordered runtime-read script:

- `runtime.system_variable_reads`

The read script is consumed in strict execution order, so it can model:

- repeated reads of the same variable with different values
- motion reads followed by later condition reads
- post-resume reads that should appear later in the trace
- per-attempt `ready`, `pending`, and `error` outcomes for the same variable

For the first async fixture slice, `runtime.linear_move_results` can declare a
deterministic sequence of linear-move submission outcomes such as:

- `ready`
- `pending` with an explicit wait token

If a fixture omits `driver`, the review runner uses the current default
single-step behavior:

- one implicit `finish`

## Enforced Scope

The current enforced core suite covers:

- `modal_update`
- `linear_move_completed`
- `linear_move_blocked`
- `linear_move_cancelled`
- `linear_move_block_resume`
- `linear_move_system_variable_x`
- `linear_move_system_variable_selector_x`
- `motion_then_condition_system_variable_reads`
- `repeated_system_variable_reads`
- `rapid_move_system_variable_z`
- `resumed_system_variable_read`
- `dwell_seconds_completed`
- `tool_change_deferred_m6`
- `rejected_invalid_line`
- `fault_unresolved_target`
- `goto_skips_line`
- `if_else_branch`
- `if_system_variable_false_branch`
- `if_system_variable_selector_false_branch`

## Event Model

Reference traces are ordered event lists. Step 1 uses:

- `diagnostic`
- `rejected`
- `modal_update`
- `system_variable_read`
- `linear_move`
- `dwell`
- `tool_change`
- `faulted`
- `completed`

`modal_update` events contain only the changed modal fields, so every fixture
declares an explicit `initial_state`.

The first async extension keeps the same event model and adds a fixture-level
driver so reviewed traces can cover:

- `linear_move` -> `blocked`
- `linear_move` -> `blocked` -> `linear_move` -> `completed`
- `linear_move` -> `blocked` -> `cancelled`

The driver now supports:

- `finish`
- `resume_blocked`
- `cancel_blocked`

`system_variable_read` is a contract-trace event, not a new public sink
callback. It records:

- the source line where execution performed the read
- the variable name
- the per-attempt outcome:
  - `ready`
  - `pending`
  - `error`
- for `ready`, the returned value
- for `pending`, the wait token
- for `error`, the runtime error message

If a pending read later resumes and execution retries that same read, the
retry emits a fresh `system_variable_read` event instead of mutating the
earlier pending event in place.

## Review CLI

Build:

```bash
cmake -S . -B build
cmake --build build -j --target gcode_execution_contract_review
```

Generate actual traces and a local HTML site:

```bash
./build/gcode_execution_contract_review \
  --fixtures-root testdata/execution_contract/core \
  --output-root output/execution_contract_review
```

This writes:

- actual traces under `output/execution_contract_review/core/`
- a local HTML site under `output/execution_contract_review/site/`

For the normal combined docs build, use:

```bash
./dev/build_docs_site.sh
```

When published, the generated review site is available under:

- [Open the generated execution contract review site](generated/execution-contract-review/index.html)

That generated subtree is written into `docs/src/generated/` during the docs
build, ignored by git, and then copied into the final mdBook output so one
`mdbook serve` instance can host both the handwritten pages and the generated
review site on the same port.

## Equality Rule

Automated tests compare reference and actual traces with exact semantic
equality.

Allowed normalization is limited to serialization details such as key order and
line endings. If behavior changes intentionally, update the reference fixture
explicitly.
