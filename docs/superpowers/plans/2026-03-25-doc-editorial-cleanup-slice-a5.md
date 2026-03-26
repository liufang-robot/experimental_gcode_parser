# Documentation Editorial Cleanup Slice A.5 Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Split the oversized product docs into smaller mdBook subtrees and reduce the highest-value duplication without changing technical meaning.

**Architecture:** Start with the two largest user-facing pages, `docs/src/product/spec.md` and `docs/src/product/prd.md`. Replace each monolith with a short index page plus smaller topical pages, then update navigation and key cross-links so the product section becomes easier to scan.

**Tech Stack:** Markdown, mdBook, existing product docs under `docs/src/product/`.

---

### Task 1: Split `product/spec.md` into a subtree

**Files:**
- Delete: `docs/src/product/spec.md`
- Create: `docs/src/product/spec/index.md`
- Create: `docs/src/product/spec/input_output.md`
- Create: `docs/src/product/spec/syntax_motion_and_program.md`
- Create: `docs/src/product/spec/syntax_control_and_variables.md`
- Create: `docs/src/product/spec/diagnostics_and_lowering.md`
- Create: `docs/src/product/spec/execution_and_runtime.md`
- Create: `docs/src/product/spec/testing_and_docs_policy.md`
- Modify: `docs/src/product/index.md`
- Modify: `docs/src/SUMMARY.md`

- [x] Create `docs/src/product/spec/index.md` as a short navigation page.
- [x] Move SPEC sections 1-2 into `input_output.md`.
- [x] Split SPEC section 3 into:
  - `syntax_motion_and_program.md`
  - `syntax_control_and_variables.md`
- [x] Move SPEC sections 4-6.1 into `diagnostics_and_lowering.md`.
- [x] Move SPEC section 6.2 into `execution_and_runtime.md`.
- [x] Move SPEC sections 7-9 into `testing_and_docs_policy.md`.
- [x] Update product navigation pages to point at the new SPEC subtree.
- [x] Run: `mdbook build docs`

### Task 2: Split `product/prd.md` into a subtree

**Files:**
- Delete: `docs/src/product/prd.md`
- Create: `docs/src/product/prd/index.md`
- Create: `docs/src/product/prd/goals_and_scope.md`
- Create: `docs/src/product/prd/api_requirements.md`
- Create: `docs/src/product/prd/functional_scope_overview.md`
- Create: `docs/src/product/prd/siemens_motion_and_modal.md`
- Create: `docs/src/product/prd/variables_and_program_structure.md`
- Create: `docs/src/product/prd/quality_and_release.md`
- Modify: `docs/src/product/index.md`
- Modify: `docs/src/SUMMARY.md`

- [x] Create `docs/src/product/prd/index.md` as a short navigation page.
- [x] Move PRD sections 0-3 into `goals_and_scope.md`.
- [x] Move PRD section 4 into `api_requirements.md`.
- [x] Split PRD section 5 into:
  - `functional_scope_overview.md`
  - `siemens_motion_and_modal.md`
  - `variables_and_program_structure.md`
- [x] Move PRD sections 6-9 into `quality_and_release.md`.
- [x] Update product navigation pages to point at the new PRD subtree.
- [x] Run: `mdbook build docs`

### Task 3: Clean the local product-section duplication

**Files:**
- Modify: `docs/src/product/index.md`
- Modify: moved product subtree files as needed
- Modify: `README.md` only if product links need to be tightened

- [x] Keep `docs/src/product/index.md` short and navigational.
- [x] Remove repeated explanatory text where a link to the subtree page is enough.
- [x] Keep the moved pages technically equivalent to the old content; this slice is structural/editorial, not behavioral.

### Task 4: Validate and record

**Files:**
- Modify: `CHANGELOG_AGENT.md`

- [x] Run: `mdbook build docs`
- [x] Run: `./dev/check.sh`
- [x] Review the rendered product navigation and page lengths.
- [x] Add a `CHANGELOG_AGENT.md` entry for the A.5 product-doc split.
- [ ] Commit with a docs-only message.

## Deferred After This A.5 Pass

Still deferred after this first editorial pass:
- `docs/src/requirements/gcode_execution_requirements.md`
- large design docs under `docs/src/development/design/`
- broader README tightening outside product-link cleanup

Plan complete and updated in `docs/superpowers/plans/2026-03-25-doc-editorial-cleanup-slice-a5.md`.
