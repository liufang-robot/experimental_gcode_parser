# Syntax: Motion, Modal, Dwell, and M Functions

## Motion Commands

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
- representative valid and invalid fixtures

Examples to review:

- `G1 X10 Y20 Z30 F100`
- `G1 AP=90 RP=10 F40`
- `G2 X10 Y20 I1 J2 F50`

Review note:

- command-family status is reviewed, but detailed accepted and rejected address
  combinations still need to be written explicitly in a follow-up pass

## Dwell and Timing Commands

Status: `Reviewed` (2026-03-10 review pass)

Families to review:

- `G4`

Per-family review checklist:

- accepted dwell words
- mutually exclusive dwell modes
- block-shape restrictions
- parse output fields to preserve
- representative valid and invalid fixtures

Examples to review:

- `G4 F3`
- `G4 S30`

Review note:

- family status is reviewed; detailed value and block-shape constraints belong
  in the semantic requirements document and should be completed there

## Modal and Mode Syntax Families

Status: `Reviewed` (2026-03-10 review pass)

Families to review:

- rapid mode: `RTLION`, `RTLIOF`
- tool radius compensation: `G40`, `G41`, `G42`
- working plane: `G17`, `G18`, `G19`
- dimensions, units, work offsets, and feed mode if wanted later

This page only covers the text forms. Runtime meaning belongs in the execution
requirements subtree.

Review note:

- syntax-family scope is reviewed; per-family accepted text forms still need
  more explicit examples where the spellings differ materially

## M Functions

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
