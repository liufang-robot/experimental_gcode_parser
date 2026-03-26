# PRD: Goals and Scope

## Review Guide

Suggested review order:

1. confirm scope and non-goals
2. confirm API constraints
3. review Siemens functional requirements by domain
4. mark each major subsection as:
   - `Approved`
   - `Needs Clarification`
   - `Out of Scope`

## Functional Scope Index

The old monolithic PRD Section 5 no longer appears as a single chapter; its content is now split across these pages:

- [Functional Scope Overview](functional_scope_overview.md)
- [Siemens Motion and Modal Domains](siemens_motion_and_modal.md)
- [Variables and Program Structure](variables_and_program_structure.md)

The major requirement areas moved from that old Section 5 are:

- comments
- tool change
- Group 7 tool radius compensation
- working plane
- Group 15 feed semantics
- dimensions and units
- work offsets
- exact-stop and continuous-path modes
- rapid traverse
- M-code semantics
- incremental parse/edit-resume
- system and user variables

## Acceptance Fixture

Reserved integrated acceptance scenario:

- `testdata/integration/simple_integrated_case.ngc`

Purpose:

- validate mixed modal, motion, and program-flow syntax in one realistic file
- verify deterministic parse/lower/runtime mapping

## 1. Product Purpose

Provide a production-usable C++ library that converts G-code text/files into:

- precise diagnostics
- typed lowering/runtime artifacts for downstream integrations

Primary users:

- CNC application and backend developers integrating parsing into planners,
  simulators, editors, and execution pipelines

## 2. Product Goals

- deterministic parsing and lowering
- actionable diagnostics
- stable integration surface
- test-first delivery with golden, regression, and fuzz-smoke coverage

## 3. Non-Goals

- executing machine motion directly
- full controller runtime semantics in the original v0 scope
- full macro and subprogram runtime completeness in the original v0 scope
