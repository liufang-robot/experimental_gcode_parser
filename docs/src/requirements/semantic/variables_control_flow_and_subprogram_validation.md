# Semantic: Variables, Control Flow, and Subprogram Validation

## Variable and Assignment Validation

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

## Control-Flow Validation

Status: `Reviewed` (2026-03-10 review pass)

Rules to review:

- malformed `IF/GOTO` shapes
- invalid jump target forms
- unsupported condition operators and forms
- duplicate or ambiguous labels / line-number targets
- loop-family parse-only vs executable-lowering diagnostics

Important distinction:

- unresolved forward targets during streaming input are usually execution-time
  or buffering concerns, not syntax concerns
- malformed target text is still a syntax and semantic issue

Review note:

- family status is reviewed; malformed-shape and invalid-target examples still
  need to be expanded into explicit accepted and rejected diagnostics

## Subprogram Validation

Status: `Reviewed` (2026-03-10 review pass)

Rules to review:

- malformed `PROC` declarations
- malformed call shapes
- invalid repeat counts
- ISO-mode gated syntax and validation
- unsupported inline arguments and signatures
- unquoted subprogram names containing characters outside `[A-Za-z0-9_]`
  should be rejected with an explicit diagnostic
- block length over 512 characters should be rejected as a validation error

Review note:

- family status is reviewed; ISO-mode gating and repeat-count details still
  need explicit examples

## Comment and Grouping Disambiguation

Status: `Reviewed` (2026-03-10 review pass)

Rules:

- `;` starts comment text to end of line
- parenthesized text after `;` is just part of the comment payload
- standalone `( ... )` is not a target comment form
- in expression and condition contexts, `( ... )` is reserved for grouping

Examples:

- valid comment:
  - `N0010 ;(CreateDate:Thu Aug 26 08:52:37 2021 by ZK)`
- valid grouping intent:
  - `IF (R1 == 1) AND (R2 > 10)`
