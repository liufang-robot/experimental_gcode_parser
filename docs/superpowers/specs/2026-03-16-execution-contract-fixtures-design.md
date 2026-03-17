# Execution Contract Fixtures Design

## Goal

Define a human-reviewable execution contract fixture system for the public
`ExecutionSession` API.

The fixture system must let humans review:

- input G-code
- approved reference event traces
- current runtime-produced event traces
- reference-vs-actual comparison in a web page

The fixture system must also let automated tests enforce exact equality between
reference and actual traces on every code change.

## Scope

This design covers **execution contract fixtures only**.

It does **not** cover:

- AST internal goldens
- AIL internal goldens
- internal lowering/intermediate outputs
- internal `StreamingExecutionEngine` integration surfaces

The public contract under test is the observable behavior of
`ExecutionSession`.

## Core Design

For each execution scenario, store:

1. persistent input G-code
2. persistent approved reference event trace
3. generated actual event trace from the current implementation
4. generated HTML comparison page

### Persistent source-of-truth artifacts

Stored in `testdata/execution_contract/`:

- `<case>.ngc`
- `<case>.events.yaml`

These files are human-reviewed and committed to git.

### Generated artifacts

Stored in `output/execution_contract_review/`:

- `<case>.actual.yaml`
- `<case>.html`
- `index.html`

These are generated from the current code and are not the source of truth.

## Event Trace Model

Expected output is an **ordered event trace**.

Initial event types:

- `diagnostic`
- `rejected`
- `modal_update`
- `linear_move`
- `arc_move`
- `dwell`
- `tool_change`
- `blocked`
- `completed`
- `cancelled`
- `faulted`

## Fixture Format

Each scenario uses two persistent source files.

### Input file

`<case>.ngc`

Plain G-code text.

### Reference trace file

`<case>.events.yaml`

YAML containing:

- case metadata
- explicit initial state
- ordered expected events

Example shape:

```yaml
name: goto_skips_middle_move
description: Forward goto skips the middle linear move and executes the label target.

initial_state:
  modal:
    motion_code: G1
    working_plane: xy
    rapid_mode: linear
    tool_radius_comp: off
    active_tool_selection: null
    pending_tool_selection: null
    selected_tool_selection: null

expected_events:
  - type: linear_move
    source:
      line: 4
      n: 30
    target:
      x: 20
      y: null
      z: null
      a: null
      b: null
      c: null
    feed: null
    effective:
      motion_code: G1
      working_plane: xy
      rapid_mode: linear
      tool_radius_comp: off
      active_tool_selection: null
      pending_tool_selection: null
      selected_tool_selection: null
  - type: completed
```

## Explicit Initial State

Every fixture must include explicit initial state.

Reason:

- modal-only updates are stored as changes only
- humans must know the starting modal context to review a trace correctly

Initial state is part of the public execution contract for the fixture.

## Modal Update Rule

Modal-only commands should emit explicit `modal_update` events.

Examples:

- `G17`
- `G18`
- `G19`
- `G40`
- `G41`
- `G42`
- `RTLION`
- `RTLIOF`

`modal_update` should contain:

- source information
- `changes`

It should **not** need to repeat the full modal snapshot if the fixture already
declares explicit initial state.

Example:

```yaml
- type: modal_update
  source:
    line: 1
  changes:
    working_plane: zx
```

Action events such as `linear_move` still carry the full `effective` modal
snapshot.

## Comparison Rule

Automated comparison between reference and actual is:

- exact semantic equality

Allowed normalization:

- key order normalization
- line ending normalization

Not allowed:

- fuzzy matching
- field omission tolerance
- semantic fallback matching

If the intended behavior changes, the reference fixture must be updated
explicitly.

## Web Review Site

The comparison site should be published together with the existing GitHub Pages
mdBook output, but as a generated static subsite.

Recommended path:

- `docs/book/execution-contract-review/index.html`

The mdBook remains the main docs site.
The execution-contract review becomes a generated companion subsite.

### Site content

Per case page:

- case name
- description
- link to `.ngc`
- link to reference `.events.yaml`
- link to generated `.actual.yaml`
- rendered G-code input
- rendered expected events
- rendered actual events
- match/mismatch summary
- human-readable diff section

Index page:

- suites
- case list
- short description
- status (`match` / `mismatch`)
- links to per-case pages

## Public Path Under Test

Fixture generation and automated comparison should use:

- `ExecutionSession`

The internal `StreamingExecutionEngine` must not be the primary subject of this
suite.

## Rollout Plan

### Step 1

Cover simpler end states first:

- `completed`
- `rejected`
- `faulted`
- plus ordinary emitted events such as:
  - `modal_update`
  - `linear_move`
  - `arc_move`
  - `dwell`
  - `tool_change`
  - `diagnostic`

Initial Step 1 case families:

1. modal-only update
2. simple linear move completed
3. dwell completed
4. tool change completed
5. rejected invalid line
6. unrecoverable fault case

Current implementation note:

- the fixture framework and supported baseline cases are in scope for Step 1
- cross-line control-flow examples (`goto_skips_line`, `if_else_branch`) remain
  under `testdata/execution_contract/pending/` until the public
  `ExecutionSession` path supports them directly

### Step 2

Add asynchronous behavior:

- `blocked`
- `resume`
- `cancelled`

## Non-Goals For Step 1

- asynchronous comparison semantics
- automatic “accept actual as new reference”
- internal IR fixture suites
- AST/AIL golden redesign
- mdBook plugin work

## Implementation Constraints

- source-of-truth fixtures remain human-readable
- actual output generation must be deterministic
- the comparison tool and HTML generator should not redefine execution logic;
  they should consume the same public session contract used by tests
- the automated test suite must be runnable from the normal repository quality
  gate
