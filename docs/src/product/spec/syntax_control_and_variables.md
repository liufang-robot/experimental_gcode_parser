# SPEC: Control Flow and Variables Syntax

## 3.6 Assignment Expressions

Statement form:

- `<lhs> = <expr>`

Current `lhs` support:

- `R`-style variables such as `R1`

Current expression operands:

- numeric literals
- variable words such as `R2`
- system variables such as `$P_ACT_X`

Current system-variable scope:

- simple token form such as `$P_ACT_X`
- single-selector form such as `$A_IN[1]` or `$AA_IM[X]`
- simple and single-selector forms are supported in assignment expressions and
  control-flow conditions
- multi-selector forms such as `$P_UIFR[1,X,TR]` are deferred

Supported operators:

- unary: `+`, `-`
- binary: `+`, `-`, `*`, `/`

Parenthesized subexpressions are not supported in v0 expression syntax.

Implemented Siemens baseline checks:

- `=` is required for multi-letter or numeric-extension address values
- single-letter plus single-constant omission remains accepted

### Planned extension

- structured variable references for richer Siemens forms
- preserved variable-reference structure in AIL
- fuller Siemens expression support

## 3.7 Control Flow Syntax

Supported goto directions:

- `GOTOF`
- `GOTOB`
- `GOTO`
- `GOTOC`

Supported targets:

- labels
- block numbers such as `N100`
- numeric targets such as `200`
- system-variable tokens for runtime indirection

Supported forms:

- `IF <condition> GOTOx <target>`
- optional `THEN`
- optional `ELSE GOTOx <target>`
- structured blocks:
  - `IF ... ELSE ... ENDIF`
  - `WHILE ... ENDWHILE`
  - `FOR ... TO ... ENDFOR`
  - `REPEAT ... UNTIL`
  - `LOOP ... ENDLOOP`

Supported condition operators:

- `==`
- `>`
- `<`
- `>=`
- `<=`
- `<>`

Supported logical composition:

- `AND` across condition terms
- parenthesized terms are preserved for runtime resolver handling

Notes:

- label names follow word-style identifiers and may include underscores
- duplicate `N` line numbers are accepted with warning
- parse stage is syntax-first; resolution happens later
- AIL preserves simple system-variable references in conditions and goto
  targets

Example:

```gcode
N100 G00 X10 Y10
GOTO END_CODE
N110 (This block is skipped)
N120 (This block is skipped)
END_CODE:
N130 G01 X20 Y20
```
