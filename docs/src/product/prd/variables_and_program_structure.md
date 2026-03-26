# PRD: Variables and Program Structure

## 5.13 Incremental Parse/Edit-Resume

Primary workflow:

1. parse and stop at the first failing line under fail-fast policy
2. edit the failing line
3. resume from that line while preserving the accepted prefix

Expected session operations:

- `firstErrorLine()`
- `applyLineEdit(...)`
- `reparseFromLine(...)`
- `reparseFromFirstError()`

Requirement:

- prefix behavior remains stable
- resumed suffix diagnostics and outputs are deterministic

## 5.14 Variables

Required categories:

- user-defined variables such as `R1`
- system variables such as `$P_ACT_X`
- structured system-variable forms such as `$A_IN[1]` and
  `$P_UIFR[1,X,TR]`

Required usage forms:

- assignment
- expressions
- conditions

Behavior boundary:

- parse/lowering preserves variable references structurally
- runtime resolves values and access policy

Diagnostics must distinguish:

- parse-time syntax errors
- runtime access/protection failures
- runtime pending and unavailable outcomes

## Siemens Fundamental NC Syntax Principles

Required coverage:

- program naming
- block numbering and block length limits
- Siemens `=` rules
- comments
- skip blocks and skip levels

Behavior boundary:

- parse stage recognizes and preserves the structure
- runtime decides effective skip behavior

## Siemens Subprograms and Call Semantics

Required coverage:

- direct subprogram calls by name
- repeat counts with `P`
- optional ISO-compat `M98`
- `M17` and optional `RET`
- optional future parameterized declaration/call forms

Behavior boundary:

- parse/lowering preserves call/declaration structure
- runtime resolves targets and manages call stack behavior
