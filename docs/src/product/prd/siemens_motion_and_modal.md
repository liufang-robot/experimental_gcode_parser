# PRD: Siemens Motion and Modal Domains

This page groups the major Siemens-aligned motion and modal requirements that
shape parser, AIL, and runtime design.

## 5.3 Siemens Comment Syntax

Required forms:

- `; ...`
- `(* ... *)`
- optional `// ...` behind explicit compatibility mode

Acceptance expectation:

- comment text preserved in parse output
- comments do not alter executable token order outside the comment span
- malformed or unclosed block comment returns actionable diagnostics

## 5.4 Siemens Tool-Change Semantics

Required behavior families:

- without tool management
  - direct-change mode
  - preselect plus `M6`
- with tool management
  - selection by location or name
  - substitution handled by runtime policy

Parser/runtime must preserve:

- raw command form
- resolved intent
- timing
- optional spindle extension
- selector kind

## 5.5 Tool Radius Compensation

Required modal group:

- Group 7
  - `G40`
  - `G41`
  - `G42`

Requirement:

- model Group 7 explicitly and track it independently from motion state

## 5.6 Working Plane Selection

Required commands:

- `G17`
- `G18`
- `G19`

Requirement:

- working plane must propagate into arc interpretation and compensation-related
  behavior

## 5.7 Feedrate Semantics

Required Group 15 coverage includes:

- `G93`
- `G94`
- `G95`
- `G96`
- `G97`
- `G931`
- `G961`
- `G971`
- `G942`
- `G952`
- `G962`
- `G972`
- `G973`

Required supporting forms:

- `F...`
- `FGROUP(...)`
- `FGREF[...]`
- `FL[...]`

Requirement:

- feed state must remain explicit and composable with motion, plane, and
  compensation state

## 5.8 Dimensions and Unit Semantics

Required coverage:

- Group 14:
  - `G90`
  - `G91`
- local overrides:
  - `AC(...)`
  - `IC(...)`
- rotary targeting:
  - `DC(...)`
  - `ACP(...)`
  - `ACN(...)`
- unit modes:
  - `G70`
  - `G71`
  - `G700`
  - `G710`
- turning diameter/radius families:
  - `DIAMON`
  - `DIAM90`
  - `DIAMOF`
  - related axis-specific and value-override forms

## 5.9 Settable Work Offsets

Required Group 8 coverage:

- `G500`
- `G54`
- `G55`
- `G56`
- `G57`
- `G505 .. G599`

Required suppress commands:

- `G53`
- `G153`
- `SUPA`

## 5.10 Exact-Stop and Continuous-Path Modes

Required modal groups:

- Group 10:
  - `G60`
  - `G64`
  - `G641`
  - `G642`
  - `G643`
  - `G644`
  - `G645`
- Group 11:
  - `G9`
- Group 12:
  - `G601`
  - `G602`
  - `G603`

Required parameter support:

- `ADIS=...`
- `ADISPOS=...`

## 5.11 Rapid Traverse

Required coverage:

- `G0`
- `RTLION`
- `RTLIOF`

Requirement:

- preserve rapid-mode state and effective override decisions explicitly

## 5.12 M-Code Syntax and Semantics

Required baseline predefined families include:

- flow control:
  - `M0`
  - `M1`
  - `M2`
  - `M17`
  - `M30`
- spindle:
  - `M3`
  - `M4`
  - `M5`
  - `M19`
  - `M70`
- tool:
  - `M6`
- gear:
  - `M40 .. M45`

Requirement:

- parsing preserves raw code, normalized identity, and source
- runtime semantics for machine-specific free M numbers must remain pluggable
