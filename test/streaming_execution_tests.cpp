#include "gtest/gtest.h"

#include "gcode/execution_interfaces.h"
#include "gcode/execution_runtime.h"
#include "gcode/streaming_execution_engine.h"

namespace {

class NullSink : public gcode::IExecutionSink {
public:
  void onDiagnostic(const gcode::Diagnostic &) override {}
  void onRejectedLine(const gcode::RejectedLineEvent &) override {}
  void onLinearMove(const gcode::LinearMoveCommand &) override {}
  void onArcMove(const gcode::ArcMoveCommand &) override {}
  void onDwell(const gcode::DwellCommand &) override {}
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

TEST(StreamingExecutionTest, PushChunkDoesNotExecuteUntilLineComplete) {
  NullSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("G1 X1"));
  const auto step = engine.pump();
  EXPECT_EQ(step.status, gcode::StepStatus::Progress);
  EXPECT_EQ(engine.state(), gcode::EngineState::ReadyToExecute);
}

TEST(StreamingExecutionTest, FinishFlushesFinalLine) {
  NullSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("G1 X1"));
  const auto step = engine.finish();
  EXPECT_EQ(step.status, gcode::StepStatus::Completed);
  EXPECT_EQ(engine.state(), gcode::EngineState::Completed);
}

TEST(StreamingExecutionTest, CrlfInputProducesSingleLogicalLine) {
  NullSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("G1 X1\r\n"));
  const auto step = engine.pump();
  EXPECT_EQ(step.status, gcode::StepStatus::Progress);
  EXPECT_EQ(engine.state(), gcode::EngineState::ReadyToExecute);
}

TEST(StreamingExecutionTest, NoNextLineExecutesWhileBlocked) {
  class BlockingRuntime : public ReadyRuntime {
  public:
    gcode::RuntimeResult<gcode::WaitToken>
    submitLinearMove(const gcode::LinearMoveCommand &) override {
      ++linear_calls;
      gcode::RuntimeResult<gcode::WaitToken> result;
      result.status = gcode::RuntimeCallStatus::Pending;
      result.wait_token = gcode::WaitToken{"motion", "blocked-1"};
      return result;
    }
    int linear_calls = 0;
  };

  NullSink sink;
  BlockingRuntime runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("G1 X1\nG1 X2\n"));
  const auto first = engine.pump();
  ASSERT_EQ(first.status, gcode::StepStatus::Blocked);
  EXPECT_EQ(runtime.linear_calls, 1);

  const auto second = engine.pump();
  EXPECT_EQ(second.status, gcode::StepStatus::Blocked);
  EXPECT_EQ(runtime.linear_calls, 1);
}

TEST(StreamingExecutionTest, FunctionExecutionRuntimeCanDriveStreamingEngine) {
  NullSink sink;
  StaticCancellation cancellation;
  gcode::FunctionExecutionRuntime runtime(
      [](const gcode::Condition &, const gcode::SourceInfo &) {
        gcode::ConditionResolution r;
        r.kind = gcode::ConditionResolutionKind::False;
        return r;
      },
      [](const gcode::LinearMoveCommand &) {
        gcode::RuntimeResult<gcode::WaitToken> result;
        result.status = gcode::RuntimeCallStatus::Ready;
        return result;
      },
      [](const gcode::ArcMoveCommand &) {
        gcode::RuntimeResult<gcode::WaitToken> result;
        result.status = gcode::RuntimeCallStatus::Ready;
        return result;
      },
      [](const gcode::DwellCommand &) {
        gcode::RuntimeResult<gcode::WaitToken> result;
        result.status = gcode::RuntimeCallStatus::Ready;
        return result;
      },
      [](std::string_view) {
        gcode::RuntimeResult<double> result;
        result.status = gcode::RuntimeCallStatus::Error;
        result.error_message = "not used";
        return result;
      },
      [](const gcode::WaitToken &) {
        gcode::RuntimeResult<gcode::WaitToken> result;
        result.status = gcode::RuntimeCallStatus::Ready;
        return result;
      });

  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("G1 X1\n"));
  const auto step = engine.finish();
  EXPECT_EQ(step.status, gcode::StepStatus::Completed);
  EXPECT_EQ(engine.state(), gcode::EngineState::Completed);
}

TEST(StreamingExecutionTest,
     FunctionExecutionRuntimeCanUseCombinedRuntimeOverload) {
  NullSink sink;
  StaticCancellation cancellation;
  gcode::FunctionExecutionRuntime runtime(
      [](const gcode::Condition &, const gcode::SourceInfo &) {
        gcode::ConditionResolution r;
        r.kind = gcode::ConditionResolutionKind::False;
        return r;
      },
      [](const gcode::LinearMoveCommand &) {
        gcode::RuntimeResult<gcode::WaitToken> result;
        result.status = gcode::RuntimeCallStatus::Ready;
        return result;
      },
      [](const gcode::ArcMoveCommand &) {
        gcode::RuntimeResult<gcode::WaitToken> result;
        result.status = gcode::RuntimeCallStatus::Ready;
        return result;
      },
      [](const gcode::DwellCommand &) {
        gcode::RuntimeResult<gcode::WaitToken> result;
        result.status = gcode::RuntimeCallStatus::Ready;
        return result;
      },
      [](std::string_view) {
        gcode::RuntimeResult<double> result;
        result.status = gcode::RuntimeCallStatus::Error;
        result.error_message = "not used";
        return result;
      },
      [](const gcode::WaitToken &) {
        gcode::RuntimeResult<gcode::WaitToken> result;
        result.status = gcode::RuntimeCallStatus::Ready;
        return result;
      });

  gcode::IExecutionRuntime &combined_runtime = runtime;
  gcode::StreamingExecutionEngine engine(sink, combined_runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("G1 X1\n"));
  const auto step = engine.finish();
  EXPECT_EQ(step.status, gcode::StepStatus::Completed);
  EXPECT_EQ(engine.state(), gcode::EngineState::Completed);
}

} // namespace
