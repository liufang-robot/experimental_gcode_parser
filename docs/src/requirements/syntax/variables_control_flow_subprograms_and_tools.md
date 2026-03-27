# Syntax: Variables, Control Flow, Subprograms, and Tools

## Variables and Assignments

Status: `Reviewed` (2026-03-10 review pass)

Requirements to review:

- user variables such as `R1`
- system variables such as `$P_ACT_X`
- selector forms such as `$A_IN[1]`, `$P_UIFR[1,X,TR]`
- assignment syntax:
  - `R1 = 2`
  - `R1 = R2 + 1`
  - `R1 = $P_ACT_X + 1`

Reviewed decisions:

- selector shapes required for now:
  - `$NAME[<int>]`
  - `$NAME[<int>,<ident>]`
  - `$NAME[<int>,<ident>,<ident>]`
- selector part rules for now:
  - first part must be a decimal integer
  - second and third parts, when present, must be identifiers
    `[A-Za-z0-9_]+`
- reject for now:
  - nested brackets
  - quoted strings in selectors
  - arithmetic expressions in selectors
  - empty selector parts
  - more than 3 selector parts
  - non-integer first selector part
- parenthesized subexpressions are wanted
- parentheses are for grouping in expressions and conditions, not comments
- general function-call-style expressions are not wanted for now
- command-specific special forms, if any, should be reviewed separately instead
  of being treated as general expression syntax

## Control Flow Syntax

Status: `Reviewed` (2026-03-10 review pass)

Families to review:

- labels
- `GOTO`, `GOTOF`, `GOTOB`, `GOTOC`
- `IF cond GOTO ... [ELSE GOTO ...]`
- structured `IF / ELSE / ENDIF`
- `WHILE / ENDWHILE`
- `FOR / ENDFOR`
- `REPEAT / UNTIL`
- `LOOP / ENDLOOP`

Review questions:

- full condition grammar wanted?
- `AND`, `OR`, and parentheses support?
- indirect or system-variable jump targets wanted?

Review note:

- family status is reviewed; the remaining questions should be converted into
  explicit accepted and rejected condition and target forms in a detailed pass

## Subprogram Syntax

Status: `Reviewed` (2026-03-10 review pass)

Families to review:

- direct subprogram call by name
- quoted-name subprogram call
- repeat count `P`
- `RET`
- `M17`
- ISO-compatible `M98 P...`
- `PROC` declaration forms

Examples:

- `THE_SHAPE`
- `"THE_SHAPE"`
- `THE_SHAPE P2`
- `P=2 THE_SHAPE`
- `RET`
- `M17`

Review note:

- family status is reviewed; declaration and call-shape details should be
  expanded from the already-reviewed naming rules above

## Tool Syntax

Status: `Reviewed` (2026-03-10 review pass)

Requirements to review:

- `T<number>`
- `T=<number>`
- `T<n>=<number>`
- management-on selector forms by location or name if wanted
- relationship to `M6`

Review note:

- family status is reviewed; mode-dependent selector shapes and `M6`
  interaction still need detailed semantic follow-up
