---
name: write-docs
description: Write or reorganize repository documentation using the project’s mdBook-first structure. Use when creating or editing docs, README, AGENTS, chapter indexes, requirement trees, or documentation IA, especially when deciding where content belongs, when to split a page, or how to keep docs concise and navigable.
---

# Write Docs

Use this skill when changing repository documentation structure or prose.

## Goal

Keep documentation aligned with the repository's mdBook-first information
architecture and the team's documentation taste.

Read first:

- `docs/src/development/docs_policy.md`
- `docs/src/index.md`
- `docs/src/SUMMARY.md`

## Core Rules

- Canonical docs live in `docs/src/`.
- Root markdown is limited to:
  - `README.md`
  - `AGENTS.md`
  - `CHANGELOG_AGENT.md`
- `AGENTS.md` is a startup map, not the full documentation body.
- Keep docs small and tree-structured.
- Prefer index pages plus focused child pages over large monoliths.
- Keep chapter ownership clear:
  - `requirements/` = review-first target behavior
  - `product/` = public-facing meaning and behavior
  - `development/` = workflow, testing, verification, architecture

## Placement Heuristics

Put content in `product/` when it explains:

- supported syntax
- semantics
- public behavior
- user-visible diagnostics
- usage examples

Put content in `development/` when it explains:

- workflow
- testing references
- verification commands
- architecture rationale
- implementation guidance

Put content in `requirements/` when it explains:

- what the project should support
- reviewed requirement decisions
- work-unit readiness or test-planning inputs

## Editing Rules

- Prefer the smallest effective change.
- Delete duplication instead of rewriting the same explanation twice.
- If a page becomes hard to scan in one pass, split it into a subtree.
- Keep index pages short and navigational.
- Update `docs/src/SUMMARY.md` whenever canonical navigation changes.
- Update `CHANGELOG_AGENT.md` for documentation changes.

## Validation

After docs changes, run:

- `python3 dev/lint_docs_policy.py`
- `mdbook build docs`
- `./dev/check.sh` for branch-final verification
