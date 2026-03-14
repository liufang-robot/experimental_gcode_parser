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
  - pose target, feed, arc parameters, dwell mode/value, or tool target
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
  - accepted, but execution must pause until you later call `resume(...)`
- `Error`
  - execution becomes `Faulted`

This pattern applies to:

- motion
- dwell
- tool change
- other runtime-managed actions

## Simple Use Case 5: `GOTO` Control Flow

Program:

```gcode
N10 GOTO END
N20 G1 X10
END:
N30 G1 X20
```

User code:

```cpp
gcode::ExecutionSession session(sink, runtime, cancellation);
session.pushChunk("N10 GOTO END\n");
session.pushChunk("N20 G1 X10\n");
session.pushChunk("END:\n");
session.pushChunk("N30 G1 X20\n");

gcode::StepResult step = session.finish();
while (step.status == gcode::StepStatus::Progress) {
  step = session.pump();
}
```

What the user handles:

- provide input text
- provide sink/runtime/cancellation
- keep calling `pump()` until execution stops at a boundary

What the parser/executor handles internally:

1. parse `GOTO END` as a goto statement
2. parse `END:` as a label
3. lower both into executable AIL control-flow instructions
4. execute `GOTO END`
5. jump the program counter to the `END` label
6. skip `N20 G1 X10`
7. execute only `N30 G1 X20`

What your runtime sees:

- one linear-move submission for `X20`
- no move submission for `X10`

Important streaming rule:

- if `GOTO END` arrives before `END:` has been received, execution waits for
  more input
- if `finish()` is called and `END:` still never appears, execution faults

## Simple Use Case 6: `IF / ELSE / ENDIF`

Program:

```gcode
IF $P_ACT_X > 100
  G1 X0
ELSE
  G1 X10
ENDIF
```

Example runtime for the system variable:

```cpp
class MyRuntime : public gcode::IRuntime {
public:
  gcode::RuntimeResult<double> readSystemVariable(std::string_view name) override {
    gcode::RuntimeResult<double> result;
    if (name == "$P_ACT_X") {
      result.status = gcode::RuntimeCallStatus::Ready;
      result.value = 120.0;
      return result;
    }
    result.status = gcode::RuntimeCallStatus::Error;
    result.error_message = "unknown variable";
    return result;
  }

  // submitLinearMove / submitArcMove / submitDwell / submitToolChange / cancelWait ...
};
```

User code:

```cpp
gcode::ExecutionSession session(sink, runtime, cancellation);
session.pushChunk(program_text);

gcode::StepResult step = session.finish();
while (step.status == gcode::StepStatus::Progress) {
  step = session.pump();
}
```

What the parser/executor handles internally:

1. parse `IF / ELSE / ENDIF` as structured control flow
2. lower the structure into executable branch/label-style AIL
3. evaluate `$P_ACT_X > 100` at execution time
4. if true, execute only `G1 X0`
5. if false, execute only `G1 X10`

If `$P_ACT_X = 120`, your runtime sees:

- one linear-move submission for `X0`

If `$P_ACT_X = 80`, your runtime sees:

- one linear-move submission for `X10`

Important rule:

- the user does not manually choose the branch
- the session/executor evaluates the condition and updates the program counter
- the runtime is only asked for primitive values or actions

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
