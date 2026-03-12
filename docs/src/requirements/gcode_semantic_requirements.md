# G-code Semantic Validation Requirements

This document collects non-syntax validation rules. These rules apply after
text has been parsed into a structured form, but before execution.

Use this document for questions like:

- the text parses, but is it allowed?
- is a numeric value in range?
- are these words legal together?
- should this produce a warning or an error?

## Review Status

Status values to use during review:

- `Wanted`
- `Reviewed`
- `Deferred`
- `Rejected`

Current state of this document:

- `Wanted`: initial checklist extracted from current parser/semantic behavior
- not yet fully reviewed item-by-item

## 1. Motion Command Validation

Status: `Reviewed` (2026-03-10 review pass)

Rules to review:

- `G1` must not mix Cartesian and polar endpoint modes in one block
- `G0` must not mix Cartesian and polar endpoint modes in one block, if that
  rule is desired consistently
- one conflicting motion-family command per block should produce a diagnostic
- arc-center words must match the effective working plane

Examples:

- valid: `G1 X10 Y20 F100`
- invalid: `G1 X10 AP=90 RP=10`
- invalid under `G17`: `G2 X1 Y2 K3`

Review note:

- family status is reviewed; detailed per-command invalid-combination rules
  still need to be expanded explicitly

## 2. Dwell Validation

Status: `Reviewed` (2026-03-10 review pass)

Rules to review:

- `G4` must be in a separate block
- `G4` requires exactly one dwell mode word
- `G4 F...` dwell value must be positive
- `G4 S...` dwell value must be positive
- non-numeric dwell values are invalid

Examples:

- valid: `G4 F3`
- valid: `G4 S30`
- invalid: `G4 F-3`
- invalid: `G4 F3 S30`
- invalid: `G4`
- invalid: `G4 X10`

Review note:

- family status is reviewed; exact diagnostic wording/severity still belongs in
  the diagnostics-policy follow-up

## 3. Variable and Assignment Validation

Status: `Reviewed` (2026-03-10 review pass)

Rules to review:

- allowed assignment left-hand sides
- invalid selector forms
- malformed expression structure
- unsupported parenthesized or function-like expression forms
- read-only vs writable variable categories, if enforced before execution

Examples:

- valid: `R1 = 2`
- valid: `R1 = R2 + 1`
- invalid: malformed selector syntax

Reviewed decisions:

- valid selector baseline:
  - `$NAME[1]`
  - `$NAME[1,X]`
  - `$NAME[1,X,TR]`
- parenthesized subexpressions are part of the target expression language
- general function-call-style expressions are not part of the baseline
  expression language for now
- invalid selector baseline:
  - nested brackets
  - quoted-string selector parts
  - arithmetic inside selector parts
  - empty selector parts
  - more than 3 selector parts
  - non-integer first selector part
- invalid expression baseline for now:
  - general function-call-style expressions such as `SIN(30)` or `ABS(R1)`

## 4. Control-Flow Validation

Status: `Reviewed` (2026-03-10 review pass)

Rules to review:

- malformed `IF/GOTO` shapes
- invalid jump target forms
- unsupported condition operators/forms
- duplicate or ambiguous labels / line-number targets
- loop-family parse-only vs executable-lowering diagnostics

Important distinction:

- unresolved forward targets during streaming input are usually execution-time
  or buffering concerns, not syntax concerns
- malformed target text is still a syntax/semantic issue

Review note:

- family status is reviewed; malformed-shape and invalid-target examples still
  need to be expanded into explicit accepted/rejected diagnostics

## 5. Subprogram Validation

Status: `Reviewed` (2026-03-10 review pass)

Rules to review:

- malformed `PROC` declarations
- malformed call shapes
- invalid repeat counts
- ISO-mode gated syntax/validation
- unsupported inline arguments/signatures
- unquoted subprogram names containing characters outside `[A-Za-z0-9_]`
  should be rejected with an explicit diagnostic
- block length over 512 characters should be rejected as a validation error

Review note:

- family status is reviewed; ISO-mode gating and repeat-count details still
  need explicit examples

## 6. Tool and Modal Validation

Status: `Reviewed` (2026-03-10 review pass)

Rules to review:

- invalid tool selector form by tool-management mode
- `M6` with no pending tool selection policy classification
- invalid modal family conflicts inside one block
- duplicate modal codes in one block: warning/error/allowed

Review note:

- family status is reviewed; duplicate-modal severity and tool-policy
  classification still need narrower rule text

## 7. Diagnostics Policy

Status: `Reviewed` (2026-03-10 review pass)

For each semantic rule, review should define:

- error vs warning
- exact source attribution
- fail-fast vs continue
- whether the line is rejected

Review note:

- policy status is reviewed; individual rule severities still need to be
  attached to specific semantic families over time

## 8. Classification Rule

Status: `Reviewed` (2026-03-10 review pass)

Use this split during review:

- Syntax requirement:
  - can the text be parsed into structured form?
- Semantic validation requirement:
  - the text parsed, but the command/value/combination is invalid
- Execution requirement:
  - what should happen for a valid command at runtime?

Example:

- `G4 F-3`
  - syntax: valid
  - semantic: invalid, dwell value must be positive
  - execution: not reached

## 9. Comment and Grouping Disambiguation

Status: `Reviewed` (2026-03-10 review pass)

Rules:

- `;` starts comment text to end of line
- parenthesized text after `;` is just part of the comment payload
- standalone `( ... )` is not a target comment form
- in expression/condition contexts, `( ... )` is reserved for grouping

Examples:

- valid comment:
  - `N0010 ;(CreateDate:Thu Aug 26 08:52:37 2021 by ZK)`
- valid grouping intent:
  - `IF (R1 == 1) AND (R2 > 10)`
