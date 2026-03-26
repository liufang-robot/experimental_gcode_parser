# PRD: Quality and Release

## 6. Command Family Policy

- `G` codes:
  - implement only families explicitly listed in spec and backlog
- `M` codes:
  - parsing visibility is allowed, but lowering/execution behavior must be
    specified before support is claimed
- vendor extensions:
  - require explicit spec entry and tests
- variable families:
  - require explicit product/spec treatment before runtime semantics are
    enabled

## 7. Quality Requirements

- no crashes or hangs on malformed input
- deterministic diagnostics and output ordering
- `./dev/check.sh` must pass for merge
- every bug fix gets regression coverage
- streaming APIs must support large-input workflows without requiring full
  output buffering
- maintain benchmark visibility for large inputs

## 8. Documentation and Traceability

Canonical linked product docs:

- `docs/src/product/prd/index.md`
- `docs/src/product/spec/index.md`
- `docs/src/product/program_reference.md`
- `docs/src/project/backlog.md`

Every feature PR updates all impacted docs in the same PR.

## 9. Release Acceptance

- API usable for text and file workflows
- supported families documented and test-proven
- no open P0/P1 correctness or crash issues
- CI and `./dev/check.sh` green
