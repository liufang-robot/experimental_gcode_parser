# OODA

## Objective
Run repository development as an explicit Observe-Orient-Decide-Act loop so work is directed, testable, and non-random.

## Loop

### 1) Observe
Collect current signals before choosing work:
- `git` state: branch, uncommitted changes, ahead/behind.
- CI status: latest workflow result and failing jobs.
- Test health: `./dev/check.sh` output.
- Spec drift: differences between implementation/tests and `SPEC.md`.
- Quality signals: diagnostics quality, parser failures, fuzz/crash findings.
- Open work: unblocked items at top of `BACKLOG.md`.

Required artifacts:
- Short observation summary in PR description or task notes.

### 2) Orient
Interpret observations against priorities:
- First priority: broken mainline, failing CI, crash/hang risk.
- Second priority: SPEC compliance gaps.
- Third priority: roadmap milestone progress.

Output:
- selected task ID(s) from `BACKLOG.md`
- explicit risks and assumptions

### 3) Decide
Choose one small slice with clear acceptance criteria.

Decision rules:
- Prefer smallest change that reduces highest risk.
- Avoid combining unrelated backlog items in one PR.
- If behavior changes, update SPEC in same PR.

Output:
- branch name (`feature/<task-or-scope>`)
- acceptance criteria checklist copied from backlog item

### 4) Act
Implement and validate.

Mandatory actions per feature PR:
- code changes
- tests in `test/`
- updates to `SPEC.md` when behavior changes
- update `CHANGELOG_AGENT.md`
- run `./dev/check.sh`

PR must include:
- evidence of passing `./dev/check.sh`
- list of new/updated tests
- list of SPEC sections covered

## Merge Policy
A PR can merge only if all are true:
- CI required checks pass.
- Acceptance criteria are satisfied.
- No unresolved P0/P1 concerns introduced by the PR.
- SPEC/tests/changelog requirements are satisfied.
- Scope is a single coherent backlog slice.

## Definition of Done
A task is done only when:
- Implementation merged to `main`.
- Tests proving behavior exist and pass.
- Regression test added for every bug fixed.
- PR links bug/issue identifier to regression test name(s).
- `BACKLOG.md` updated (task moved/marked done).
- `ROADMAP.md` updated if milestone status changed.

## Cadence
- Run OODA loop at least once per PR.
- Re-run Observe/Orient immediately when CI fails or new blocker appears.
