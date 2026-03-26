# G-code Syntax Requirements

This document collects the target parser-input requirements. It is for review
and planning, not just for describing what is already implemented.

For each syntax family, review should answer:

- what input forms must be accepted
- what input forms must be rejected
- what diagnostics are required
- what parse structure should be preserved
- what tests/fixtures must exist

## Review Status

Status values to use during review:

- `Wanted`
- `Reviewed`
- `Deferred`
- `Rejected`

Current state of this document:

- `Wanted`: initial consolidated checklist extracted from the current product spec (`../product/spec/index.md`)
- not yet fully reviewed item-by-item

## 1. Line and Program Structure

Status: `Reviewed` (2026-03-10 review pass)

Requirements:

- line endings: `\n` and `\r\n`
- optional block delete `/`
- optional skip levels `/0` .. `/9`
- optional `N` line number at block start
- words are case-insensitive
- support file/program metadata forms such as leading `%...`
- preserve comments as parse items where applicable
- support unquoted program/subprogram names using only `[A-Za-z0-9_]`
- document the unquoted-name restriction explicitly for users
- support quoted subprogram names/targets such as `"LIB/DRILL_CYCLE"`
- preserve quoted subprogram target text verbatim
- block length baseline: 512 characters per block, excluding line ending

Reviewed decisions:

- leading `%...` program metadata line is wanted
- Siemens-style underscore-heavy names are wanted through the unquoted-name
  rule `[A-Za-z0-9_]`
- quoted subprogram names/targets are wanted and may contain path-like text
  such as `"LIB/DRILL_CYCLE"`
- quoted path-like names are syntax-level preserved strings; later execution
  policy may interpret them as file/path-like targets
- block length baseline is 512 characters per block, excluding line ending
- skip-level syntax/behavior must be stated explicitly in requirements

Examples:

- valid: `%MAIN`
- valid: `%_N_MAIN_MPF`
- valid: `PROC MAIN`
- valid: `PROC _N_MAIN_SPF`
- valid: `PROC "LIB/DRILL_CYCLE"`
- invalid unquoted name examples if these are used as identifiers:
  - `PROC MAIN-PROG`
  - `PROC MAIN.PROG`

Open follow-up questions:

- exact accepted `%...` metadata character policy beyond the current baseline
  still needs a dedicated program-name review

## 2. Comments and Whitespace

Status: `Reviewed` (2026-03-10 review pass)

Requirements:

- `; ...`
- `(* ... *)`
- optional `// ...` mode gate
- whitespace/tabs should not change semantics beyond token separation
- `( ... )` is reserved for expressions/grouping, not for comments

Reviewed decisions:

- nested comments are rejected for now
- text after `;` is comment text to end of line
- parenthesized text inside `; ...` is just literal comment content
- standalone `( ... )` comment syntax is not part of the target requirement set
- in expressions/conditions, `( ... )` is reserved for grouping semantics, not
  comments

Examples:

- valid line comment:
  - `N0010 ;(CreateDate:Thu Aug 26 08:52:37 2021 by ZK)`
- valid grouping intent:
  - `IF (R1 == 1) AND (R2 > 10)`
- rejected as comment syntax target:
  - `(standalone comment)`

Open follow-up questions:

- whether `(* ... *)` remains wanted as a Siemens-style block-comment form
- whether `// ...` should remain optional or become required in some mode

## 3. Motion Commands

Status: `Reviewed` (2026-03-10 review pass)

Families to review:

- `G0`
- `G1`
- `G2`
- `G3`

Per-family review checklist:

- accepted address words
- optional vs required words
- invalid combinations
- modal-group membership
- parse output fields to preserve
- representative valid/invalid fixtures

Examples to review:

- `G1 X10 Y20 Z30 F100`
- `G1 AP=90 RP=10 F40`
- `G2 X10 Y20 I1 J2 F50`

Review note:

- command-family status is reviewed, but detailed accepted/rejected address
  combinations still need to be written explicitly in a follow-up pass

## 4. Dwell / Timing Commands

Status: `Reviewed` (2026-03-10 review pass)

Families to review:

- `G4`

Per-family review checklist:

- accepted dwell words
- mutually exclusive dwell modes
- block-shape restrictions
- parse output fields to preserve
- representative valid/invalid fixtures

Examples to review:

- `G4 F3`
- `G4 S30`

Review note:

- family status is reviewed; detailed value and block-shape constraints belong
  in the semantic requirements document and should be completed there

## 5. Modal/Mode Syntax Families

Status: `Reviewed` (2026-03-10 review pass)

Families to review:

- rapid mode: `RTLION`, `RTLIOF`
- tool radius compensation: `G40`, `G41`, `G42`
- working plane: `G17`, `G18`, `G19`
- dimensions/units/work offsets/feed mode if wanted later

This document only covers the text forms. Runtime meaning belongs in the
execution requirements document.

Review note:

- syntax-family scope is reviewed; per-family accepted text forms still need
  more explicit examples where the spellings differ materially

## 6. M Functions

Status: `Reviewed` (2026-03-10 review pass)

Requirements to review:

- `M<value>`
- extended forms such as `M<ext>=<value>`
- predefined Siemens baseline values wanted
- invalid extension forms
- parser diagnostics for malformed `M` words

Examples:

- `M3`
- `M30`
- `M17`

Review note:

- baseline family status is reviewed; detailed extension-shape rules still need
  to be written explicitly

## 7. Variables and Assignments

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
- parentheses are for grouping in expressions/conditions, not comments
- general function-call-style expressions are not wanted for now
- command-specific special forms, if any, should be reviewed separately instead
  of being treated as general expression syntax

## 8. Control Flow Syntax

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
- `AND` / `OR` / parentheses support?
- indirect/system-variable jump targets wanted?

Review note:

- family status is reviewed; the remaining questions should be converted into
  explicit accepted/rejected condition and target forms in a detailed pass

## 9. Subprogram Syntax

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

## 10. Tool Syntax

Status: `Reviewed` (2026-03-10 review pass)

Requirements to review:

- `T<number>`
- `T=<number>`
- `T<n>=<number>`
- management-on selector forms by location/name if wanted
- relationship to `M6`

Review note:

- family status is reviewed; mode-dependent selector shapes and `M6`
  interaction still need detailed semantic follow-up

## 11. Requirement Inventory Output

When review is complete, each family above should eventually carry:

- reviewed status
- canonical examples
- invalid examples
- owning tests/fixtures
- linked work unit IDs
