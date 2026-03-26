# Program Reference: Control Flow and Runtime

## AilExecutor Control-Flow Examples

These examples describe the current `AilExecutor` runtime contract, not the
current public `ExecutionSession` buffered-session behavior.

### `GOTO`

Input:

```gcode
N10 GOTO END
N20 G1 X10
END:
N30 G1 X20
```

Runtime effect in `AilExecutor`:

- parser lowers `GOTO END` and `END:` into control-flow instructions
- execution jumps to `END`
- `N20 G1 X10` is skipped
- only `N30 G1 X20` reaches the runtime as a move command

### `IF / ELSE / ENDIF`

Input:

```gcode
IF $P_ACT_X > 100
  G1 X0
ELSE
  G1 X10
ENDIF
```

Runtime effect in `AilExecutor`:

- the condition is evaluated at execution time, not parse time
- if `$P_ACT_X > 100`, only `G1 X0` reaches the runtime
- otherwise only `G1 X10` reaches the runtime

See [Execution Workflow](../execution_workflow.md) for the current
`ExecutionSession` boundary and when to drop down to `AilExecutor`.

## M Functions

Status:

- `Partial` (parse + validation + AIL emission + executor boundary policy)

Supported syntax:

- `M<value>`
- `M<address_extension>=<value>`

Validation:

- M value must be integer `0..2147483647`
- extended form must use equals syntax
- extended form is rejected for predefined stop/end families:
  - `M0`
  - `M1`
  - `M2`
  - `M17`
  - `M30`

AIL output:

- emits `m_function` instructions with:
  - source
  - value
  - optional address extension

Executor boundary behavior:

- known predefined Siemens M values are accepted and advanced without machine
  actuation in v0
- unknown M values follow executor policy:
  - `error`
  - `warning`
  - `ignore`

Current limitation:

- runtime machine-function execution mapping is not implemented in this slice

## Control Flow and Runtime

Implemented behavior:

- `GOTO` target search:
  - `GOTOF`: forward only
  - `GOTOB`: backward only
  - `GOTO`: forward then backward
  - `GOTOC`: same search as `GOTO`, unresolved target does not fault
- target kinds:
  - label
  - `N` line number
  - numeric line number
  - system-variable token target
- `AilExecutor` branch resolver contract:
  - injected resolver interface returns `true`, `false`, `pending`, or `error`
  - `pending` supports `wait_token` and retry timestamp
  - function/lambda overload remains as a compatibility adapter
- structured `IF/ELSE/ENDIF` lowering uses `branch_if` plus internal labels and
  gotos

Current limitations:

- loop-family statements remain parse-only
- live system-variable evaluation remains a runtime-resolver responsibility, not
  parser/lowering behavior
