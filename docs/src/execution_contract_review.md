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
- `runtime.linear_move_results`

The current fixture-level options mirror the public `LowerOptions` fields that
can affect observable execution behavior:

- `filename`
- `active_skip_levels`
- `tool_change_mode`
- `enable_iso_m98_calls`

The runtime-input slice still uses `runtime.system_variables` only for
ready-valued system-variable reads on the public `ExecutionSession` path.

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
- `linear_move_block_resume`
- `dwell_seconds_completed`
- `tool_change_deferred_m6`
- `rejected_invalid_line`
- `fault_unresolved_target`
- `goto_skips_line`
- `if_else_branch`
- `if_system_variable_false_branch`

## Event Model

Reference traces are ordered event lists. Step 1 uses:

- `diagnostic`
- `rejected`
- `modal_update`
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

`cancelled` remains follow-up work for the fixture driver and is still covered
through direct public-session tests rather than persistent execution-contract
review fixtures.

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
