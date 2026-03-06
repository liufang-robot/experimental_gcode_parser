# Development Workflow

## Repository Layout

- `src/`: parser, AST, diagnostics, lowering, streaming APIs.
- `test/`: GoogleTest unit, regression, and fuzz-smoke suites.
- `testdata/`: golden and fixture assets.
- `dev/`: local scripts such as `check.sh` and `bench.sh`.
- `docs/`: mdBook sources and checked-in Mermaid assets.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Quality Gate

```bash
./dev/check.sh
```

`./dev/check.sh` runs format check, configure/build, tests, tidy checks, and
sanitizer tests.

## Documentation Build

```bash
cargo install mdbook mdbook-mermaid
mdbook build docs
mdbook serve docs --open
```

Mermaid diagrams in `docs/src/development/design/*.md` require
`mdbook-mermaid`.

## CLI Modes

`gcode_parse` supports:

- `--mode parse` for AST + diagnostics output
- `--mode ail` for intermediate AIL instruction output (`json` or debug summary)
- `--mode packet` for AIL->packet output (`json` or debug summary)
- `--mode lower` for lowered message output (`json` or debug summary)
- CLI stage goldens are stored in `testdata/cli/` and should be updated in the
  same PR when stage output intentionally changes.

## OODA Development Loop

1. Observe current repo and CI state.
2. Orient against `ROADMAP.md`, `BACKLOG.md`, and `SPEC.md`.
3. Decide one scoped backlog slice.
4. Act with code, tests, and docs updates in one PR.

## Contribution Requirements

- Keep C++17.
- Add or adjust tests for every feature.
- Update `SPEC.md` if behavior changes.
- Update `CHANGELOG_AGENT.md` in every change.
- Keep `docs/` aligned with implementation changes.
