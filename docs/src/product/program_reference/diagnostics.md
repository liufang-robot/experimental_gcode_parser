# Program Reference: Diagnostics and Invalid Programs

This page covers user-visible parser diagnostics and common invalid program
shapes.

## Parser Diagnostics Highlights

Implemented checks include:

- block length limit: 512 chars including LF
- invalid skip-level value (`/0.. /9` enforced)
- invalid or misplaced `N` address words
- duplicate `N` warning when line-number jumps are present
- `G4` block-shape checks
- assignment-shape baseline (`AP90` rejected; `AP=90`, `X1=10` accepted)

Use the requirements and SPEC pages for the broader diagnostic contract:

- [G-code Syntax Requirements](../../requirements/gcode_syntax_requirements.md)
- [G-code Semantic Validation Requirements](../../requirements/gcode_semantic_requirements.md)
- [SPEC: Diagnostics and Lowering](../spec/diagnostics_and_lowering.md)
