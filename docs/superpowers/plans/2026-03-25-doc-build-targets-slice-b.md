# Documentation Build Targets Slice B Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Generate docs into the build tree and make install/export consume those generated outputs instead of source-tree generated content.

**Architecture:** Source docs remain under `docs/src/`, while generated outputs move to `build/docs/...`. CMake custom targets become the normal entry points for docs generation, and install/export copies from build outputs.

**Tech Stack:** CMake, mdBook, execution-contract review CLI, install/export rules.

---

### Task 1: Add docs generation targets
- [ ] Add CMake custom targets for mdBook docs and execution-contract review generation.
- [ ] Standardize build output directories under `build/docs/...`.

### Task 2: Update install/export
- [ ] Point install/export rules at build-tree docs outputs.
- [ ] Keep optional/strict install behavior explicit.

### Task 3: Update workflow docs
- [ ] Update developer workflow docs to use the new targets.
- [ ] Remove stale references to source-tree generated doc locations.

### Task 4: Validate
- [ ] Run the docs targets directly.
- [ ] Run `./dev/check.sh`.
- [ ] Verify install/export from the build-tree outputs.

Plan complete and saved to `docs/superpowers/plans/2026-03-25-doc-build-targets-slice-b.md`. Slice B is intentionally deferred until after Slice A.
