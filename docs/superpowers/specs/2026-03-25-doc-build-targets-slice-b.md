# Documentation Build Targets Slice B

## Goal

Move generated documentation outputs out of the source tree and make docs
generation a normal build-tree concern.

## Scope

This slice is a follow-up to documentation information architecture cleanup.

It should include:

- CMake custom targets for docs generation
- mdBook output under `build/docs/book/`
- execution-contract review output under `build/docs/execution-contract-review/`
- install/export updates that copy generated docs from the build tree

It should not include:

- canonical source-doc relocation inside `docs/src/`
- new product/design content changes unrelated to docs generation

## Target Build Outputs

Generated documentation should live under the build tree:

- `build/docs/book/`
- `build/docs/execution-contract-review/`

Source-controlled docs should remain under:

- `docs/src/`

Generated docs should no longer be treated as canonical source artifacts.

## Target Build Targets

Expected custom targets:

- `docs`
- `execution_contract_review_site`
- `docs_all`

## Install/Export Rule

Public install/export should consume generated docs from build output
directories, not from generated files under `docs/src/`.

## Follow-Up

This slice should be implemented only after Slice A is merged.
