# AGENTS.md — CNC G-code parser library

This file is the startup map, not the full documentation body.

## Purpose
Implement a C++17 CNC G-code parser/execution library with:
- AST + diagnostics
- lowering into AIL
- public execution through `ExecutionSession`
- reviewed execution-contract fixtures

## Session Role
Before implementation work, the session must be acting as either:
- `developer`
- `integration tester`

If the user has not already assigned the role, ask first.

Role workflow details:
- `docs/src/development/dual_agent_workflow.md`
- `docs/src/development/dual_agent_feature_checklist.md`

## Command Gate
Authoritative local gate:

```bash
./dev/check.sh
```

If this is green, CI should be green.

## Hard Rules
- No new dependencies without asking.
- Public headers belong in `include/gcode/` only when they are part of the
  stable public surface.
- Every behavior change must update canonical docs in `docs/src/`.
- Every feature change must add a `CHANGELOG_AGENT.md` entry.
- Follow OODA before implementation work.
- Prefer small PRs and small diffs.

## Read Next
- `docs/src/index.md`
- `docs/src/requirements/index.md`
- `docs/src/product/index.md`
- `docs/src/project/backlog.md`
- `docs/src/project/roadmap.md`
- `docs/src/development/ooda.md`
- `docs/src/development/workflow.md`
- `docs/src/development/design/index.md`

If a topic grows, split it into smaller docs under `docs/src/` rather than
expanding this file.
