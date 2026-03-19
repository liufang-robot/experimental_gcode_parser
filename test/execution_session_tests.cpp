#include "gtest/gtest.h"

#include "gcode/execution_session.h"

namespace {

class RecordingSink : public gcode::IExecutionSink {
public:
  void onDiagnostic(const gcode::Diagnostic &diag) override {
    diagnostics.push_back(diag);
  }
  void onRejectedLine(const gcode::RejectedLineEvent &event) override {
    rejected_lines.push_back(event);
  }
  void onModalUpdate(const gcode::ModalUpdateEvent &) override {}
  void onLinearMove(const gcode::LinearMoveCommand &cmd) override {
    linear_moves.push_back(cmd);
  }
  void onArcMove(const gcode::ArcMoveCommand &) override {}
  void onDwell(const gcode::DwellCommand &) override {}
  void onToolChange(const gcode::ToolChangeCommand &) override {}

  std::vector<gcode::Diagnostic> diagnostics;
  std::vector<gcode::RejectedLineEvent> rejected_lines;
  std::vector<gcode::LinearMoveCommand> linear_moves;
};

class ReadyRuntime : public gcode::IRuntime {
public:
  gcode::RuntimeResult<gcode::WaitToken>
  submitLinearMove(const gcode::LinearMoveCommand &) override {
    gcode::RuntimeResult<gcode::WaitToken> result;
    result.status = gcode::RuntimeCallStatus::Ready;
    return result;
  }
  gcode::RuntimeResult<gcode::WaitToken>
  submitArcMove(const gcode::ArcMoveCommand &) override {
    gcode::RuntimeResult<gcode::WaitToken> result;
    result.status = gcode::RuntimeCallStatus::Ready;
    return result;
  }
  gcode::RuntimeResult<gcode::WaitToken>
  submitDwell(const gcode::DwellCommand &) override {
    gcode::RuntimeResult<gcode::WaitToken> result;
    result.status = gcode::RuntimeCallStatus::Ready;
    return result;
  }
  gcode::RuntimeResult<gcode::WaitToken>
  submitToolChange(const gcode::ToolChangeCommand &) override {
    gcode::RuntimeResult<gcode::WaitToken> result;
    result.status = gcode::RuntimeCallStatus::Ready;
    return result;
  }
  gcode::RuntimeResult<double> readSystemVariable(std::string_view) override {
    gcode::RuntimeResult<double> result;
    result.status = gcode::RuntimeCallStatus::Error;
    result.error_message = "not implemented";
    return result;
  }
  gcode::RuntimeResult<gcode::WaitToken>
  cancelWait(const gcode::WaitToken &) override {
    gcode::RuntimeResult<gcode::WaitToken> result;
    result.status = gcode::RuntimeCallStatus::Ready;
    return result;
  }
};

class StaticCancellation : public gcode::ICancellation {
public:
  bool isCancelled() const override { return cancelled; }
  bool cancelled = false;
};

class BlockingLinearRuntime : public ReadyRuntime {
public:
  gcode::RuntimeResult<gcode::WaitToken>
  submitLinearMove(const gcode::LinearMoveCommand &) override {
    ++linear_calls;
    gcode::RuntimeResult<gcode::WaitToken> result;
    result.status = gcode::RuntimeCallStatus::Pending;
    result.wait_token = gcode::WaitToken{"motion", "line-1"};
    return result;
  }

  gcode::RuntimeResult<gcode::WaitToken>
  cancelWait(const gcode::WaitToken &token) override {
    ++cancel_wait_calls;
    cancelled_token = token;
    return ReadyRuntime::cancelWait(token);
  }

  int linear_calls = 0;
  int cancel_wait_calls = 0;
  std::optional<gcode::WaitToken> cancelled_token;
};

class FirstMoveBlocksRuntime : public BlockingLinearRuntime {
public:
  gcode::RuntimeResult<gcode::WaitToken>
  submitLinearMove(const gcode::LinearMoveCommand &) override {
    ++linear_calls;
    if (linear_calls == 1) {
      gcode::RuntimeResult<gcode::WaitToken> result;
      result.status = gcode::RuntimeCallStatus::Pending;
      result.wait_token = gcode::WaitToken{"motion", "line-1"};
      return result;
    }
    return ReadyRuntime::submitLinearMove(gcode::LinearMoveCommand{});
  }
};

TEST(ExecutionSessionTest, RejectedSuffixCanBeReplacedAndExecutionContinues) {
  RecordingSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::ExecutionSession session(sink, runtime, cancellation);

  ASSERT_TRUE(session.pushChunk("G1 X10\nG1 G2 X15\nG1 X20\n"));

  const auto first = session.finish();
  EXPECT_EQ(first.status, gcode::StepStatus::Rejected);
  ASSERT_TRUE(first.rejected.has_value());
  EXPECT_EQ(first.rejected->source.line, 2);
  EXPECT_EQ(session.rejectedLine(), 2);
  ASSERT_EQ(sink.linear_moves.size(), 1u);

  ASSERT_TRUE(session.replaceEditableSuffix("G1 X15\nG1 X20\n"));

  auto completed = session.pump();
  while (completed.status == gcode::StepStatus::Progress) {
    completed = session.pump();
  }
  EXPECT_EQ(completed.status, gcode::StepStatus::Completed);

  ASSERT_EQ(sink.linear_moves.size(), 3u);
  ASSERT_TRUE(sink.linear_moves[0].target.x.has_value());
  ASSERT_TRUE(sink.linear_moves[1].target.x.has_value());
  ASSERT_TRUE(sink.linear_moves[2].target.x.has_value());
  EXPECT_EQ(*sink.linear_moves[0].target.x, 10.0);
  EXPECT_EQ(*sink.linear_moves[1].target.x, 15.0);
  EXPECT_EQ(*sink.linear_moves[2].target.x, 20.0);
}

TEST(ExecutionSessionTest, ReplaceEditableSuffixOnlyWorksInRejectedState) {
  RecordingSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::ExecutionSession session(sink, runtime, cancellation);

  EXPECT_FALSE(session.replaceEditableSuffix("G1 X1\n"));

  ASSERT_TRUE(session.pushChunk("G1 G2 X10\n"));
  const auto rejected = session.finish();
  EXPECT_EQ(rejected.status, gcode::StepStatus::Rejected);
  EXPECT_TRUE(session.replaceEditableSuffix("G1 X10\n"));
}

TEST(ExecutionSessionTest, CancelFromRejectedSetsCancelled) {
  RecordingSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::ExecutionSession session(sink, runtime, cancellation);

  ASSERT_TRUE(session.pushChunk("G1 G2 X10\n"));
  const auto rejected = session.finish();
  EXPECT_EQ(rejected.status, gcode::StepStatus::Rejected);

  session.cancel();
  EXPECT_EQ(session.state(), gcode::EngineState::Cancelled);
}

TEST(ExecutionSessionTest, BlockedSessionRejectsEditableSuffixReplacement) {
  RecordingSink sink;
  BlockingLinearRuntime runtime;
  StaticCancellation cancellation;
  gcode::ExecutionSession session(sink, runtime, cancellation);

  ASSERT_TRUE(session.pushChunk("G1 X1\n"));
  const auto blocked = session.finish();
  ASSERT_EQ(blocked.status, gcode::StepStatus::Blocked);
  EXPECT_EQ(session.state(), gcode::EngineState::Blocked);
  EXPECT_FALSE(session.replaceEditableSuffix("G1 X2\n"));
}

TEST(ExecutionSessionTest,
     BlockedSessionCanResumeAndContinueBufferedExecution) {
  RecordingSink sink;
  FirstMoveBlocksRuntime runtime;
  StaticCancellation cancellation;
  gcode::ExecutionSession session(sink, runtime, cancellation);

  ASSERT_TRUE(session.pushChunk("G1 X1\nG1 X2\n"));

  const auto blocked = session.finish();
  ASSERT_EQ(blocked.status, gcode::StepStatus::Blocked);
  ASSERT_TRUE(blocked.blocked.has_value());
  EXPECT_EQ(blocked.blocked->token.kind, "motion");
  EXPECT_EQ(blocked.blocked->token.id, "line-1");
  EXPECT_EQ(session.state(), gcode::EngineState::Blocked);
  ASSERT_EQ(sink.linear_moves.size(), 1u);

  auto step = session.resume(blocked.blocked->token);
  while (step.status == gcode::StepStatus::Progress) {
    step = session.pump();
  }

  EXPECT_EQ(step.status, gcode::StepStatus::Completed);
  EXPECT_EQ(session.state(), gcode::EngineState::Completed);
  ASSERT_EQ(sink.linear_moves.size(), 2u);
  ASSERT_TRUE(sink.linear_moves[0].target.x.has_value());
  ASSERT_TRUE(sink.linear_moves[1].target.x.has_value());
  EXPECT_EQ(*sink.linear_moves[0].target.x, 1.0);
  EXPECT_EQ(*sink.linear_moves[1].target.x, 2.0);
}

TEST(ExecutionSessionTest,
     CancelWhileBlockedCancelsWaitAndPumpReturnsCancelled) {
  RecordingSink sink;
  BlockingLinearRuntime runtime;
  StaticCancellation cancellation;
  gcode::ExecutionSession session(sink, runtime, cancellation);

  ASSERT_TRUE(session.pushChunk("G1 X1\n"));

  const auto blocked = session.finish();
  ASSERT_EQ(blocked.status, gcode::StepStatus::Blocked);
  ASSERT_TRUE(blocked.blocked.has_value());
  EXPECT_EQ(session.state(), gcode::EngineState::Blocked);

  session.cancel();

  EXPECT_EQ(session.state(), gcode::EngineState::Cancelled);
  EXPECT_EQ(runtime.cancel_wait_calls, 1);
  ASSERT_TRUE(runtime.cancelled_token.has_value());
  EXPECT_EQ(runtime.cancelled_token->kind, "motion");
  EXPECT_EQ(runtime.cancelled_token->id, "line-1");

  const auto cancelled = session.pump();
  EXPECT_EQ(cancelled.status, gcode::StepStatus::Cancelled);
}

TEST(ExecutionSessionTest, ForwardGotoFaultEmitsSingleDiagnostic) {
  RecordingSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::ExecutionSession session(sink, runtime, cancellation);

  ASSERT_TRUE(session.pushChunk("N10 GOTO END\nN20 G1 X10\n"));

  const auto faulted = session.finish();
  EXPECT_EQ(faulted.status, gcode::StepStatus::Faulted);
  EXPECT_EQ(session.state(), gcode::EngineState::Faulted);
  EXPECT_TRUE(sink.linear_moves.empty());

  ASSERT_EQ(sink.diagnostics.size(), 1u);
  EXPECT_EQ(sink.diagnostics[0].message, "unresolved goto target: END");
  EXPECT_EQ(sink.diagnostics[0].location.line, 1);
}

TEST(ExecutionSessionTest, ForwardGotoResolvesAcrossBufferedLines) {
  RecordingSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::ExecutionSession session(sink, runtime, cancellation);

  ASSERT_TRUE(
      session.pushChunk("N10 GOTO END\nN20 G1 X10\nEND:\nN30 G1 X20\n"));

  auto step = session.finish();
  while (step.status == gcode::StepStatus::Progress) {
    step = session.pump();
  }

  EXPECT_EQ(step.status, gcode::StepStatus::Completed);
  EXPECT_EQ(session.state(), gcode::EngineState::Completed);
  ASSERT_EQ(sink.linear_moves.size(), 1u);
  ASSERT_TRUE(sink.linear_moves[0].target.x.has_value());
  EXPECT_EQ(*sink.linear_moves[0].target.x, 20.0);
  EXPECT_TRUE(sink.diagnostics.empty());
}

TEST(ExecutionSessionTest, StructuredIfElseExecutesTrueBranchAcrossLines) {
  RecordingSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::ExecutionSession session(sink, runtime, cancellation);

  ASSERT_TRUE(
      session.pushChunk("R1=0\nIF R1 == 0\nG1 X10\nELSE\nG1 X20\nENDIF\n"));

  auto step = session.finish();
  while (step.status == gcode::StepStatus::Progress) {
    step = session.pump();
  }

  EXPECT_EQ(step.status, gcode::StepStatus::Completed);
  EXPECT_EQ(session.state(), gcode::EngineState::Completed);
  ASSERT_EQ(sink.linear_moves.size(), 1u);
  ASSERT_TRUE(sink.linear_moves[0].target.x.has_value());
  EXPECT_EQ(*sink.linear_moves[0].target.x, 10.0);
  EXPECT_TRUE(sink.diagnostics.empty());
}

} // namespace
