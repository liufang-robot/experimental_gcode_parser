# Documentation Editorial Cleanup Slice A.5 Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Clean up duplicated and overly long documentation after the canonical-doc move into `docs/src/`.

**Architecture:** First stabilize where canonical pages live, then improve clarity inside that structure. The cleanup should make the mdBook easier to scan without changing technical meaning.

**Tech Stack:** Markdown, mdBook, repo documentation structure.

---

### Task 1: Audit duplication and page size
- [ ] Identify duplicated explanations across `docs/src/`, `README.md`, and `AGENTS.md`.
- [ ] Identify oversized pages that should become indexes plus smaller children.
- [ ] Record a small list of canonical pages for each repeated topic.

### Task 2: Clean top-level entry pages
- [ ] Tighten `docs/src/index.md`.
- [ ] Tighten root `README.md`.
- [ ] Tighten root `AGENTS.md` without expanding it back into an encyclopedia.

### Task 3: Clean product/project/development pages
- [ ] Remove repeated explanations where a link is enough.
- [ ] Split large pages into subpages when needed.
- [ ] Keep navigation pages short.

### Task 4: Validate
- [ ] Run `mdbook build docs`.
- [ ] Run `./dev/check.sh` if the docs/build gate requires it.
- [ ] Review rendered navigation and page readability.

Plan complete and saved to `docs/superpowers/plans/2026-03-25-doc-editorial-cleanup-slice-a5.md`. This slice is intentionally deferred until after Slice A.
