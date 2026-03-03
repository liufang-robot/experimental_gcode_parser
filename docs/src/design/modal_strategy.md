# Modal and Context Strategy

## Group-Oriented State Model

Modal/context behavior is organized by Siemens-aligned groups with selective
current implementation coverage.

Target registry model includes:
- Group 1: motion type family (`G0/G1/G2/G3` baseline)
- Group 6: working plane (`G17/G18/G19`)
- Group 7: compensation (`G40/G41/G42`)
- Group 8: settable offsets (`G500`, `G54..`)
- Group 10/11/12: exact-stop/continuous-path and criteria
- Group 13/14/15: unit/dimensions/feed families

Auxiliary context state:
- rapid interpolation control (`RTLION`/`RTLIOF`)
- block-scope suppress contexts (`G53/G153/SUPA`)

## Runtime Boundary

- Parse/semantic layers capture declarative modal/control intent.
- Runtime/executor applies dynamic policy-dependent behavior.
- Machine-specific behavior is intended to be config/policy-driven.

## Incremental Session Contract

`ParseSession` model goals:
- preserve immutable prefix before resume line
- reparse/re-lower suffix from resume line
- merge diagnostics and outputs deterministically

This is a summary of root `ARCHITECTURE.md` sections 6 and 6.1.
