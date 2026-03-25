# Documentation Information Architecture Slice A Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move canonical project documentation into `docs/src/`, keep mdBook as the main reading surface, and reduce root `AGENTS.md` to a short startup map.

**Architecture:** The root of the repo should expose only a stable entry surface (`README.md` and `AGENTS.md`) while all canonical project docs live under `docs/src/`. The mdBook table of contents becomes the primary navigation structure, and large topics are split into smaller pages instead of accumulating in root-level markdown files.

**Tech Stack:** Markdown, mdBook, repository doc structure, existing dual-agent workflow docs.

---

### Task 1: Create the target mdBook structure

**Files:**
- Create: `docs/src/product/index.md`
- Create: `docs/src/product/prd.md`
- Create: `docs/src/product/spec.md`
- Create: `docs/src/product/program_reference.md`
- Create: `docs/src/project/index.md`
- Create: `docs/src/project/roadmap.md`
- Create: `docs/src/project/backlog.md`
- Create: `docs/src/development/ooda.md`
- Modify: `docs/src/index.md`
- Modify: `docs/src/SUMMARY.md`

- [ ] Add product/project index pages with short navigation-first content.
- [ ] Move canonical content from root `PRD.md`, `SPEC.md`, `PROGRAM_REFERENCE.md`, `ROADMAP.md`, `BACKLOG.md`, and `OODA.md` into the new `docs/src/` paths.
- [ ] Update `docs/src/index.md` so the mdBook entry page points to the reorganized sections.
- [ ] Update `docs/src/SUMMARY.md` so all moved canonical pages appear in the book.
- [ ] Run: `mdbook build docs`
- [ ] Confirm the new pages are rendered in the generated book.

### Task 2: Re-home architecture and implementation-plan root docs

**Files:**
- Create: `docs/src/development/design/architecture.md`
- Create: `docs/src/development/design/implementation_plan_root.md`
- Modify: `docs/src/development/design/index.md`

- [ ] Move canonical content from root `ARCHITECTURE.md` into a mdBook-visible architecture page or split pages.
- [ ] Move canonical content from root `IMPLEMENTATION_PLAN.md` into a mdBook-visible development/design page.
- [ ] Keep pages reasonably small; split further only if needed for clarity.
- [ ] Update the development/design index so it points at the new canonical locations.
- [ ] Run: `mdbook build docs`

### Task 3: Reduce root AGENTS.md to a startup map

**Files:**
- Modify: `AGENTS.md`
- Modify: `docs/src/development/dual_agent_workflow.md` (only if cross-links need refresh)

- [ ] Replace the long root `AGENTS.md` body with a short entry-point structure.
- [ ] Add explicit role startup guidance: `developer` or `integration tester`.
- [ ] Add the rule that the agent should ask for the role if the user has not already assigned one.
- [ ] Link to canonical docs in `docs/src/` instead of duplicating detailed content.
- [ ] Keep only the minimal hard rules in root `AGENTS.md`.

### Task 4: Delete migrated root docs and repair links

**Files:**
- Delete: `ARCHITECTURE.md`
- Delete: `BACKLOG.md`
- Delete: `IMPLEMENTATION_PLAN.md`
- Delete: `OODA.md`
- Delete: `PRD.md`
- Delete: `PROGRAM_REFERENCE.md`
- Delete: `ROADMAP.md`
- Delete: `SPEC.md`
- Modify: any markdown files that still link to those old root paths

- [ ] Update internal links to the new `docs/src/` canonical paths.
- [ ] Delete the migrated root markdown files.
- [ ] Keep `README.md` and `AGENTS.md` at the root.
- [ ] Run: `rg -n "ARCHITECTURE\\.md|BACKLOG\\.md|IMPLEMENTATION_PLAN\\.md|OODA\\.md|PRD\\.md|PROGRAM_REFERENCE\\.md|ROADMAP\\.md|SPEC\\.md" .`
- [ ] Fix any stale references that should now point into `docs/src/`.

### Task 5: Record the slice and validate docs

**Files:**
- Modify: `CHANGELOG_AGENT.md`
- Modify: `docs/src/development/design/implementation_plan_from_requirements.md`

- [ ] Record Slice A completion notes in `CHANGELOG_AGENT.md`.
- [ ] Update the implementation-planning document so `WU-9` clearly reflects the docs IA cleanup split.
- [ ] Run: `mdbook build docs`
- [ ] Run: `./dev/check.sh`
- [ ] Commit with a small docs-only message.

## Follow-Up

Slice B is intentionally separate:

- add CMake docs targets
- move generated docs into `build/docs/...`
- update install/export to consume build-tree outputs

Plan complete and saved to `docs/superpowers/plans/2026-03-25-doc-information-architecture-slice-a.md`. Ready to execute?
