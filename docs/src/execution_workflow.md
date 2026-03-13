# Execution Workflow

The public execution-facing workflow is now:

1. parse and lower to AIL when you need executable IR
2. execute incrementally with `StreamingExecutionEngine`
3. use `ExecutionSession` when you need halt-fix-continue recovery after a
   rejected line

The older batch message/session APIs are internal tooling surfaces and are no
longer part of the supported public library model.

## Public Execution APIs

Primary public headers:

- `gcode/gcode_parser.h`
- `gcode/ast.h`
- `gcode/ail.h`
- `gcode/lowering_types.h`
- `gcode/execution_commands.h`
- `gcode/execution_interfaces.h`
- `gcode/execution_runtime.h`
- `gcode/streaming_execution_engine.h`
- `gcode/execution_session.h`

## Basic Streaming Execution

```cpp
#include "gcode/streaming_execution_engine.h"

MySink sink;
MyRuntime runtime;
MyCancellation cancellation;

gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);
engine.pushChunk("N10 G1 X10 Y20 F100\n");

for (;;) {
  const auto step = engine.finish();
  if (step.status == gcode::StepStatus::Progress) {
    continue;
  }
  if (step.status == gcode::StepStatus::Blocked) {
    engine.resume(step.blocked->token);
    continue;
  }
  break;
}
```

## Halt-Fix-Continue Recovery

`ExecutionSession` is the public recovery layer above
`StreamingExecutionEngine`.

```cpp
#include "gcode/execution_session.h"

MySink sink;
MyRuntime runtime;
MyCancellation cancellation;

gcode::ExecutionSession session(sink, runtime, cancellation);
session.pushChunk("G1 X10\nG1 G2 X15\nG1 X20\n");

auto step = session.finish();
while (step.status == gcode::StepStatus::Progress) {
  step = session.pump();
}

if (step.status == gcode::StepStatus::Rejected) {
  session.replaceEditableSuffix("G1 X15\nG1 X20\n");
  for (;;) {
    step = session.pump();
    if (step.status != gcode::StepStatus::Progress) {
      break;
    }
  }
}
```

Recovery rules:

- `Rejected` is recoverable
- `Faulted` is unrecoverable
- the rejected line and any later lines form the editable suffix
- the prefix before the rejected line remains locked in the normal recovery path

## Demo CLIs

Streaming event trace:

```bash
./build/gcode_stream_exec --format debug testdata/messages/g4_dwell.ngc
```

Execution-session recovery trace:

```bash
./build/gcode_exec_session --format debug \
  --replace-editable-suffix testdata/execution/session_fix_suffix.ngc \
  testdata/execution/session_reject.ngc
```
