---
name: write-docs
description: Write or reorganize repository documentation using the project’s mdBook-first structure. Use when creating or editing docs, README, AGENTS, chapter indexes, requirement trees, or documentation IA, especially when deciding how to split pages, preserve reading order, reduce duplication, and keep docs concise and navigable.
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

## What This Skill Owns

This skill is the repo's docs-authoring playbook.

Use it for:

- deciding the smallest clean docs change
- splitting large pages into subtrees
- choosing between `requirements/`, `product/`, and `development/`
- tightening reading order and navigation
- removing duplication
- keeping mdBook pages small and scannable

Keep the linter narrow and mechanical. Keep the detailed writing workflow here.

## Core Rules

- Canonical docs live in `docs/src/`.
- Root markdown is limited to:
  - `README.md`
  - `AGENTS.md`
  - `CHANGELOG_AGENT.md`
- `AGENTS.md` is a startup map, not the full documentation body.
- Prefer mdBook pages and navigation over repo-root markdown.
- Keep docs small and tree-structured.
- Prefer index pages plus focused child pages over large monoliths.
- Keep chapter ownership clear at a high level:
  - `requirements/` = review-first target behavior
  - `product/` = public-facing meaning and behavior
  - `development/` = workflow, testing, verification, architecture

## Working Sequence

When changing docs, follow this order:

1. Read the current entry points.
- Start with the page being edited.
- Check `docs/src/SUMMARY.md` to understand where the page sits in the book.
- Read the nearest chapter index before moving content across chapter boundaries.

2. Decide whether this is a placement change, a split, or a wording cleanup.
- Placement change: move content to the chapter that owns it.
- Split: replace one large page with an index plus focused children.
- Wording cleanup: shorten, deduplicate, or clarify without moving ownership.

3. Prefer the smallest effective change.
- Delete duplication instead of rewriting the same explanation twice.
- Move a paragraph before creating a new page.
- Split only when a page is hard to scan in one pass or mixes multiple concerns.

4. Update navigation last.
- Once page ownership is correct, update `docs/src/SUMMARY.md`.
- Then update the nearest chapter index page.
- Then update any entry-point docs that should link to the new location.

## Splitting a Large Page

When splitting a page:

- keep a short `index.md` as the subtree entry point
- let the index explain:
  - what the subtree is for
  - what pages exist
  - where to read next
- move detail into focused child pages
- remove the old monolith after links are updated

A good subtree usually has:

- one short index page
- a few child pages grouped by concern
- titles that describe the user's question, not just repo history

## Writing Patterns

### Index pages

Index pages should be short and navigational.

They should answer:

- what this section is for
- what the major subtopics are
- what order to read them in

They should not re-explain every detail from child pages.

### Topic pages

Topic pages should own one concern.

They should:

- state the point early
- group related bullets together
- avoid reopening material already explained in the parent index
- link out instead of re-explaining sibling topics

### AGENTS.md

Keep `AGENTS.md` stable and brief.

It should:

- identify the repo and command gate
- require role selection: `developer` or `integration tester`
- point to canonical docs in `docs/src/`

It should not absorb growing docs content.

## Non-Redundancy Rules

- one topic should have one canonical home
- if the same explanation appears twice, choose one owner and delete the other
- prefer a short link or pointer over duplicated framing
- avoid repeating maintainer workflow inside public reference pages

## Navigation Rules

Update these when structure changes:

- `docs/src/SUMMARY.md`
- the nearest chapter index page
- any entry-point page that should now send readers somewhere else

If navigation changes but `SUMMARY.md` does not, the work is usually incomplete.

## Tuning Guidance

This skill is expected to evolve.

Update it when the team learns new durable writing preferences about:

- when to split pages
- how index pages should look
- how to preserve progressive disclosure
- how to avoid duplication across chapters
- how to handle new recurring docs patterns

Do not push subjective prose preferences into the linter unless they become
clear mechanical repo rules.

## Validation

After docs changes, run:

- `python3 dev/lint_docs_policy.py`
- `mdbook build docs`
- `./dev/check.sh` for branch-final verification
