# Execution Contract Review

This page documents the human-reviewable execution contract fixture workflow.

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

## Step 1 Scope

The current enforced Step 1 suite covers:

- `modal_update`
- `linear_move_completed`
- `rejected_invalid_line`
- `fault_unresolved_target`
- `goto_skips_line`
- `if_else_branch`

## Event Model

Reference traces are ordered event lists. Step 1 uses:

- `diagnostic`
- `rejected`
- `modal_update`
- `linear_move`
- `faulted`
- `completed`

`modal_update` events contain only the changed modal fields, so every fixture
declares an explicit `initial_state`.

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
