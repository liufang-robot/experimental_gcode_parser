# Semantic: Diagnostics Policy and Classification

## Diagnostics Policy

Status: `Reviewed` (2026-03-10 review pass)

For each semantic rule, review should define:

- error vs warning
- exact source attribution
- fail-fast vs continue
- whether the line is rejected

Review note:

- policy status is reviewed; individual rule severities still need to be
  attached to specific semantic families over time

## Classification Rule

Status: `Reviewed` (2026-03-10 review pass)

Use this split during review:

- Syntax requirement:
  - can the text be parsed into structured form?
- Semantic validation requirement:
  - the text parsed, but the command, value, or combination is invalid
- Execution requirement:
  - what should happen for a valid command at runtime?

Example:

- `G4 F-3`
  - syntax: valid
  - semantic: invalid, dwell value must be positive
  - execution: not reached
