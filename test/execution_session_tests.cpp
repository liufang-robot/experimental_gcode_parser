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
    gcode::RuntimeResult<gcode::WaitToken> result;
    result.status = gcode::RuntimeCallStatus::Pending;
    result.wait_token = gcode::WaitToken{"motion", "line-1"};
    return result;
  }
};

TEST(ExecutionSessionTest, RejectedSuffixCanBeReplacedAndExecutionContinues) {
  RecordingSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::ExecutionSession session(sink, runtime, cancellation);

  ASSERT_TRUE(session.pushChunk("G1 X10\nG1 G2 X15\nG1 X20\n"));

  const auto first = session.finish();
  EXPECT_EQ(first.status, gcode::StepStatus::Progress);
  ASSERT_EQ(sink.linear_moves.size(), 1u);

  const auto rejected = session.pump();
  EXPECT_EQ(rejected.status, gcode::StepStatus::Rejected);
  ASSERT_TRUE(rejected.rejected.has_value());
  EXPECT_EQ(rejected.rejected->source.line, 2);
  EXPECT_EQ(session.rejectedLine(), 2);

  ASSERT_TRUE(session.replaceEditableSuffix("G1 X15\nG1 X20\n"));

  const auto resumed_line_2 = session.pump();
  EXPECT_EQ(resumed_line_2.status, gcode::StepStatus::Progress);

  const auto resumed_line_3 = session.pump();
  EXPECT_EQ(resumed_line_3.status, gcode::StepStatus::Progress);

  const auto completed = session.pump();
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

} // namespace
