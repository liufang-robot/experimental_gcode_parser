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

Pending examples that describe reviewed requirements but are not yet enforced by
the public `ExecutionSession` path live in:

- `testdata/execution_contract/pending/`

## Step 1 Scope

The current enforced Step 1 suite covers:

- `modal_update`
- `linear_move_completed`
- `rejected_invalid_line`
- `fault_unresolved_target`

The current pending control-flow examples are:

- `goto_skips_line`
- `if_else_branch`

Those pending cases are kept for future work but are not part of the exact
equality gate yet.

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

To publish the review site into the GitHub Pages output tree after `mdbook build
docs`, use:

```bash
./build/gcode_execution_contract_review \
  --fixtures-root testdata/execution_contract/core \
  --output-root output/execution_contract_review \
  --publish-root docs/book/execution-contract-review
```

For the normal combined docs build, use:

```bash
./dev/build_docs_site.sh
```

When published, the generated review site is available under:

- `execution-contract-review/index.html`

## Equality Rule

Automated tests compare reference and actual traces with exact semantic
equality.

Allowed normalization is limited to serialization details such as key order and
line endings. If behavior changes intentionally, update the reference fixture
explicitly.
