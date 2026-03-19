# Development: Dual-Agent Workflow

This is lightweight workflow guidance for using two Codex sessions on the same
feature:

- `developer`
- `integration tester`

This is guidance, not a hard repo policy.

## Goal

Use the split to get:

- clearer production-code ownership
- stronger black-box testing
- faster discovery of contract mismatches
- less self-confirming implementation/test behavior

## Roles

### Developer

Primary ownership:

- `include/`
- `src/`
- behavior docs and spec updates when behavior changes

Developer may also write:

- internal unit tests
- white-box regression tests for internal classes/helpers
- refactor-safety tests that depend on implementation detail

Developer should generally avoid:

- editing execution-contract fixtures in `testdata/execution_contract/`
- owning public black-box contract expectations unless explicitly intended

### Integration Tester

Primary ownership:

- public-behavior tests
- integration tests
- execution-contract fixtures
- `testdata/`
- test-only helpers under `test/`

Integration tester should generally avoid:

- editing `include/`
- editing `src/`
- changing production behavior to satisfy tests

## Test Boundary

Use this rule:

- if a test depends on private structure or internal helper behavior, it belongs
  to the developer
- if a test checks user-visible behavior, public contracts, fixtures, CLI
  behavior, or end-to-end execution flow, it belongs to the integration tester

Examples:

- developer-owned:
  - parser helper unit tests
  - executor internal state transition tests
  - refactor regressions for internal support types

- integration-tester-owned:
  - `ExecutionSession` public behavior
  - execution-contract review fixtures
  - integration/regression tests across parser/lowering/execution boundaries

## Recommended Loop

1. User and developer discuss the feature, contract, and implementation intent.
2. Developer implements production code and commits it.
3. Integration tester checks out that committed developer state.
4. Integration tester adds public-behavior and contract tests only.
5. If tests fail:
   - developer fixes production code, or
   - tester fixes an incorrect test
6. Repeat until:
   - production code
   - tests
   - docs/spec
   - `./dev/check.sh`
   are all green.

## Branch And Workspace Model

Recommended setup:

- developer workspace:
  - main repo working tree or a dedicated developer worktree
- integration tester workspace:
  - separate git worktree or standalone clone

Recommended branch pattern:

- `feature/<topic>-dev`
- `feature/<topic>-test`

Or, if you prefer a simpler flow:

- one shared feature branch
- two separate worktrees
- only committed handoffs between roles

The important rule is:

- the integration tester should test committed developer code, not uncommitted
  local edits

## Disagreement Rule

When developer and integration tester disagree:

- public contract/spec wins
- reviewed fixture behavior wins over assumptions
- if contract wording is unclear, escalate to the human owner before changing
  production code or reference fixtures

## Completion Rule

A feature is done only when:

- production code is in the intended state
- tests are accepted
- docs/spec are updated where behavior changed
- `./dev/check.sh` passes

## Notes

- This workflow is especially useful for execution-contract and regression-heavy
  work.
- It is less important for tiny docs-only or formatting-only changes.
- If the split becomes cumbersome, keep the role boundary by test purpose rather
  than forcing every test change through the integration tester.

Related page:

- [Dual-Agent Feature Checklist](dual_agent_feature_checklist.md)
