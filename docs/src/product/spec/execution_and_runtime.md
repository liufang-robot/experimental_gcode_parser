# SPEC: Execution and Runtime Contract

## 6.2 Streaming Execution Engine

Primary interfaces:

- execution sink
- runtime
- cancellation

### Per-line motion execution contract

- `G1` is normalized into a line-scoped linear-move command
- the engine emits the normalized command to the sink
- the engine submits the same command to the runtime
- runtime returns:
  - `ready`
  - `pending`
  - `error`

Blocked/resume rules:

- on `pending`, execution becomes blocked
- `pending` means the runtime accepted responsibility for the command
- `pending` does not mean the library should resubmit the same command later
- `resume(token)` means the wait for that exact token is already satisfied
- `resume(token)` continues the blocked state and must not resubmit the
  original command
- queue-backed runtimes should return `pending` quickly and resume only after
  the held command has actually been pushed downstream

### Runtime read contract

System-variable reads may return:

- ready value
- pending token
- error

Execution-contract fixtures support:

- fixed ready-valued `runtime.system_variables`
- ordered `runtime.system_variable_reads`

Ordered read scripts support per-attempt outcomes:

- `ready`
- `pending`
- `error`

Tracing rule:

- every read attempt emits a `system_variable_read` event
- if resume retries the same read, the retry emits a fresh read event

Supported runtime-backed public cases include:

- `if_system_variable_false_branch`
- `G1 X=$P_ACT_X`
- `G1 X=$AA_IM[X]`

Current name support:

- simple `$NAME`
- single-selector `$NAME[part]`

Deferred:

- multi-selector forms
- feed expressions
- arc-word expressions

### Public session status

`ExecutionSession` supports:

- buffered text input
- line-by-line parse/lower/execute
- explicit `finish`, `pump`, `resume`, and `cancel`
- editable suffix recovery after rejected lines

Public-session behavior currently includes:

- supported motion execution
- cross-line `GOTO`
- baseline structured `IF/ELSE/ENDIF`
- baseline system-variable conditions through runtime reads

### Execution-contract fixture baseline

Persistent public fixture files live under:

- `testdata/execution_contract/core/`

Observable fixture options include:

- `filename`
- `active_skip_levels`
- `tool_change_mode`
- `enable_iso_m98_calls`

Driver actions currently supported:

- `finish`
- `resume_blocked`
- `cancel_blocked`

Linear-move result scripting currently supports:

- `ready`
- `pending` with explicit wait token

System-variable read scripting currently supports:

- `outcome: ready` with `value`
- `outcome: pending` with `token`
- `outcome: error` with `message`

Reference-vs-actual comparison is exact semantic equality.

Generated actual traces are written under:

- `output/execution_contract_review/`

Generated review HTML can be published under:

- `docs/book/execution-contract-review/`

### Additional runtime boundaries

The current public execution model also includes explicit contracts for:

- M-function execution policy
- subprogram call and return boundaries
- tool selection and tool change behavior
- combined motion/runtime and condition/runtime integrations through
  `IExecutionRuntime`

The detailed architecture remains in the development design docs. This page is
the product-facing summary of the supported runtime contract.
