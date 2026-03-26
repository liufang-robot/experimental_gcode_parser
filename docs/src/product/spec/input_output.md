# SPEC: Input and Output

## 1. Purpose

Parse G-code text into a simple syntax-level model with diagnostics and
supported downstream lowering/execution paths.

Current implementation focus:

- parser and diagnostics
- AIL lowering
- packet/message compatibility surfaces
- public `ExecutionSession` execution for the supported motion subset

## 2. Input and Output

### 2.1 Input

- UTF-8 text
- line endings: `\n` or `\r\n`
- tabs are whitespace
- comments:
  - `; ...` to end of line
  - `( ... )` with no nesting
  - `(* ... *)` block comments
  - optional `// ...` when
    `ParseOptions.enable_double_slash_comments=true`

### 2.2 Output

The parser returns:

- AST (`program -> lines -> items`)
- diagnostics with 1-based line and column

Supported downstream stages:

- AST -> typed AIL instructions
- AIL -> motion packets
- public streaming execution through `ExecutionSession`

Normative execution-model contract:

- machine-executable semantics must be represented as explicit typed AIL
  instructions
- packet emission is an execution transport, not a requirement for every
  instruction
- non-motion control instructions may execute through runtime/control paths
  without standalone motion packets

Current primary execution API slice:

- line-by-line streaming execution engine under the public
  `ExecutionSession` surface
- injected execution sink, runtime, and cancellation interfaces
- explicit blocked, resumed, cancelled, rejected, faulted, and completed states
- current public execution coverage: `G0/G1/G2/G3/G4` plus the supported
  runtime-backed control/runtime subset

### CLI surface

`gcode_parse` supports:

- `--mode parse|ail|packet|lower`
- parse mode:
  - `--format debug`
  - `--format json`
- lower mode:
  - `--format json`
  - `--format debug`
- ail mode:
  - `--format json`
  - `--format debug`
- packet mode:
  - `--format json`
  - `--format debug`

### Installed public distribution

The project supports installation to a public prefix for downstream consumers
and hidden black-box validation.

Installed public surface:

- public headers under `include/gcode/`
- the public parser library target
- installed CLI binaries:
  - `gcode_parse`
  - `gcode_stream_exec`
  - `gcode_exec_session`
  - `gcode_execution_contract_review`
- exported CMake package metadata under `lib/cmake/gcode/`
- generated mdBook docs under `share/gcode/docs/` when built
- generated execution-contract review HTML under
  `share/gcode/execution-contract-review/` when built

Downstream consumers should use the installed prefix rather than repository
relative source paths.

### AST shape

- `Program`: ordered list of `Line`
- `Line`:
  - optional `block_delete`
  - optional `line_number`
  - ordered `items`
- `Word`:
  - `text`
  - `head`
  - optional `value`
  - `has_equal`
- `Comment`:
  - `text`
