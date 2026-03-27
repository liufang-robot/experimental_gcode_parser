# Syntax: Line and Program Structure

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
- support quoted subprogram names and targets such as `"LIB/DRILL_CYCLE"`
- preserve quoted subprogram target text verbatim
- block length baseline: 512 characters per block, excluding line ending

Reviewed decisions:

- leading `%...` program metadata line is wanted
- Siemens-style underscore-heavy names are wanted through the unquoted-name
  rule `[A-Za-z0-9_]`
- quoted subprogram names and targets are wanted and may contain path-like text
  such as `"LIB/DRILL_CYCLE"`
- quoted path-like names are syntax-level preserved strings; later execution
  policy may interpret them as file/path-like targets
- block length baseline is 512 characters per block, excluding line ending
- skip-level syntax and behavior must be stated explicitly in requirements

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
