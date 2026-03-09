#include <cmath>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "execution_interfaces.h"
#include "streaming_execution_engine.h"

namespace {

using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

bool closeEnough(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-9; }

class MockExecutionSink : public gcode::IExecutionSink {
public:
  MOCK_METHOD(void, onDiagnostic, (const gcode::Diagnostic &diag), (override));
  MOCK_METHOD(void, onRejectedLine, (const gcode::RejectedLineEvent &event),
              (override));
  MOCK_METHOD(void, onLinearMove, (const gcode::LinearMoveCommand &cmd),
              (override));
  MOCK_METHOD(void, onArcMove, (const gcode::ArcMoveCommand &cmd), (override));
  MOCK_METHOD(void, onDwell, (const gcode::DwellCommand &cmd), (override));
};

class MockRuntime : public gcode::IRuntime {
public:
  MOCK_METHOD(gcode::RuntimeResult<gcode::WaitToken>, submitLinearMove,
              (const gcode::LinearMoveCommand &cmd), (override));
  MOCK_METHOD(gcode::RuntimeResult<gcode::WaitToken>, submitArcMove,
              (const gcode::ArcMoveCommand &cmd), (override));
  MOCK_METHOD(gcode::RuntimeResult<gcode::WaitToken>, submitDwell,
              (const gcode::DwellCommand &cmd), (override));
  MOCK_METHOD(gcode::RuntimeResult<double>, readSystemVariable,
              (std::string_view name), (override));
  MOCK_METHOD(gcode::RuntimeResult<gcode::WaitToken>, cancelWait,
              (const gcode::WaitToken &token), (override));
};

class MockCancellation : public gcode::ICancellation {
public:
  MOCK_METHOD(bool, isCancelled, (), (const, override));
};

gcode::RuntimeResult<gcode::WaitToken> readyMove() {
  gcode::RuntimeResult<gcode::WaitToken> result;
  result.status = gcode::RuntimeCallStatus::Ready;
  return result;
}

TEST(StreamingExecutionGmockTest, G1CallsSinkThenRuntime) {
  MockExecutionSink sink;
  MockRuntime runtime;
  MockCancellation cancellation;
  gcode::LowerOptions options;
  options.filename = "job.ngc";

  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation, options);

  EXPECT_CALL(cancellation, isCancelled()).WillOnce(Return(false));

  {
    InSequence seq;
    EXPECT_CALL(sink, onDiagnostic(_)).Times(0);
    EXPECT_CALL(sink, onRejectedLine(_)).Times(0);
    EXPECT_CALL(sink, onLinearMove(_))
        .WillOnce(Invoke([](const gcode::LinearMoveCommand &cmd) {
          ASSERT_TRUE(cmd.source.filename.has_value());
          EXPECT_EQ(*cmd.source.filename, "job.ngc");
          EXPECT_EQ(cmd.source.line, 1);
          ASSERT_TRUE(cmd.source.line_number.has_value());
          EXPECT_EQ(*cmd.source.line_number, 10);
          EXPECT_EQ(cmd.opcode, "G1");
          ASSERT_TRUE(cmd.x.has_value());
          ASSERT_TRUE(cmd.y.has_value());
          ASSERT_TRUE(cmd.feed.has_value());
          EXPECT_TRUE(closeEnough(*cmd.x, 10.0));
          EXPECT_TRUE(closeEnough(*cmd.y, 20.0));
          EXPECT_TRUE(closeEnough(*cmd.feed, 100.0));
          ASSERT_TRUE(cmd.effective.working_plane.has_value());
          EXPECT_EQ(*cmd.effective.working_plane, gcode::WorkingPlane::XY);
          ASSERT_TRUE(cmd.effective.tool_radius_comp.has_value());
          EXPECT_EQ(*cmd.effective.tool_radius_comp,
                    gcode::ToolRadiusCompMode::Off);
        }));
    EXPECT_CALL(runtime, submitLinearMove(_))
        .WillOnce(Invoke([](const gcode::LinearMoveCommand &cmd) {
          EXPECT_EQ(cmd.opcode, "G1");
          EXPECT_TRUE(cmd.x.has_value());
          EXPECT_TRUE(cmd.y.has_value());
          EXPECT_TRUE(cmd.feed.has_value());
          if (cmd.x.has_value()) {
            EXPECT_TRUE(closeEnough(*cmd.x, 10.0));
          }
          if (cmd.y.has_value()) {
            EXPECT_TRUE(closeEnough(*cmd.y, 20.0));
          }
          if (cmd.feed.has_value()) {
            EXPECT_TRUE(closeEnough(*cmd.feed, 100.0));
          }
          EXPECT_TRUE(cmd.effective.working_plane.has_value());
          if (cmd.effective.working_plane.has_value()) {
            EXPECT_EQ(*cmd.effective.working_plane, gcode::WorkingPlane::XY);
          }
          return readyMove();
        }));
  }

  ASSERT_TRUE(engine.pushChunk("N10 G1 X10 Y20 F100\n"));
  const auto step = engine.pump();
  EXPECT_EQ(step.status, gcode::StepStatus::Progress);
  EXPECT_EQ(engine.state(), gcode::EngineState::ReadyToExecute);
}

TEST(StreamingExecutionGmockTest, G1PendingMoveBlocksEngine) {
  MockExecutionSink sink;
  MockRuntime runtime;
  MockCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  EXPECT_CALL(cancellation, isCancelled()).WillOnce(Return(false));
  EXPECT_CALL(sink, onDiagnostic(_)).Times(0);
  EXPECT_CALL(sink, onRejectedLine(_)).Times(0);
  EXPECT_CALL(sink, onLinearMove(_)).Times(1);
  EXPECT_CALL(runtime, submitLinearMove(_))
      .WillOnce(Invoke([](const gcode::LinearMoveCommand &) {
        gcode::RuntimeResult<gcode::WaitToken> result;
        result.status = gcode::RuntimeCallStatus::Pending;
        result.wait_token = gcode::WaitToken{"motion", "move-001"};
        return result;
      }));

  ASSERT_TRUE(engine.pushChunk("G1 X1 F2\n"));
  const auto step = engine.pump();
  ASSERT_EQ(step.status, gcode::StepStatus::Blocked);
  ASSERT_TRUE(step.blocked.has_value());
  EXPECT_EQ(step.blocked->line, 1);
  EXPECT_EQ(step.blocked->token.kind, "motion");
  EXPECT_EQ(step.blocked->token.id, "move-001");
  EXPECT_EQ(engine.state(), gcode::EngineState::Blocked);
}

TEST(StreamingExecutionGmockTest, ResumeAfterPendingMoveContinues) {
  MockExecutionSink sink;
  MockRuntime runtime;
  MockCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  EXPECT_CALL(cancellation, isCancelled()).WillOnce(Return(false));
  EXPECT_CALL(sink, onDiagnostic(_)).Times(0);
  EXPECT_CALL(sink, onRejectedLine(_)).Times(0);
  EXPECT_CALL(sink, onLinearMove(_)).Times(1);
  EXPECT_CALL(runtime, submitLinearMove(_))
      .WillOnce(Invoke([](const gcode::LinearMoveCommand &) {
        gcode::RuntimeResult<gcode::WaitToken> result;
        result.status = gcode::RuntimeCallStatus::Pending;
        result.wait_token = gcode::WaitToken{"motion", "move-002"};
        return result;
      }));

  ASSERT_TRUE(engine.pushChunk("G1 X1\n"));
  const auto blocked = engine.pump();
  ASSERT_EQ(blocked.status, gcode::StepStatus::Blocked);

  const auto resumed = engine.resume(gcode::WaitToken{"motion", "move-002"});
  EXPECT_EQ(resumed.status, gcode::StepStatus::Progress);
  EXPECT_EQ(engine.state(), gcode::EngineState::ReadyToExecute);
}

TEST(StreamingExecutionGmockTest, CancelWhileBlockedCallsRuntimeCancelWait) {
  MockExecutionSink sink;
  MockRuntime runtime;
  MockCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  EXPECT_CALL(cancellation, isCancelled()).WillOnce(Return(false));
  EXPECT_CALL(sink, onDiagnostic(_)).Times(0);
  EXPECT_CALL(sink, onRejectedLine(_)).Times(0);
  EXPECT_CALL(sink, onLinearMove(_)).Times(1);
  EXPECT_CALL(runtime, submitLinearMove(_))
      .WillOnce(Invoke([](const gcode::LinearMoveCommand &) {
        gcode::RuntimeResult<gcode::WaitToken> result;
        result.status = gcode::RuntimeCallStatus::Pending;
        result.wait_token = gcode::WaitToken{"motion", "move-003"};
        return result;
      }));
  EXPECT_CALL(runtime, cancelWait(_))
      .WillOnce(Invoke([](const gcode::WaitToken &token) {
        EXPECT_EQ(token.kind, "motion");
        EXPECT_EQ(token.id, "move-003");
        return readyMove();
      }));

  ASSERT_TRUE(engine.pushChunk("G1 X1\n"));
  const auto blocked = engine.pump();
  ASSERT_EQ(blocked.status, gcode::StepStatus::Blocked);

  engine.cancel();
  EXPECT_EQ(engine.state(), gcode::EngineState::Cancelled);
}

TEST(StreamingExecutionGmockTest, G4CallsSubmitDwell) {
  MockExecutionSink sink;
  MockRuntime runtime;
  MockCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  EXPECT_CALL(cancellation, isCancelled()).WillOnce(Return(false));
  EXPECT_CALL(sink, onDiagnostic(_)).Times(0);
  EXPECT_CALL(sink, onRejectedLine(_)).Times(0);
  EXPECT_CALL(sink, onDwell(_))
      .WillOnce(Invoke([](const gcode::DwellCommand &cmd) {
        EXPECT_EQ(cmd.dwell_mode, gcode::DwellMode::Seconds);
        EXPECT_TRUE(closeEnough(cmd.dwell_value, 3.0));
      }));
  EXPECT_CALL(runtime, submitDwell(_))
      .WillOnce(Invoke([](const gcode::DwellCommand &cmd) {
        EXPECT_EQ(cmd.dwell_mode, gcode::DwellMode::Seconds);
        EXPECT_TRUE(closeEnough(cmd.dwell_value, 3.0));
        return readyMove();
      }));

  ASSERT_TRUE(engine.pushChunk("G4 F3\n"));
  const auto step = engine.pump();
  EXPECT_EQ(step.status, gcode::StepStatus::Progress);
}

TEST(StreamingExecutionGmockTest,
     EffectiveModalStateTracksPlaneRapidAndToolComp) {
  MockExecutionSink sink;
  MockRuntime runtime;
  MockCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  EXPECT_CALL(cancellation, isCancelled())
      .Times(4)
      .WillRepeatedly(Return(false));
  EXPECT_CALL(sink, onDiagnostic(_)).Times(0);
  EXPECT_CALL(sink, onRejectedLine(_)).Times(0);
  EXPECT_CALL(sink, onLinearMove(_))
      .WillOnce(Invoke([](const gcode::LinearMoveCommand &cmd) {
        ASSERT_TRUE(cmd.effective.working_plane.has_value());
        EXPECT_EQ(*cmd.effective.working_plane, gcode::WorkingPlane::ZX);
        ASSERT_TRUE(cmd.effective.rapid_mode.has_value());
        EXPECT_EQ(*cmd.effective.rapid_mode,
                  gcode::RapidInterpolationMode::Linear);
        ASSERT_TRUE(cmd.effective.tool_radius_comp.has_value());
        EXPECT_EQ(*cmd.effective.tool_radius_comp,
                  gcode::ToolRadiusCompMode::Left);
      }));
  EXPECT_CALL(runtime, submitLinearMove(_)).WillOnce(Return(readyMove()));

  ASSERT_TRUE(engine.pushChunk("G18\nRTLION\nG41\nG1 X1\n"));
  EXPECT_EQ(engine.pump().status, gcode::StepStatus::Progress);
  EXPECT_EQ(engine.pump().status, gcode::StepStatus::Progress);
  EXPECT_EQ(engine.pump().status, gcode::StepStatus::Progress);
  EXPECT_EQ(engine.pump().status, gcode::StepStatus::Progress);
}

TEST(StreamingExecutionGmockTest, InvalidLineEmitsDiagnosticAndRejectedLine) {
  NiceMock<MockExecutionSink> sink;
  MockRuntime runtime;
  MockCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  EXPECT_CALL(cancellation, isCancelled()).WillOnce(Return(false));
  EXPECT_CALL(sink, onDiagnostic(_)).Times(testing::AtLeast(1));
  EXPECT_CALL(sink, onRejectedLine(_))
      .WillOnce(Invoke([](const gcode::RejectedLineEvent &event) {
        EXPECT_EQ(event.source.line, 1);
        ASSERT_FALSE(event.reasons.empty());
        EXPECT_EQ(event.reasons[0].severity,
                  gcode::Diagnostic::Severity::Error);
      }));
  EXPECT_CALL(runtime, submitLinearMove(_)).Times(0);
  EXPECT_CALL(runtime, submitArcMove(_)).Times(0);
  EXPECT_CALL(runtime, submitDwell(_)).Times(0);

  ASSERT_TRUE(engine.pushChunk("G1 G2 X10\n"));
  const auto step = engine.pump();
  EXPECT_EQ(step.status, gcode::StepStatus::Faulted);
  EXPECT_EQ(engine.state(), gcode::EngineState::Faulted);
}

} // namespace
