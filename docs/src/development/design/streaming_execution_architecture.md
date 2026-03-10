# Streaming Execution Architecture

This note defines the target contract for a true line-by-line streaming
execution pipeline. It is an architecture-only slice intended to guide the
refactor away from whole-input parse/lower APIs.

## Problem Statement

Current `parseAndLowerStream(...)` behavior is callback-based delivery after the
entire input has already been parsed into a full `ParseResult`.

Current flow:

```text
whole input text -> parse() full AST -> lowerToMessagesStream() callbacks
```

Target flow:

```text
chunk input -> assemble complete line -> parse line -> lower line -> execute or block
```

The target pipeline must:

- accept arbitrary text chunks
- emit deterministic per-line execution events
- support blocking runtime operations
- support resume after blocking completes
- support cancellation while blocked or between lines
- preserve source attribution and deterministic diagnostics

## Scope

In scope for the refactor:

- line/chunk input handling
- streaming parse/lower/execute boundaries
- injected interfaces for sink/runtime/cancellation
- blocking and resume state machine
- event-log contract for integration tests

Out of scope for this note:

- parser grammar changes
- Siemens feature expansion
- transport-specific servo/planner internals
- coroutine/threading implementation details

## Public Direction

The new primary public execution API should be streaming-first.

Recommended high-level object:

```cpp
class StreamingExecutionEngine;
```

Recommended usage model:

1. construct engine with injected interfaces
2. feed text with `pushChunk(...)`
3. drive execution with `pump()`
4. if blocked, wait externally and call `resume(...)`
5. call `cancel()` for cooperative stop
6. call `finish()` after end-of-input

Batch APIs may remain temporarily as adapters during migration, but the target
public surface is incremental streaming.

## Core Data Model

Message and instruction payloads should remain plain value types. Runtime
behavior should be injected through interfaces.

Rationale:

- value types are simpler to serialize and diff
- closed message families map naturally to `std::variant`
- behavior boundaries belong in services, not payload objects

Therefore:

- keep `G1Message`, `G2Message`, `G4Message`, `AilLinearMoveInstruction`, etc.
  as structs
- inject execution/runtime behavior via abstract interfaces

## Injected Interfaces

### `IExecutionSink`

Observability boundary for deterministic event delivery.

```cpp
class IExecutionSink {
public:
  virtual ~IExecutionSink() = default;

  virtual void onDiagnostic(const gcode::Diagnostic &diag) = 0;
  virtual void onRejectedLine(const RejectedLineEvent &event) = 0;

  virtual void onLinearMove(const LinearMoveCommand &cmd) = 0;
  virtual void onArcMove(const ArcMoveCommand &cmd) = 0;
  virtual void onDwell(const DwellCommand &cmd) = 0;
  virtual void onControl(const ControlCommandEvent &event) = 0;
};
```

`IExecutionSink` is for logging, trace review, test capture, and optional
application-facing event delivery. It should not own blocking machine work.

### `IRuntime`

Slow or external machine-facing operations.

```cpp
class IRuntime {
public:
  virtual ~IRuntime() = default;

  virtual RuntimeResult<double> readVariable(std::string_view name) = 0;

  virtual RuntimeResult<double> readSystemVariable(std::string_view name) = 0;

  virtual RuntimeResult<void> writeVariable(std::string_view name,
                                            double value) = 0;

  virtual RuntimeResult<WaitToken>
  submitLinearMove(const LinearMoveCommand &cmd) = 0;

  virtual RuntimeResult<WaitToken> submitArcMove(const ArcMoveCommand &cmd) = 0;

  virtual RuntimeResult<WaitToken> submitDwell(const DwellCommand &cmd) = 0;

  virtual RuntimeResult<void> cancelWait(const WaitToken &token) = 0;
};
```

`IRuntime` handles operations that may complete immediately, block, or fail.

### `ICancellation`

Cooperative cancellation boundary.

```cpp
class ICancellation {
public:
  virtual ~ICancellation() = default;
  virtual bool isCancelled() const = 0;
};
```

Cancellation must be checked:

- before starting a new line
- after emitted diagnostics/events
- while blocked
- before accepting `resume(...)`

## Runtime Result Contract

Blocking must be explicit in return values.

```cpp
enum class RuntimeCallStatus {
  Ready,
  Pending,
  Error,
};

template <typename T>
struct RuntimeResult {
  RuntimeCallStatus status = RuntimeCallStatus::Error;
  std::optional<T> value;
  std::optional<WaitToken> wait_token;
  std::string error_message;
};
```

Rules:

- `Ready`: operation completed synchronously; `value` is available when needed
- `Pending`: engine must enter `Blocked` state and wait for `wait_token`
- `Error`: engine emits diagnostic/fault and stops current execution path

## Engine State Machine

Recommended top-level states:

```cpp
enum class EngineState {
  AcceptingInput,
  ReadyToExecute,
  Blocked,
  Completed,
  Cancelled,
  Faulted,
};
```

Blocked state should retain explicit context:

```cpp
struct WaitToken {
  std::string kind;
  std::string id;
};

struct BlockedState {
  int line = 0;
  WaitToken token;
  std::string reason;
};
```

Step result contract:

```cpp
enum class StepStatus {
  Progress,
  Blocked,
  Completed,
  Cancelled,
  Faulted,
};

struct StepResult {
  StepStatus status = StepStatus::Progress;
  std::optional<BlockedState> blocked;
  std::optional<gcode::Diagnostic> fault;
};
```

## Streaming Engine API

Recommended public API:

```cpp
class StreamingExecutionEngine {
public:
  StreamingExecutionEngine(IExecutionSink &sink, IRuntime &runtime,
                           ICancellation &cancellation);

  bool pushChunk(std::string_view chunk);
  StepResult pump();
  StepResult finish();
  StepResult resume(const WaitToken &token);
  void cancel();

  EngineState state() const;
};
```

Behavior summary:

- `pushChunk(...)`
  - appends raw bytes
  - extracts complete logical lines
  - does not itself block on runtime work
- `pump()`
  - processes available complete lines until one boundary is reached:
    - one line executes and returns progress
    - runtime blocks
    - cancellation triggers
    - fault occurs
    - no work remains
- `finish()`
  - marks end-of-input
  - flushes final unterminated line if allowed
  - returns `Completed` once all work is done
- `resume(...)`
  - resumes a previously blocked operation only when token matches current wait

## Per-Line Execution Contract

For each complete line:

1. parse the line into a line-level syntax/result form
2. emit parse diagnostics for that line
3. if line has an error diagnostic:
   - emit `onRejectedLine(...)`
   - enter `Faulted` or stop according to policy
4. lower the line using current modal/runtime context
5. emit execution event through `IExecutionSink`
6. call the required `IRuntime` operation
7. if runtime returns `Pending`, enter `Blocked`
8. if runtime returns `Ready`, continue
9. if runtime returns `Error`, emit diagnostic and fault

No subsequent line may execute while the engine is in `Blocked`.

## `G1` Contract

For a line containing `G1`:

1. line is parsed
2. line is lowered into a linear-motion command
3. `sink.onLinearMove(...)` is called with normalized parameters
4. `runtime.submitLinearMove(...)` is called with the same normalized command
5. if pending, engine returns `Blocked`
6. only after `resume(...)` may the next line proceed

Minimum command fields:

- `source`:
  - filename if known
  - physical line number
  - `N` number if present
- `opcode`:
  - `"G1"` or `"G0"` as applicable
- `target_pose`
- `feed`
- modal metadata needed by downstream consumers

## System-Variable Read Contract

Variable reads are runtime-resolved, not parser-owned.

For a line or condition using `R...` or `$...`:

1. parser preserves the variable reference in syntax/IR
2. execution requests value through:
   - `IRuntime::readVariable(...)` for `R...`
   - `IRuntime::readSystemVariable(...)` for `$...`
3. if ready, evaluation proceeds immediately
4. if pending, engine enters `Blocked`
5. if error, engine emits diagnostic/fault

This contract applies equally to:

- assignment expressions
- branch/condition evaluation
- richer execution-runtime embeddings that want to own expression semantics

Structured multi-term `AND` conditions can use the same runtime path when each
comparison term is preserved as structured AST. Parenthesized `AND` text that
is not yet lowered into structured comparison terms should use the richer
execution-runtime path; the plain `IRuntime` path faults with a targeted
diagnostic for that parser-limited form.
- Assignment expressions follow the same split: plain `IRuntime` supports the
  current simple numeric expression subset, while `IExecutionRuntime` can
  override expression evaluation for richer runtime semantics.
- future runtime-evaluated selector forms

## Cancellation Contract

Cancellation is cooperative and observable.

Rules:

- if `ICancellation::isCancelled()` becomes `true` before starting a new line,
  the engine transitions to `Cancelled`
- if cancelled while blocked, the engine calls `IRuntime::cancelWait(...)`
  best-effort and then transitions to `Cancelled`
- once cancelled, no further line execution occurs
- `resume(...)` after cancellation is invalid

## Event Log Contract for Integration Tests

The refactor should include a deterministic event-log sink for integration
tests. JSON Lines is the recommended format.

Each record should contain:

- `seq`: monotonically increasing event number
- `event`: stable event name
- source data when applicable
- normalized parameters

Recommended event names:

- `chunk_received`
- `line_completed`
- `diagnostic`
- `rejected_line`
- `sink.linear_move`
- `sink.arc_move`
- `sink.dwell`
- `runtime.read_system_variable`
- `runtime.read_system_variable.result`
- `runtime.submit_linear_move`
- `runtime.submit_arc_move`
- `runtime.submit_dwell`
- `engine.blocked`
- `engine.resumed`
- `engine.cancelled`
- `engine.completed`
- `engine.faulted`

Example:

```json
{"seq":1,"event":"line_completed","line":1,"text":"N10 G1 X10 Y20 F100"}
{
  "seq" : 2, "event" : "sink.linear_move",
                       "line" : 1,
                       "params"
      : {"opcode" : "G1", "x" : 10.0, "y" : 20.0, "feed" : 100.0}
}
{
  "seq" : 3, "event" : "runtime.submit_linear_move",
                       "line" : 1,
                       "params"
      : {"opcode" : "G1", "x" : 10.0, "y" : 20.0, "feed" : 100.0}
}
{
  "seq" : 4, "event" : "engine.blocked",
                       "line" : 1,
                       "token" : {"kind" : "motion", "id" : "m1"}
}
{
  "seq" : 5, "event" : "engine.resumed",
                       "token" : {"kind" : "motion", "id" : "m1"}
}
{"seq" : 6, "event" : "engine.completed"}
```

        ##Integration Test Structure

            Recommended new test files :

    - `test /
    streaming_execution_tests
        .cpp` - `test / streaming_cancellation_tests
                            .cpp` - `test /
                                        runtime_integration_tests.cpp`

                                        Recommended fixture directories :

    - `testdata / execution /` - `testdata / execution_logs /`

                                     Each integration test case should define:

- input G-code
- mock runtime script/config
- expected event log

## Migration Plan

### Phase 1

- add the new streaming engine and interface set
- add event-log sink and mock runtime
- keep current batch/stream APIs unchanged

### Phase 2

- adapt current `parseAndLowerStream(...)` to the new engine where possible
- add parity tests between old callback behavior and new engine behavior for
  supported motion-only flows

### Phase 3

- move downstream callers to `StreamingExecutionEngine`
- deprecate direct whole-input execution surfaces

### Phase 4

- remove legacy execution APIs only after parity and integration coverage are
  green

## Risks and Open Questions

- true line-by-line parsing may require a line-scoped parser entry point rather
  than current whole-program parse flow
- modal state and structured multi-line constructs must define whether they are
  syntax-validated incrementally or buffered until closing statements arrive
- `sink` event ordering must remain deterministic under blocking/resume paths
- line/chunk semantics must define whether CRLF normalization occurs before or
  during line assembly

## Recommended First Implementation Slice

Smallest coherent slice:

1. add the architecture interfaces and state types
2. add event-log sink and scripted mock runtime
3. implement motion-only `G0/G1/G2/G3/G4` line execution
4. add integration tests with blocking/resume/cancel coverage
5. leave variable-evaluation execution and structured control flow for follow-up
