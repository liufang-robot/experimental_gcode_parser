# 2026-03-19 WU-11 Async Execution Contract Fixtures

## Selected Slice

- `WU-11 Execution Contract Fixtures Step 2`
- first slice: async linear move with `blocked` + `resume`

## Why This Next

- the public `ExecutionSession` async API already exists
- the current contract suite still stops at synchronous end states
- one reviewed async motion case will validate the fixture-driver design before
  we add `cancelled` or more complex async families

## Scope

Implement only:

- fixture-level `driver`
- runner support for:
  - `finish`
  - `resume_blocked`
- one enforced async motion contract case
- HTML rendering of the new driver section

Do not implement:

- `cancelled`
- invalid resume edge cases
- async dwell/tool-change cases
- generalized runtime scripting

## Files Likely In Scope

- `src/execution_contract_fixture.h`
- `src/execution_contract_fixture.cpp`
- `src/execution_contract_runner.cpp`
- `src/execution_contract_html.cpp`
- `test/execution_contract_fixture_tests.cpp`
- `test/execution_contract_runner_tests.cpp`
- `test/execution_contract_html_tests.cpp`
- `testdata/execution_contract/core/`
- `docs/src/execution_contract_review.md`
- `SPEC.md`
- `CHANGELOG_AGENT.md`

## Proposed Steps

1. Extend the fixture schema with a `driver` section.
2. Add loader/serializer tests for driver actions.
3. Extend the runner to execute driver actions against the public
   `ExecutionSession`.
4. Add one deterministic async motion runtime path for the reviewed case.
5. Add one enforced core fixture:
   - `linear_move_block_resume`
6. Render driver data in the generated HTML review page.
7. Update docs/spec/changelog.
8. Run `./dev/check.sh`.

## Verification Target

- `./build/execution_contract_fixture_tests`
- `./build/execution_contract_runner_tests`
- `./build/execution_contract_html_tests`
- `./build/gcode_execution_contract_review --fixtures-root testdata/execution_contract/core --output-root output/execution_contract_review --publish-root docs/src/generated/execution-contract-review`
- `./dev/check.sh`

## Review Checkpoint

Before implementation starts, confirm:

- first async case stays motion-only
- `cancelled` remains a later slice
- driver behavior is explicit in the fixture, not hidden in the runner
