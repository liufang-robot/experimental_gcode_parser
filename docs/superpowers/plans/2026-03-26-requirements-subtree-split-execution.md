# Requirements Subtree Split: Execution First

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Split `docs/src/requirements/execution/index.md` into a
small mdBook subtree without changing technical meaning.

**Architecture:** Keep the top-level `Requirements` chapter as
`syntax` / `semantic validation` / `execution`, but replace the large execution
page with a short index page plus focused child pages.

**Tech Stack:** Markdown, mdBook, existing requirements docs under
`docs/src/requirements/`.

---

### Task 1: Create the execution subtree

**Files:**
- Delete: `docs/src/requirements/execution/index.md`
- Create: `docs/src/requirements/execution/index.md`
- Create: `docs/src/requirements/execution/modal_state_and_streaming.md`
- Create: `docs/src/requirements/execution/runtime_boundaries_and_actions.md`
- Create: `docs/src/requirements/execution/motion_and_timing.md`
- Create: `docs/src/requirements/execution/variables_control_flow_and_calls.md`
- Create: `docs/src/requirements/execution/tools_diagnostics_and_validation.md`

- [ ] Create `docs/src/requirements/execution/index.md` as a short navigation page.
- [ ] Move review-status and sections 1-2 into `modal_state_and_streaming.md`.
- [ ] Move section 3 into `runtime_boundaries_and_actions.md`.
- [ ] Move sections 4-5 into `motion_and_timing.md`.
- [ ] Move sections 6-8 into `variables_control_flow_and_calls.md`.
- [ ] Move sections 9-12 into `tools_diagnostics_and_validation.md`.

### Task 2: Update navigation and entry points

**Files:**
- Modify: `docs/src/requirements/index.md`
- Modify: `docs/src/SUMMARY.md`
- Modify: any nearby chapter index pages that still link the old monolith

- [ ] Update `docs/src/requirements/index.md` to point to the execution subtree.
- [ ] Update `docs/src/SUMMARY.md` to expose the execution subtree pages.
- [ ] Tighten any nearby references so they point at the new index page rather than the deleted monolith.

### Task 3: Validate and record

**Files:**
- Modify: `CHANGELOG_AGENT.md`

- [ ] Run `mdbook build docs`.
- [ ] Run `./dev/check.sh`.
- [ ] Review page length and reading sequence in the rendered book.
- [ ] Add a `CHANGELOG_AGENT.md` entry.
- [ ] Commit with a docs-only message.

## Deferred After This Slice

Still deferred after this execution split:
- splitting `docs/src/requirements/gcode_syntax_requirements.md`
- splitting `docs/src/requirements/gcode_semantic_requirements.md`
- broader editorial shortening inside the execution pages beyond the structural move
