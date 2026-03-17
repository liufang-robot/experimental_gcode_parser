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

Step 1 covers only simple end states:

- `completed`
- `rejected`
- `faulted`

Step 2 will extend the suite to asynchronous cases such as `blocked`,
`resume`, and `cancelled`.

Cases that describe reviewed requirements but are not yet supported by the
public `ExecutionSession` path should live under `pending/`. They are reference
examples for future work, not enforced Step 1 fixtures.

Implementation note:

- Step 1 `.events.yaml` files use JSON-compatible YAML syntax so they remain
  readable while avoiding a new YAML dependency in the library code.
