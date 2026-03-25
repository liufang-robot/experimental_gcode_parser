# Documentation Information Architecture Slice A

## Goal

Make `docs/src/` the canonical home for project documentation and keep the repo
root documentation surface minimal.

## Scope

This slice includes:

- moving canonical root documentation into `docs/src/`
- making those pages visible in the generated mdBook
- shrinking root `AGENTS.md` into a short startup map
- adding explicit role-selection guidance for new agent sessions
- deleting migrated root markdown files except `README.md` and `AGENTS.md`

This slice does not include:

- CMake docs-generation targets
- moving generated docs into the build tree
- C++ standard upgrades
- changing `docs/superpowers/` workflow artifacts into mdBook content

## Canonical Documentation Rule

After this slice:

- canonical project docs live in `docs/src/`
- root `README.md` remains a top-level repository entry point
- root `AGENTS.md` remains a short agent startup entry point
- other root markdown files are removed after their content is moved

## Target Structure

The mdBook should become the normal reading path for project docs.

Suggested structure:

- `docs/src/index.md`
- `docs/src/product/`
  - `prd.md`
  - `spec.md`
  - `program_reference.md`
- `docs/src/project/`
  - `roadmap.md`
  - `backlog.md`
- `docs/src/development/`
  - `workflow.md`
  - `ooda.md`
  - `dual_agent_workflow.md`
  - `dual_agent_feature_checklist.md`
  - `gcode_text_flow.md`
- `docs/src/development/design/`
  - architecture/design pages
  - implementation-plan pages

The final structure may differ slightly if an existing subtree is a better fit,
but the key rule is:

- product/project/development topics should be separated into small pages
- canonical pages should be linked through `docs/src/SUMMARY.md`

## AGENTS.md Role

Root `AGENTS.md` should stop acting as the full repository encyclopedia.

It should contain only:

- repo purpose
- authoritative gate command
- a few hard rules
- required startup role selection
- links into canonical docs in `docs/src/`

## Agent Startup Rule

The short root `AGENTS.md` should explicitly say:

- before implementation work, the session must be acting as either:
  - `developer`
  - `integration tester`
- if the user has not already assigned the role, the agent should ask first

This rule should link to:

- `docs/src/development/dual_agent_workflow.md`
- `docs/src/development/dual_agent_feature_checklist.md`

## Page Size Rule

Documentation should prefer progressive disclosure:

- small entry pages
- topic indexes for larger areas
- split subpages when a topic becomes large

Avoid growing single markdown files into monoliths when a subtree would be more
readable.

## Follow-Up

Build-tree docs generation and CMake docs targets are intentionally deferred to
Slice B.
