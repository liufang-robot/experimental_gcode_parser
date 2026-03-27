# Syntax: Comments and Whitespace

Status: `Reviewed` (2026-03-10 review pass)

Requirements:

- `; ...`
- `(* ... *)`
- optional `// ...` mode gate
- whitespace and tabs should not change semantics beyond token separation
- `( ... )` is reserved for expressions and grouping, not for comments

Reviewed decisions:

- nested comments are rejected for now
- text after `;` is comment text to end of line
- parenthesized text inside `; ...` is just literal comment content
- standalone `( ... )` comment syntax is not part of the target requirement set
- in expressions and conditions, `( ... )` is reserved for grouping semantics,
  not comments

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
