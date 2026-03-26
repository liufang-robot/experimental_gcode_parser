# Requirements Subtree Split: Execution First

## Goal

Split the oversized execution requirements page into a small subtree with a
short index page and focused topical pages.

## Why This Slice

`docs/src/requirements/execution/index.md` is now the largest
requirements page and mixes multiple execution domains in one document. It is a
strong candidate for the next editorial split after the product-doc cleanup.

## Scope

This slice covers only the execution requirements chapter.

In scope:
- move `docs/src/requirements/execution/index.md` into an
  `execution/` subtree
- keep the current top-level requirements chapter structure:
  - syntax
  - semantic validation
  - execution
- preserve technical meaning while improving navigation and progressive
  disclosure

Out of scope:
- splitting syntax requirements
- splitting semantic requirements
- changing reviewed technical decisions
- changing requirement status values or work-unit semantics

## Target Shape

```text
docs/src/requirements/
  index.md
  gcode_syntax_requirements.md
  gcode_semantic_requirements.md
  execution/
    index.md
    modal_state_and_streaming.md
    runtime_boundaries_and_actions.md
    motion_and_timing.md
    variables_control_flow_and_calls.md
    tools_diagnostics_and_validation.md
```

## Placement Rules

- `requirements/` remains the review-first source of truth for what the project
  should support.
- index pages should stay short and navigational.
- each child page should stay focused on one execution domain family.
- requirement content should remain normative and review-oriented, not drift
  into implementation notes beyond what is already part of the reviewed
  contract.

## Split Strategy

Recommended grouping:

- `modal_state_and_streaming.md`
  - review status
  - modal state model
  - streaming execution model
- `runtime_boundaries_and_actions.md`
  - runtime interface boundaries
  - general async action-command pattern
- `motion_and_timing.md`
  - motion execution requirements
  - dwell / timing execution requirements
- `variables_control_flow_and_calls.md`
  - variable and expression runtime requirements
  - control-flow execution requirements
  - subprogram and call-stack requirements
- `tools_diagnostics_and_validation.md`
  - tool execution requirements
  - diagnostics and failure policy
  - test and validation requirements
  - work-unit readiness

## Acceptance

- `docs/src/requirements/index.md` links to the new execution subtree index.
- `docs/src/SUMMARY.md` exposes the new execution subtree pages.
- the old monolithic execution requirements page is removed.
- `mdbook build docs` succeeds.
- `./dev/check.sh` succeeds.

## Follow-Up

After this slice:
- split syntax requirements into a `requirements/syntax/` subtree if still too
  large
- split semantic requirements into a `requirements/semantic/` subtree if still
  too large
