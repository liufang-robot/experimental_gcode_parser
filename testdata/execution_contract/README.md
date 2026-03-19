# Execution Contract Fixtures

This directory stores source-of-truth fixtures for the public
`gcode::ExecutionSession` contract.

Each case has two persistent files:

- `<case>.ngc`
- `<case>.events.yaml`

Rules:

- `.ngc` is the input program text.
- `.events.yaml` is the approved ordered event trace.
- comparison is exact semantic equality against the generated actual trace.
- generated output does not overwrite these files automatically.

The enforced core suite covers:

- `completed`
- `rejected`
- `faulted`
- `blocked`
- `resume`

The current async Step 2 slice is limited to deterministic linear-move wait
cases driven by fixture-level `driver` steps plus `runtime.linear_move_results`.

`cancelled` remains a follow-up fixture workflow item and is currently covered
by direct public-session tests rather than persistent execution-contract review
fixtures.

Cases that describe reviewed requirements but are not yet supported by the
public `ExecutionSession` path should live under `pending/`. They are reference
examples for future work, not enforced core fixtures.

Implementation note:

- `.events.yaml` files use JSON-compatible YAML syntax so they remain readable
  while avoiding a new YAML dependency in the library code.
