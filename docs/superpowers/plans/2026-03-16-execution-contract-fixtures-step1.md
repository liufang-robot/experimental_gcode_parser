# Execution Contract Fixtures Step 1 Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Step 1 execution-contract fixtures for the public `ExecutionSession` API, including persistent `.ngc` and reference `.events.yaml` files, generated `.actual.yaml` results, and a static HTML comparison site.

**Architecture:** The fixture system tests only public execution-visible behavior. Source-of-truth lives in `testdata/execution_contract/`, generated actual traces and HTML review pages live under `output/` and the published docs tree, and automated tests compare actual vs reference with exact semantic equality. The implementation should reuse the existing execution-session behavior and logging patterns instead of inventing a second execution path.

**Tech Stack:** C++17, CMake, GoogleTest, existing `ExecutionSession` + sink/runtime contracts, YAML serialization, static HTML generation, `./dev/check.sh`

---

## File Structure

### New or Expanded Source-of-Truth Areas

- Create: `testdata/execution_contract/README.md`
  - explains fixture layout and review workflow
- Create: `testdata/execution_contract/core/*.ngc`
  - Step 1 input programs
- Create: `testdata/execution_contract/core/*.events.yaml`
  - Step 1 approved reference traces

### New Code

- Create: `src/execution_contract_fixture.h`
  - fixture model types and loader declarations
- Create: `src/execution_contract_fixture.cpp`
  - parse/load `.events.yaml`, serialize actual event traces
- Create: `src/execution_contract_runner.h`
  - runner declarations for producing actual traces from `ExecutionSession`
- Create: `src/execution_contract_runner.cpp`
  - drives `ExecutionSession`, records ordered events, writes actual YAML
- Create: `src/execution_contract_html.h`
  - HTML rendering declarations
- Create: `src/execution_contract_html.cpp`
  - static compare-site generation
- Create: `src/execution_contract_main.cpp`
  - CLI entry point for fixture execution and HTML generation

### New Tests

- Create: `test/execution_contract_fixture_tests.cpp`
  - fixture parsing/serialization unit tests
- Create: `test/execution_contract_runner_tests.cpp`
  - scenario-driven exact comparison tests against reference fixtures
- Create: `test/execution_contract_html_tests.cpp`
  - smoke tests for generated HTML/index structure

### Build / Docs

- Modify: `CMakeLists.txt`
  - add new library sources, test targets, and CLI target
- Modify: `docs/src/SUMMARY.md`
  - add link to the execution-contract review site description if needed
- Modify: `docs/src/index.md`
  - mention the execution-contract review capability
- Modify: `README.md`
  - mention the new fixture runner/contract review tool
- Modify: `SPEC.md`
  - document execution-contract fixture expectations and exact-equality rule
- Modify: `CHANGELOG_AGENT.md`

---

## Chunk 1: Fixture Schema And Loader

### Task 1: Add fixture model types

**Files:**
- Create: `src/execution_contract_fixture.h`

- [ ] **Step 1: Define fixture model structs**

Include:
- case metadata
- initial modal state
- event representation for Step 1 event types
- reference/actual trace container

- [ ] **Step 2: Keep the schema Step 1 only**

Do not include async-only Step 2 fields yet:
- no `resume` workflow fields
- no blocked timeline helpers beyond plain event representation

- [ ] **Step 3: Commit the header skeleton**

```bash
git add src/execution_contract_fixture.h
git commit -m "feat: add execution contract fixture model types"
```

### Task 2: Write failing fixture parsing test

**Files:**
- Create: `test/execution_contract_fixture_tests.cpp`
- Test: `testdata/execution_contract/core/`

- [ ] **Step 1: Write failing test for loading one fixture**

Test should verify:
- `.events.yaml` loads
- `initial_state` exists
- ordered event list is parsed
- modal-update `changes` are preserved

- [ ] **Step 2: Run the single test and verify failure**

Run:
```bash
cmake --build build -j --target execution_contract_fixture_tests
./build/execution_contract_fixture_tests
```

Expected:
- build or test fails because loader implementation is missing

- [ ] **Step 3: Implement loader/serializer minimally**

**Files:**
- Create: `src/execution_contract_fixture.cpp`

Implement:
- load reference fixture YAML
- serialize actual trace YAML with stable key order

- [ ] **Step 4: Run the test again and verify pass**

Run:
```bash
./build/execution_contract_fixture_tests
```

- [ ] **Step 5: Commit**

```bash
git add src/execution_contract_fixture.cpp test/execution_contract_fixture_tests.cpp
git commit -m "feat: add execution contract fixture loading"
```

## Chunk 2: Step 1 Fixture Cases

### Task 3: Add initial Step 1 fixtures

**Files:**
- Create:
  - `testdata/execution_contract/README.md`
  - `testdata/execution_contract/core/modal_update.ngc`
  - `testdata/execution_contract/core/modal_update.events.yaml`
  - `testdata/execution_contract/core/linear_move_completed.ngc`
  - `testdata/execution_contract/core/linear_move_completed.events.yaml`
  - `testdata/execution_contract/core/goto_skips_line.ngc`
  - `testdata/execution_contract/core/goto_skips_line.events.yaml`
  - `testdata/execution_contract/core/if_else_branch.ngc`
  - `testdata/execution_contract/core/if_else_branch.events.yaml`
  - `testdata/execution_contract/core/rejected_invalid_line.ngc`
  - `testdata/execution_contract/core/rejected_invalid_line.events.yaml`
  - `testdata/execution_contract/core/fault_unresolved_target.ngc`
  - `testdata/execution_contract/core/fault_unresolved_target.events.yaml`

- [ ] **Step 1: Write the fixture README**

Document:
- file naming
- persistent source-of-truth rule
- exact-equality comparison rule
- Step 1 vs Step 2 scope

- [ ] **Step 2: Author the six Step 1 fixtures**

Keep all fixtures human-readable and concise.

- [ ] **Step 3: Commit**

```bash
git add testdata/execution_contract
git commit -m "test: add step-1 execution contract fixtures"
```

## Chunk 3: Runner And Exact Comparison

### Task 4: Write failing runner comparison test

**Files:**
- Create: `test/execution_contract_runner_tests.cpp`

- [ ] **Step 1: Write failing comparison test for one fixture**

Test should:
- load `.ngc`
- run `ExecutionSession`
- record actual ordered event trace
- compare to reference fixture

- [ ] **Step 2: Run the test and verify failure**

Run:
```bash
cmake --build build -j --target execution_contract_runner_tests
./build/execution_contract_runner_tests
```

Expected:
- failure because runner implementation is missing

- [ ] **Step 3: Implement the runner**

**Files:**
- Create: `src/execution_contract_runner.h`
- Create: `src/execution_contract_runner.cpp`

Implement:
- deterministic execution through `ExecutionSession`
- recording sink/runtime suitable for Step 1 cases
- exact comparison against reference fixture
- actual-yaml writeout into `output/execution_contract_review/`

- [ ] **Step 4: Expand tests to all Step 1 fixtures**

Ensure each initial fixture runs through the same public session path.

- [ ] **Step 5: Run tests and verify pass**

Run:
```bash
./build/execution_contract_runner_tests
```

- [ ] **Step 6: Commit**

```bash
git add src/execution_contract_runner.h src/execution_contract_runner.cpp test/execution_contract_runner_tests.cpp
git commit -m "feat: add execution contract fixture runner"
```

## Chunk 4: HTML Review Site

### Task 5: Write failing HTML smoke test

**Files:**
- Create: `test/execution_contract_html_tests.cpp`

- [ ] **Step 1: Write failing test for generated index and case page**

Verify:
- index contains case links
- case page contains input/reference/actual sections
- mismatch status can render

- [ ] **Step 2: Run test and verify failure**

Run:
```bash
cmake --build build -j --target execution_contract_html_tests
./build/execution_contract_html_tests
```

- [ ] **Step 3: Implement HTML generator**

**Files:**
- Create: `src/execution_contract_html.h`
- Create: `src/execution_contract_html.cpp`

Generate:
- `output/execution_contract_review/index.html`
- `output/execution_contract_review/core/<case>.html`

- [ ] **Step 4: Add a CLI**

**Files:**
- Create: `src/execution_contract_main.cpp`

CLI responsibilities:
- run one case or all cases
- write `.actual.yaml`
- write HTML review site
- emit clear summary for pass/fail

- [ ] **Step 5: Run HTML tests and verify pass**

Run:
```bash
./build/execution_contract_html_tests
```

- [ ] **Step 6: Commit**

```bash
git add src/execution_contract_html.h src/execution_contract_html.cpp src/execution_contract_main.cpp test/execution_contract_html_tests.cpp
git commit -m "feat: add execution contract review site generator"
```

## Chunk 5: Build Integration And Docs

### Task 6: Wire targets into CMake

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add new sources to the library or helper targets**
- [ ] **Step 2: Add the new CLI target**
- [ ] **Step 3: Add the three new test targets**
- [ ] **Step 4: Build all new targets**

Run:
```bash
cmake --build build -j --target execution_contract_fixture_tests execution_contract_runner_tests execution_contract_html_tests execution_contract_review
```

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: add execution contract fixture targets"
```

### Task 7: Document the contract suite

**Files:**
- Modify: `README.md`
- Modify: `SPEC.md`
- Modify: `docs/src/index.md`
- Modify: `docs/src/SUMMARY.md`
- Modify: `CHANGELOG_AGENT.md`

- [ ] **Step 1: Document the fixture contract**

Include:
- source-of-truth file locations
- actual-yaml output location
- HTML review site location
- exact-equality rule
- Step 1 scope

- [ ] **Step 2: Document the runner commands**

Examples:
- run one case
- run all cases
- generate review site

- [ ] **Step 3: Commit**

```bash
git add README.md SPEC.md docs/src/index.md docs/src/SUMMARY.md CHANGELOG_AGENT.md
git commit -m "docs: add execution contract fixture documentation"
```

## Chunk 6: Full Verification

### Task 8: Run end-to-end verification

**Files:**
- No new files

- [ ] **Step 1: Run focused fixture tests**

```bash
./build/execution_contract_fixture_tests
./build/execution_contract_runner_tests
./build/execution_contract_html_tests
```

- [ ] **Step 2: Run the fixture CLI on all Step 1 cases**

Run:
```bash
./build/execution_contract_review --suite core
```

Expected:
- `.actual.yaml` files generated under `output/execution_contract_review/`
- HTML site generated under `output/execution_contract_review/`
- all Step 1 comparisons pass

- [ ] **Step 3: Run the repository gate**

Run:
```bash
./dev/check.sh
```

- [ ] **Step 4: Final commit if needed**

```bash
git status
```

No additional code changes should remain after verification.

## Execution Handoff

Plan complete and saved to `docs/superpowers/plans/2026-03-16-execution-contract-fixtures-step1.md`. Ready to execute?
