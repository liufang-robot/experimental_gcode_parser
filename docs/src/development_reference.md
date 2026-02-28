# Development Reference

## Repository Layout

- `src/`: parser, AST, diagnostics, lowering, streaming APIs.
- `test/`: GoogleTest unit/regression/fuzz-smoke suites.
- `testdata/`: golden and fixture assets.
- `dev/`: local scripts such as `check.sh` and `bench.sh`.
- `docs/`: mdBook sources.

## Local Workflow

### Build

```bash
cmake -S . -B build
cmake --build build -j
```

### Quality Gate

```bash
./dev/check.sh
```

`./dev/check.sh` runs format check, configure/build, tests, tidy checks, and
sanitizer tests.

### CLI Modes

`gcode_parse` supports:

- `--mode parse` for AST + diagnostics output
- `--mode lower` for lowered message output (`json` or debug summary)

## OODA Development Loop

1. Observe current repo + CI state.
2. Orient against `ROADMAP.md`, `BACKLOG.md`, and `SPEC.md`.
3. Decide one scoped backlog slice.
4. Act with code/tests/docs updates in one PR.

## Contribution Requirements

- Keep C++17.
- Add/adjust tests for every feature.
- Update `SPEC.md` if behavior changes.
- Update `CHANGELOG_AGENT.md` in every change.
- Keep `docs/` aligned with implementation changes.
