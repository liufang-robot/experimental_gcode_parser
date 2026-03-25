# Documentation Editorial Cleanup Slice A.5

## Goal

Improve documentation quality after the canonical move into `docs/src/` by
reducing duplication, tightening wording, and splitting oversized pages into
smaller topic trees.

## Scope

This slice should happen after Slice A.

It includes:

- removing duplicated explanations across mdBook pages
- shortening overly long pages
- splitting large topics into smaller pages or index subtrees
- tightening wording so pages are easier for humans and agents to scan
- preserving meaning while improving structure and clarity

It does not include:

- changing product behavior
- changing build-system docs generation targets
- moving generated docs into the build tree

## Editing Principles

- prefer one canonical explanation per topic
- replace duplication with links to the canonical page
- keep entry pages short and navigational
- split long pages before they become encyclopedic
- prefer concise wording over repetitive narrative
- preserve reviewed technical meaning while improving readability

## Review Focus

Primary review targets should include:

- `docs/src/index.md`
- `docs/src/product/`
- `docs/src/project/`
- `docs/src/development/`
- `docs/src/development/design/`
- root `README.md`
- root `AGENTS.md`

## Follow-Up

This slice may identify a later docs-gardening workflow or stale-doc linting
work, but that is not part of Slice A.5 itself.
