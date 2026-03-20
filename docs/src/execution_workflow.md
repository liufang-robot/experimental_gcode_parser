# Execution Workflow

This page answers the practical question:

> If I want to execute G-code with this library, what do I create, what
> interfaces do I implement, and what loop do I run?

The supported public execution entry point is:

- `ExecutionSession`

`StreamingExecutionEngine` still exists inside the implementation, but it is an
internal building block and not the intended integration surface.

## Public Headers

For execution, start with these public headers:

- `gcode/execution_commands.h`
- `gcode/execution_interfaces.h`
- `gcode/execution_runtime.h`
- `gcode/execution_session.h`

Use these additional headers only if you separately need syntax or IR access:

- `gcode/gcode_parser.h`
- `gcode/ast.h`
- `gcode/ail.h`

## What You Need To Implement

To execute G-code you usually provide three objects:

1. `IExecutionSink`
   - receives normalized commands and diagnostics
   - receives explicit modal-only updates through `onModalUpdate(...)`
2. `IRuntime` or `IExecutionRuntime`
   - performs slow or external work
3. `ICancellation`
   - tells the session whether execution should stop

## 1. Execution Sink

The sink receives normalized commands, not raw `G1 X10 Y20` words.

```cpp
#include "gcode/execution_interfaces.h"

class MySink : public gcode::IExecutionSink {
public:
  void onDiagnostic(const gcode::Diagnostic &diag) override {
    // Log or display diagnostics.
  }

  void onRejectedLine(const gcode::RejectedLineEvent &event) override {
    // Show a recoverable rejected-line error in the UI.
  }

  void onModalUpdate(const gcode::ModalUpdateEvent &event) override {
    // Observe modal-only changes such as G17/G18/G19 or G40/G41/G42.
  }

  void onLinearMove(const gcode::LinearMoveCommand &cmd) override {
    // Observe the normalized move command.
  }

  void onArcMove(const gcode::ArcMoveCommand &cmd) override {}
  void onDwell(const gcode::DwellCommand &cmd) override {}
  void onToolChange(const gcode::ToolChangeCommand &cmd) override {}
};
```

## 2. Runtime Interface

The runtime performs external work such as motion submission, dwell, tool
change, and system-variable access.

```cpp
class MyRuntime : public gcode::IRuntime {
public:
  gcode::RuntimeResult<gcode::WaitToken>
  submitLinearMove(const gcode::LinearMoveCommand &cmd) override;

  gcode::RuntimeResult<gcode::WaitToken>
  submitArcMove(const gcode::ArcMoveCommand &cmd) override;

  gcode::RuntimeResult<gcode::WaitToken>
  submitDwell(const gcode::DwellCommand &cmd) override;

  gcode::RuntimeResult<gcode::WaitToken>
  submitToolChange(const gcode::ToolChangeCommand &cmd) override;

  gcode::RuntimeResult<double>
  readSystemVariable(std::string_view name) override;

  gcode::RuntimeResult<gcode::WaitToken>
  cancelWait(const gcode::WaitToken &token) override;
};
```

If you also want one object that can handle richer language-aware evaluation,
implement `IExecutionRuntime` instead of plain `IRuntime`.

### Runtime-Backed G0/G1 Axis Words

The public execution path also supports scalar system variables in `G0/G1`
axis words with explicit `=` form, for example:

```gcode
G1 X=$P_ACT_X
G0 Z=$P_SET_Z
```

Execution behavior is:

- `ExecutionSession` resolves the scalar system-variable value through
  `IRuntime.readSystemVariable(...)`
- if the read is `Ready`, the sink/runtime receive a normal numeric
  `LinearMoveCommand`
- if the read is `Pending`, the session becomes `Blocked` before any
  `LinearMoveCommand` is emitted or submitted
- `resume(token)` re-evaluates that same motion instruction; it does not
  resubmit a previously accepted move

Current limits:

- supported only on `G0/G1`
- supported only on axis words `X/Y/Z/A/B/C`
- indexed selector forms like `$AA_IM[X]`, feed expressions, and arc-word
  expressions remain deferred

## 3. Cancellation Interface

```cpp
class MyCancellation : public gcode::ICancellation {
public:
  bool isCancelled() const override { return cancelled_; }
  bool cancelled_ = false;
};
```

## What Data You Receive

The session emits normalized command structs:

- `ModalUpdateEvent`
- `LinearMoveCommand`
- `ArcMoveCommand`
- `DwellCommand`
- `ToolChangeCommand`

Each command carries:

- `source`
  - file name if known
  - physical line number
  - `N...` block number if present
- command payload
  - modal changes, pose target, feed, arc parameters, dwell mode/value, or tool target
- `EffectiveModalSnapshot`
  - effective modal state attached to that command

That means your runtime does not need to reinterpret raw G-code text.

## StepResult Meanings

`ExecutionSession` methods return `StepResult`.

Important values:

- `Progress`
  - execution advanced, keep going
- `Blocked`
  - runtime accepted an async action and execution must wait
- `Rejected`
  - rejected line is recoverable
- `Completed`
  - execution finished successfully
- `Cancelled`
  - caller requested stop
- `Faulted`
  - unrecoverable failure

Important distinction:

- `Rejected`
  - recoverable by editing the rejected suffix
- `Faulted`
  - not recoverable through the normal edit-and-continue path

## Simple Use Case 1: Execute A Program

```cpp
#include "gcode/execution_session.h"

MySink sink;
MyRuntime runtime;
MyCancellation cancellation;

gcode::ExecutionSession session(sink, runtime, cancellation);

session.pushChunk("N10 G1 X10 Y20 F100\n");
session.pushChunk("N20 G4 F3\n");

gcode::StepResult step = session.finish();
while (step.status == gcode::StepStatus::Progress) {
  step = session.pump();
}

if (step.status == gcode::StepStatus::Blocked) {
  // Your runtime/planner completed the async action later.
  step = session.resume(step.blocked->token);
}
```

Use this when:

- you want the normal execution API
- you do not want to build your own low-level engine wrapper
- you may later need recovery behavior

## Simple Use Case 2: Feed Live Text Chunks

If input arrives in pieces, keep feeding chunks and pumping.

```cpp
session.pushChunk("N10 G1 X");
session.pushChunk("10 Y20 F100\n");

for (;;) {
  const auto step = session.pump();
  if (step.status == gcode::StepStatus::Progress) {
    continue;
  }
  break;
}
```

Rules:

- `pushChunk(...)`
  - only buffers/prepares more input
- `pump()`
  - advances execution using input already buffered
- `finish()`
  - says no more input is coming

## Simple Use Case 3: Halt-Fix-Continue Recovery

```cpp
session.pushChunk("G1 X10\nG1 G2 X15\nG1 X20\n");

gcode::StepResult step = session.finish();
while (step.status == gcode::StepStatus::Progress) {
  step = session.pump();
}

if (step.status == gcode::StepStatus::Rejected) {
  session.replaceEditableSuffix("G1 X15\nG1 X20\n");
  do {
    step = session.pump();
  } while (step.status == gcode::StepStatus::Progress);
}
```

Recovery rules:

- the rejected line and all later lines form the editable suffix
- the accepted prefix before the rejected line stays locked
- `replaceEditableSuffix(...)` replaces the editable suffix
- retry uses the stored rejected boundary automatically

## Simple Use Case 4: Asynchronous Runtime

Most external actions follow one pattern:

1. session emits a normalized command to the sink
2. session calls the runtime
3. runtime returns:
   - `Ready`
   - `Pending`
   - `Error`

Meaning:

- `Ready`
  - accepted and execution can continue now
- `Pending`
  - accepted by the runtime boundary, but execution must pause until you later
    call `resume(...)`
- `Error`
  - execution becomes `Faulted`

Important async contract details:

- `Pending` does not mean “call the same submission again later”
- once the runtime returns `Pending(token)`, the runtime side owns the wait for
  that command
- `resume(token)` means the external/runtime wait for that exact token is now
  satisfied
- `resume(token)` must continue the existing blocked execution state; it does
  not resubmit the original command
- readiness for a token is determined by the embedding runtime or run manager,
  not by the library
- `resume(token)` should be invoked by the thread that owns
  `ExecutionSession`
- `cancel()` is mainly intended for a session that is already `Blocked`, so the
  runtime can abort the pending work through `cancelWait(token)`

For a queue-backed runtime, a valid definition of “token satisfied” is:

- the runtime held the move because the downstream queue was full
- later, queue space became available
- the runtime successfully pushed the held move into that queue
- then the session-owning thread calls `resume(token)`

That means the API is a poor fit for long blocking inside
`submitLinearMove(...)`. A better integration shape is:

- return `Pending(token)` quickly
- manage the wait asynchronously in the runtime/run manager
- notify the session-owning thread when the token is ready to resume

The library does not provide that notification channel; it belongs to the
embedding application.

This pattern applies to:

- motion
- dwell
- tool change
- other runtime-managed actions

## Current Control-Flow Boundary

Today `ExecutionSession`:

- buffers the editable suffix as one executable program view
- handles motion/dwell/tool-change dispatch, async waiting, rejection
  recovery, modal carry-forward, and buffered control-flow resolution
- executes forward `GOTO`/label flows and structured `IF/ELSE/ENDIF` on the
  public path

Practical consequences:

- unresolved forward `GOTO`/branch targets can wait for later input before EOF
- structured `IF/ELSE/ENDIF` runs through `ExecutionSession`
- branch conditions in the baseline expression subset do not need
  `IExecutionRuntime`; the plain `IRuntime` adapter only reports condition
  resolution as unavailable when the condition falls outside the
  executor-owned supported subset

```cpp
const auto lowered = gcode::parseAndLowerAil(program_text);
gcode::AilExecutor exec(lowered.instructions);

while (exec.state().status == gcode::ExecutorStatus::Ready) {
  exec.step(0, sink, execution_runtime);
}
```

## Demo CLIs

Main public-workflow demo:

```bash
./build/gcode_exec_session --format debug testdata/execution/session_reject.ngc
```

Recover by replacing the editable suffix:

```bash
./build/gcode_exec_session --format debug \
  --replace-editable-suffix testdata/execution/session_fix_suffix.ngc \
  testdata/execution/session_reject.ngc
```

Internal engine-inspection demo:

```bash
./build/gcode_stream_exec --format debug testdata/execution/plane_switch.ngc
```

## Recommended Starting Point

If you are integrating this library into a controller:

1. implement `IExecutionSink`
2. implement `IRuntime`
3. create `ExecutionSession`
4. use:
   - `pushChunk(...)`
   - `pump()`
   - `finish()`
   - `resume(...)`
   - `cancel()`
5. add `replaceEditableSuffix(...)` only if you need recoverable operator-style
   correction after a rejected line
