# SPEC: Diagnostics and Lowering

## 4. Non-goals

- full modal-group validation beyond the current supported motion rule
- units and distance-mode completeness
- full RS274NGC expression/macro support
- full subprogram execution semantics in the original v0.1 scope

## 5. Diagnostics

- syntax errors are reported with 1-based line and column
- syntax messages should be actionable and prefixed with `syntax error:`
- semantic diagnostics include explicit correction hints

Current semantic error examples:

- mixed Cartesian and polar coordinates in one `G0/G1` line
- multiple motion commands in one line
- invalid `G4` block composition
- invalid dwell mode/value combinations

Planned Siemens compatibility diagnostics:

- malformed skip-level marker
- invalid assignment-shape diagnostics for Siemens `=` rules

## 6. Message Lowering

Current supported paths:

- `parse -> AIL`
- `parse -> AIL -> packets`
- `parse -> messages` as a compatibility surface

Legacy note:

- older batch message/session APIs remain for compatibility and tooling
- the public execution architecture is centered on `ExecutionSession`

### Result model

- diagnostics
- lowered instructions or packets/messages
- rejected lines for fail-fast policies

### JSON schema notes

- top-level `schema_version`
- ordered payload lists
- deterministic `diagnostics`
- deterministic `rejected_lines`

### Packet stage

Packet result shape:

- `packets`
- `diagnostics`
- `rejected_lines`

Current packetization converts motion and dwell AIL instructions. Non-motion
instructions are skipped with warning diagnostics where appropriate.

### 6.1 Control-Flow AIL and Executor

AIL may include:

- `label`
- `goto`
- `branch_if`

Structured control flow is lowered into explicit linear control flow with
generated labels and gotos.

Runtime/executor contract:

- branch resolution uses injected resolver interfaces
- condition evaluation may be:
  - `true`
  - `false`
  - `pending`
  - `error`
- variable references are preserved structurally for runtime resolution
- skip-level execution is applied at lowering/execution time from configured
  active levels

Executor state model:

- `ready`
- `blocked`
- `completed`
- `fault`

Target resolution behavior:

- `GOTOF`: forward-only
- `GOTOB`: backward-only
- `GOTO`: forward, then backward
- `GOTOC`: non-faulting unresolved-target variant

Simple system-variable goto targets are still unresolved in the v0 executor
model and follow the fault-or-continue behavior of the opcode.
