# BACKLOG

## How To Use
- Agent picks from top to bottom by priority.
- Each task must include acceptance criteria before work starts.
- Completed tasks are moved to a done section in this file or linked from PR.

## Priority Legend
- `P0`: must fix now (blocker, broken CI, crash/data-loss/security risk)
- `P1`: high-value near-term
- `P2`: important but can wait
- `P3`: optional/enhancement

## Ready Queue
### T-022 (P2) mdBook documentation + GitHub Pages publishing
Why:
- Keep developer and program reference docs structured, versioned, and always
  aligned with implementation.
Scope:
- Add `docs/` mdBook source tree with at least:
  - development reference
  - program reference
- Ignore generated mdBook HTML output in git.
- Build docs in CI and publish docs to GitHub Pages from `main`.
- Update `SPEC.md` with mdBook documentation maintenance policy.
Acceptance criteria:
- `docs/book.toml` and `docs/src/*` exist with the required two sections.
- `.gitignore` excludes generated docs output directory.
- CI includes mdBook build step and Pages deployment flow.
- `SPEC.md` explicitly requires docs update on behavior/API changes.
Out of scope:
- Full rewrite of all existing markdown docs into mdBook pages.
SPEC Sections:
- Section 9 (Program/Documentation reference policy), Section 7 (CI/testing if needed).
Tests To Add/Update:
- CI docs build job (`mdbook build docs`).

## Icebox
- Coverage threshold policy and badge.
- Multi-file include/subprogram parsing (future SPEC).

## Task Template
Use this template for new backlog items:

- `ID`: T-XXX
- `Priority`: P0/P1/P2/P3
- `Title`:
- `Why`:
- `Scope`:
- `Acceptance Criteria`:
- `Out of Scope`:
- `SPEC Sections`:
- `Tests To Add/Update`:

## In Progress
(List tasks currently being worked on; only one assignee/task per PR)
- T-022 (feature/t022-mdbook-docs-and-pages)

## Done
(Move completed tasks here with PR link)
- T-010 (PR #17)
- T-009 (PR #16)
- T-008 (PR #15)
- T-013 (PR #14)
- T-012 (PR #13)
- T-007 (PR #3)
- T-006 (PR #3)
- T-005 (PR #12)
- T-004 (PR #11)
- T-003 (PR #10)
- T-002 (PR #9)
- T-001 (PR #8)
- T-011 (PR #6)
- T-014 (PR #20)
- T-015 (PR #21)
- T-016 (PR #22)
- T-017 (PR #23)
- T-018 (PR #25)
- T-019 (PR #26)
- T-020 (PR #27)
- T-021 (PR #28)
