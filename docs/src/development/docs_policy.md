# Documentation Policy

This page records the repository's documentation structure rules and the
writing preferences the team expects agents to follow.

## Canonical Locations

Canonical project documentation lives in `docs/src/`.

Allowed root markdown files:

- `README.md`
- `AGENTS.md`
- `CHANGELOG_AGENT.md`

Rules:

- `README.md` is the repository entry page.
- `AGENTS.md` is the agent startup map, not the full docs body.
- `CHANGELOG_AGENT.md` is the agent-facing change record.
- All other canonical documentation belongs under `docs/src/`.

## mdBook Is Canonical

The mdBook under `docs/src/` is the canonical reading surface.

Rules:

- important docs should be reachable from `docs/src/SUMMARY.md`
- chapter order should reflect reading order, not repo history
- index pages should explain where to read next

Current intended top-level reading order:

1. `Requirements`
2. `Product Reference`
3. `Execution Workflow`
4. `Execution Contract Review`
5. `Development Reference`
6. `Project Planning`

## AGENTS.md Role

`AGENTS.md` must stay short and stable.

It should:

- explain the repo purpose briefly
- identify the command gate: `./dev/check.sh`
- require the session to act as either `developer` or `integration tester`
- tell the agent to ask if the role is not already clear
- point into `docs/src/` for the real documentation body

It should not become an encyclopedia.

## Tree Structure Preference

Documentation should stay small and tree-structured.

Rules:

- prefer short index pages plus focused child pages
- if a page grows too large or mixes several concerns, split it into a subtree
- avoid large monolithic pages when a topic naturally separates into domains

Recommended limits:

- `AGENTS.md` should remain short
- chapter index pages should remain short and navigational
- large content pages should be split once they become difficult to scan in one pass

## Chapter Ownership

Keep chapter ownership clear at a high level:

- `requirements/` = review-first target behavior
- `product/` = public-facing meaning and behavior
- `development/` = workflow, testing, verification, architecture

Avoid mixing maintainer-only material into public reference pages.

## Non-Redundancy

Prefer one canonical home for each topic.

Rules:

- do not duplicate the same explanation across multiple chapters
- move material to the section that owns the concern
- prefer deletion or a link over repeating the same framing twice

## Agent Guidance

Repo-local agent guidance for documentation lives in:

- `.agents/skills/write-docs/SKILL.md`

Use that skill for writing or reorganizing docs. Use the linter for mechanical enforcement.

## Generated Docs

Generated docs are outputs, not canonical sources.

Rules:

- checked-in source docs live under `docs/src/`
- generated HTML output is validated by `mdbook build docs`
- review sites and generated book output must not become the only source of truth
