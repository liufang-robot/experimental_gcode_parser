# Development: Dual-Agent Feature Checklist

Use this checklist when running a feature through the `developer` +
`integration tester` workflow.

## 1. Define The Feature

- name the feature or work unit
- identify the user-visible contract
- identify any required spec or doc updates
- decide whether the work is:
  - internal/refactor-heavy
  - public-behavior-heavy
  - both

## 2. Developer Setup

- create or choose the developer branch/worktree
- implement production code in:
  - `include/`
  - `src/`
- add internal/unit tests if needed for implementation safety
- update behavior docs/spec if behavior changed
- commit the developer slice before handoff

## 3. Integration Tester Setup

- check out the committed developer state in a separate worktree/session
- add only:
  - public-behavior tests
  - integration tests
  - contract fixtures
  - `testdata/`
- avoid editing production code
- commit the tester slice before handing failures back

## 4. Failure Triage

If tests fail, decide which side owns the fix:

- developer owns:
  - production bugs
  - missing implementation
  - public behavior that does not match the approved contract

- integration tester owns:
  - incorrect assertions
  - bad fixture expectations
  - test bugs or invalid setup

If unclear:

- check the public contract/spec first
- escalate to the human owner if the contract is ambiguous

## 5. Repeat Loop

- developer fixes code and commits
- integration tester reruns/extends tests and commits
- repeat until the behavior and tests converge

## 6. Completion Checklist

- production code is complete
- internal tests are acceptable
- public/integration/contract tests are acceptable
- docs/spec are updated where behavior changed
- `./dev/check.sh` passes
- merge/cleanup plan is clear

## Suggested Commit Labels

These are optional but useful:

- developer:
  - `dev: implement <feature>`
  - `dev: fix <feature> after tester feedback`

- integration tester:
  - `test: add public coverage for <feature>`
  - `test: fix fixture/assertion for <feature>`
