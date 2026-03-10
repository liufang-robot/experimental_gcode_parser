#include "gtest/gtest.h"

#include "gcode/execution_interfaces.h"
#include "gcode/execution_runtime.h"
#include "gcode/streaming_execution_engine.h"

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

  std::vector<gcode::Diagnostic> diagnostics;
  std::vector<gcode::RejectedLineEvent> rejected_lines;
  std::vector<gcode::LinearMoveCommand> linear_moves;
};

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
  gcode::RuntimeResult<double> readVariable(std::string_view) override {
    gcode::RuntimeResult<double> result;
    result.status = gcode::RuntimeCallStatus::Error;
    result.error_message = "not implemented";
    return result;
  }
  gcode::RuntimeResult<void> writeVariable(std::string_view, double) override {
    gcode::RuntimeResult<void> result;
    result.status = gcode::RuntimeCallStatus::Ready;
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

class VariableRuntime : public ReadyRuntime {
public:
  gcode::RuntimeResult<double>
  readSystemVariable(std::string_view name) override {
    ++read_calls;
    last_name = std::string(name);
    gcode::RuntimeResult<double> result;
    result.status = gcode::RuntimeCallStatus::Ready;
    result.value = value;
    return result;
  }

  int read_calls = 0;
  double value = 0.0;
  std::string last_name;
};

class RecordingVariableRuntime : public ReadyRuntime {
public:
  gcode::RuntimeResult<double> readVariable(std::string_view name) override {
    ++read_calls;
    last_read_name = std::string(name);
    gcode::RuntimeResult<double> result;
    result.status = gcode::RuntimeCallStatus::Ready;
    result.value = read_value;
    return result;
  }

  gcode::RuntimeResult<double>
  readSystemVariable(std::string_view name) override {
    ++read_calls;
    last_read_name = std::string(name);
    gcode::RuntimeResult<double> result;
    result.status = gcode::RuntimeCallStatus::Ready;
    result.value = read_value;
    return result;
  }

  gcode::RuntimeResult<void> writeVariable(std::string_view name,
                                           double value) override {
    ++write_calls;
    last_write_name = std::string(name);
    last_write_value = value;
    gcode::RuntimeResult<void> result;
    result.status = gcode::RuntimeCallStatus::Ready;
    return result;
  }

  int read_calls = 0;
  int write_calls = 0;
  double read_value = 0.0;
  double last_write_value = 0.0;
  std::string last_read_name;
  std::string last_write_name;
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

TEST(StreamingExecutionTest, ResumeCompletesActiveExecutorAfterFinish) {
  class BlockingRuntime : public ReadyRuntime {
  public:
    gcode::RuntimeResult<gcode::WaitToken>
    submitLinearMove(const gcode::LinearMoveCommand &) override {
      ++linear_calls;
      gcode::RuntimeResult<gcode::WaitToken> result;
      result.status = gcode::RuntimeCallStatus::Pending;
      result.wait_token = gcode::WaitToken{"motion", "resume-1"};
      return result;
    }
    int linear_calls = 0;
  };

  NullSink sink;
  BlockingRuntime runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("G1 X1\n"));
  const auto blocked = engine.finish();
  ASSERT_EQ(blocked.status, gcode::StepStatus::Blocked);
  EXPECT_EQ(engine.state(), gcode::EngineState::Blocked);

  const auto resumed = engine.resume(gcode::WaitToken{"motion", "resume-1"});
  EXPECT_EQ(resumed.status, gcode::StepStatus::Progress);

  const auto completed = engine.pump();
  EXPECT_EQ(completed.status, gcode::StepStatus::Completed);
  EXPECT_EQ(engine.state(), gcode::EngineState::Completed);
  EXPECT_EQ(runtime.linear_calls, 1);
}

TEST(StreamingExecutionTest, ForwardGotoWaitsForMoreInputInsteadOfFaulting) {
  RecordingSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("GOTOF END\n"));
  const auto step = engine.pump();
  EXPECT_EQ(step.status, gcode::StepStatus::WaitingForInput);
  EXPECT_EQ(engine.state(), gcode::EngineState::WaitingForInput);
  ASSERT_TRUE(step.waiting.has_value());
  EXPECT_EQ(step.waiting->line, 1);
  EXPECT_NE(step.waiting->reason.find("future input"), std::string::npos);

  EXPECT_TRUE(sink.rejected_lines.empty());
  EXPECT_TRUE(sink.diagnostics.empty());
}

TEST(StreamingExecutionTest, ForwardGotoResolvesAfterLaterLabelArrives) {
  RecordingSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("GOTOF END\n"));
  ASSERT_EQ(engine.pump().status, gcode::StepStatus::WaitingForInput);

  ASSERT_TRUE(engine.pushChunk("G1 X1\nEND:\nG1 X2\n"));
  const auto resumed = engine.finish();
  EXPECT_EQ(resumed.status, gcode::StepStatus::Completed);
  EXPECT_EQ(engine.state(), gcode::EngineState::Completed);
  EXPECT_TRUE(sink.rejected_lines.empty());
  EXPECT_TRUE(sink.diagnostics.empty());
}

TEST(StreamingExecutionTest, FinishFaultsIfForwardGotoNeverResolves) {
  RecordingSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("GOTOF END\n"));
  ASSERT_EQ(engine.pump().status, gcode::StepStatus::WaitingForInput);

  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Faulted);
  EXPECT_EQ(engine.state(), gcode::EngineState::Faulted);
  ASSERT_FALSE(sink.diagnostics.empty());
  EXPECT_NE(sink.diagnostics.back().message.find("unresolved goto target"),
            std::string::npos);
}

TEST(StreamingExecutionTest, BranchIfWaitsForMoreInputUntilForwardLabelExists) {
  RecordingSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::FunctionExecutionRuntime combined_runtime(
      [](const gcode::Condition &, const gcode::SourceInfo &) {
        gcode::ConditionResolution r;
        r.kind = gcode::ConditionResolutionKind::True;
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
      [](std::string_view) {
        gcode::RuntimeResult<double> result;
        result.status = gcode::RuntimeCallStatus::Error;
        result.error_message = "not used";
        return result;
      },
      [](std::string_view, double) {
        gcode::RuntimeResult<void> result;
        result.status = gcode::RuntimeCallStatus::Ready;
        return result;
      },
      [](const gcode::WaitToken &) {
        gcode::RuntimeResult<gcode::WaitToken> result;
        result.status = gcode::RuntimeCallStatus::Ready;
        return result;
      });

  gcode::StreamingExecutionEngine engine(sink, combined_runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("IF R1 == 1 GOTOF TARGET\n"));
  ASSERT_EQ(engine.pump().status, gcode::StepStatus::WaitingForInput);

  ASSERT_TRUE(engine.pushChunk("G1 X1\nTARGET:\nG1 X2\n"));
  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Completed);
  ASSERT_EQ(sink.linear_moves.size(), 1u);
  ASSERT_TRUE(sink.linear_moves[0].x.has_value());
  EXPECT_DOUBLE_EQ(*sink.linear_moves[0].x, 2.0);
}

TEST(StreamingExecutionTest,
     PlainRuntimeResolvesSimpleSystemVariableCondition) {
  RecordingSink sink;
  VariableRuntime runtime;
  runtime.value = 1.0;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(
      engine.pushChunk("IF $P_ACT_X == 1 GOTOF END\nG1 X1\nEND:\nG1 X2\n"));
  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Completed);
  EXPECT_EQ(runtime.read_calls, 1);
  EXPECT_EQ(runtime.last_name, "$P_ACT_X");
  ASSERT_EQ(sink.linear_moves.size(), 1u);
  ASSERT_TRUE(sink.linear_moves[0].x.has_value());
  EXPECT_DOUBLE_EQ(*sink.linear_moves[0].x, 2.0);
}

TEST(StreamingExecutionTest, PlainRuntimeResolvesSimpleUserVariableCondition) {
  RecordingSink sink;
  RecordingVariableRuntime runtime;
  runtime.read_value = 1.0;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("IF R1 == 1 GOTOF END\nG1 X1\nEND:\nG1 X2\n"));
  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Completed);
  EXPECT_EQ(runtime.read_calls, 1);
  EXPECT_EQ(runtime.last_read_name, "R1");
  ASSERT_EQ(sink.linear_moves.size(), 1u);
  ASSERT_TRUE(sink.linear_moves[0].x.has_value());
  EXPECT_DOUBLE_EQ(*sink.linear_moves[0].x, 2.0);
}

TEST(StreamingExecutionTest,
     PlainRuntimeResolvesStructuredLogicalAndCondition) {
  RecordingSink sink;
  class MixedVariableRuntime : public RecordingVariableRuntime {
  public:
    gcode::RuntimeResult<double> readVariable(std::string_view name) override {
      ++read_calls;
      last_read_name = std::string(name);
      gcode::RuntimeResult<double> result;
      result.status = gcode::RuntimeCallStatus::Ready;
      result.value = 1.0;
      return result;
    }

    gcode::RuntimeResult<double>
    readSystemVariable(std::string_view name) override {
      ++read_calls;
      last_read_name = std::string(name);
      gcode::RuntimeResult<double> result;
      result.status = gcode::RuntimeCallStatus::Ready;
      result.value = 3.0;
      return result;
    }
  } runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk(
      "IF R1 == 1 AND $P_ACT_X > 2 GOTOF END\nG1 X1\nEND:\nG1 X2\n"));
  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Completed);
  EXPECT_EQ(runtime.read_calls, 2);
  ASSERT_EQ(sink.linear_moves.size(), 1u);
  ASSERT_TRUE(sink.linear_moves[0].x.has_value());
  EXPECT_DOUBLE_EQ(*sink.linear_moves[0].x, 2.0);
}

TEST(StreamingExecutionTest,
     PlainRuntimeParenthesizedLogicalAndRequiresExecutionRuntime) {
  RecordingSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk(
      "IF (R1 == 1) AND (R2 > 10) GOTOF END\nG1 X1\nEND:\nG1 X2\n"));
  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Faulted);
  ASSERT_FALSE(sink.diagnostics.empty());
  EXPECT_NE(sink.diagnostics.back().message.find("IExecutionRuntime"),
            std::string::npos);
}

TEST(StreamingExecutionTest,
     PlainRuntimeSystemVariableConditionCanBlockAndResume) {
  class PendingVariableRuntime : public ReadyRuntime {
  public:
    gcode::RuntimeResult<double>
    readSystemVariable(std::string_view name) override {
      ++read_calls;
      last_name = std::string(name);
      gcode::RuntimeResult<double> result;
      if (read_calls == 1) {
        result.status = gcode::RuntimeCallStatus::Pending;
        result.wait_token = gcode::WaitToken{"variable", "p_act_x"};
        return result;
      }
      result.status = gcode::RuntimeCallStatus::Ready;
      result.value = 1.0;
      return result;
    }

    int read_calls = 0;
    std::string last_name;
  };

  RecordingSink sink;
  PendingVariableRuntime runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(
      engine.pushChunk("IF $P_ACT_X == 1 GOTOF END\nG1 X1\nEND:\nG1 X2\n"));
  const auto blocked = engine.pump();
  ASSERT_EQ(blocked.status, gcode::StepStatus::Blocked);
  ASSERT_TRUE(blocked.blocked.has_value());
  EXPECT_EQ(blocked.blocked->token.kind, "variable");
  EXPECT_EQ(blocked.blocked->token.id, "p_act_x");

  const auto resumed = engine.resume(gcode::WaitToken{"variable", "p_act_x"});
  EXPECT_EQ(resumed.status, gcode::StepStatus::Progress);

  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Completed);
  EXPECT_EQ(runtime.read_calls, 2);
  EXPECT_EQ(runtime.last_name, "$P_ACT_X");
  ASSERT_EQ(sink.linear_moves.size(), 1u);
  ASSERT_TRUE(sink.linear_moves[0].x.has_value());
  EXPECT_DOUBLE_EQ(*sink.linear_moves[0].x, 2.0);
}

TEST(StreamingExecutionTest, PlainRuntimeExecutesSimpleAssignmentWrite) {
  RecordingSink sink;
  RecordingVariableRuntime runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("R1 = 2\n"));
  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Completed);
  EXPECT_EQ(runtime.write_calls, 1);
  EXPECT_EQ(runtime.last_write_name, "R1");
  EXPECT_DOUBLE_EQ(runtime.last_write_value, 2.0);
  EXPECT_TRUE(sink.linear_moves.empty());
}

TEST(StreamingExecutionTest,
     PlainRuntimeAssignmentCanReadSystemVariableAndWriteResult) {
  RecordingSink sink;
  RecordingVariableRuntime runtime;
  runtime.read_value = 7.5;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("R1 = $P_ACT_X + 1.5\n"));
  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Completed);
  EXPECT_EQ(runtime.read_calls, 1);
  EXPECT_EQ(runtime.last_read_name, "$P_ACT_X");
  EXPECT_EQ(runtime.write_calls, 1);
  EXPECT_EQ(runtime.last_write_name, "R1");
  EXPECT_DOUBLE_EQ(runtime.last_write_value, 9.0);
}

TEST(StreamingExecutionTest,
     PlainRuntimeAssignmentCanReadUserVariableAndWriteResult) {
  RecordingSink sink;
  RecordingVariableRuntime runtime;
  runtime.read_value = 4.0;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("R1 = R2 + 1.5\n"));
  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Completed);
  EXPECT_EQ(runtime.read_calls, 1);
  EXPECT_EQ(runtime.last_read_name, "R2");
  EXPECT_EQ(runtime.write_calls, 1);
  EXPECT_EQ(runtime.last_write_name, "R1");
  EXPECT_DOUBLE_EQ(runtime.last_write_value, 5.5);
}

TEST(StreamingExecutionTest, PlainRuntimeAssignmentWriteCanBlockAndResume) {
  class PendingWriteRuntime : public ReadyRuntime {
  public:
    gcode::RuntimeResult<void> writeVariable(std::string_view name,
                                             double value) override {
      ++write_calls;
      last_name = std::string(name);
      last_value = value;
      gcode::RuntimeResult<void> result;
      if (write_calls == 1) {
        result.status = gcode::RuntimeCallStatus::Pending;
        result.wait_token = gcode::WaitToken{"write", "r1"};
        return result;
      }
      result.status = gcode::RuntimeCallStatus::Ready;
      return result;
    }

    int write_calls = 0;
    double last_value = 0.0;
    std::string last_name;
  };

  RecordingSink sink;
  PendingWriteRuntime runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("R1 = 2\nG1 X3\n"));
  const auto blocked = engine.pump();
  ASSERT_EQ(blocked.status, gcode::StepStatus::Blocked);
  ASSERT_TRUE(blocked.blocked.has_value());
  EXPECT_EQ(blocked.blocked->token.kind, "write");
  EXPECT_EQ(blocked.blocked->token.id, "r1");
  EXPECT_EQ(runtime.write_calls, 1);
  EXPECT_EQ(runtime.last_name, "R1");
  EXPECT_DOUBLE_EQ(runtime.last_value, 2.0);

  const auto resumed = engine.resume(gcode::WaitToken{"write", "r1"});
  EXPECT_EQ(resumed.status, gcode::StepStatus::Progress);

  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Completed);
  EXPECT_EQ(runtime.write_calls, 1);
  ASSERT_EQ(sink.linear_moves.size(), 1u);
  ASSERT_TRUE(sink.linear_moves[0].x.has_value());
  EXPECT_DOUBLE_EQ(*sink.linear_moves[0].x, 3.0);
}

TEST(StreamingExecutionTest,
     SubprogramCallWaitsForMoreInputAndResolvesWithCallStack) {
  RecordingSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("GOTO START\nSTART:\nL1000\nGOTO END\n"));
  ASSERT_EQ(engine.pump().status, gcode::StepStatus::WaitingForInput);

  ASSERT_TRUE(engine.pushChunk("L1000:\nG1 X1\nRET\nEND:\nG1 X2\n"));
  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Completed);
  ASSERT_EQ(sink.linear_moves.size(), 2u);
  ASSERT_TRUE(sink.linear_moves[0].x.has_value());
  ASSERT_TRUE(sink.linear_moves[1].x.has_value());
  EXPECT_DOUBLE_EQ(*sink.linear_moves[0].x, 1.0);
  EXPECT_DOUBLE_EQ(*sink.linear_moves[1].x, 2.0);
  EXPECT_TRUE(sink.diagnostics.empty());
}

TEST(StreamingExecutionTest, FinishFaultsIfSubprogramTargetNeverResolves) {
  RecordingSink sink;
  ReadyRuntime runtime;
  StaticCancellation cancellation;
  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("L1000\n"));
  ASSERT_EQ(engine.pump().status, gcode::StepStatus::WaitingForInput);

  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Faulted);
  ASSERT_FALSE(sink.diagnostics.empty());
  EXPECT_NE(sink.diagnostics.back().message.find("unresolved subprogram"),
            std::string::npos);
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
      [](std::string_view) {
        gcode::RuntimeResult<double> result;
        result.status = gcode::RuntimeCallStatus::Error;
        result.error_message = "not used";
        return result;
      },
      [](std::string_view, double) {
        gcode::RuntimeResult<void> result;
        result.status = gcode::RuntimeCallStatus::Ready;
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
     FunctionExecutionRuntimeCanResolveParenthesizedLogicalAndCondition) {
  RecordingSink sink;
  StaticCancellation cancellation;
  gcode::FunctionExecutionRuntime runtime(
      [](const gcode::Condition &condition, const gcode::SourceInfo &) {
        gcode::ConditionResolution r;
        r.kind = condition.raw_text == "(R1 == 1)AND(R2 > 10)"
                     ? gcode::ConditionResolutionKind::True
                     : gcode::ConditionResolutionKind::False;
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
      [](std::string_view) {
        gcode::RuntimeResult<double> result;
        result.status = gcode::RuntimeCallStatus::Error;
        result.error_message = "not used";
        return result;
      },
      [](std::string_view, double) {
        gcode::RuntimeResult<void> result;
        result.status = gcode::RuntimeCallStatus::Ready;
        return result;
      },
      [](const gcode::WaitToken &) {
        gcode::RuntimeResult<gcode::WaitToken> result;
        result.status = gcode::RuntimeCallStatus::Ready;
        return result;
      });

  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk(
      "IF (R1 == 1) AND (R2 > 10) GOTOF END\nG1 X1\nEND:\nG1 X2\n"));
  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Completed);
  ASSERT_EQ(sink.linear_moves.size(), 1u);
  ASSERT_TRUE(sink.linear_moves[0].x.has_value());
  EXPECT_DOUBLE_EQ(*sink.linear_moves[0].x, 2.0);
}

TEST(StreamingExecutionTest,
     FunctionExecutionRuntimeCanEvaluateAssignmentExpression) {
  RecordingSink sink;
  class RecordingWriteRuntime : public ReadyRuntime {
  public:
    gcode::RuntimeResult<void> writeVariable(std::string_view name,
                                             double value) override {
      ++write_calls;
      last_name = std::string(name);
      last_value = value;
      gcode::RuntimeResult<void> result;
      result.status = gcode::RuntimeCallStatus::Ready;
      return result;
    }

    int write_calls = 0;
    double last_value = 0.0;
    std::string last_name;
  } io_runtime;
  StaticCancellation cancellation;
  int expression_calls = 0;
  gcode::FunctionExecutionRuntime runtime(
      [](const gcode::Condition &, const gcode::SourceInfo &) {
        gcode::ConditionResolution r;
        r.kind = gcode::ConditionResolutionKind::False;
        return r;
      },
      [&](const gcode::LinearMoveCommand &cmd) {
        return io_runtime.submitLinearMove(cmd);
      },
      [&](const gcode::ArcMoveCommand &cmd) {
        return io_runtime.submitArcMove(cmd);
      },
      [&](const gcode::DwellCommand &cmd) {
        return io_runtime.submitDwell(cmd);
      },
      [&](std::string_view name) { return io_runtime.readVariable(name); },
      [&](std::string_view name) {
        return io_runtime.readSystemVariable(name);
      },
      [&](std::string_view name, double value) {
        return io_runtime.writeVariable(name, value);
      },
      [&](const gcode::WaitToken &token) {
        return io_runtime.cancelWait(token);
      },
      [&](const gcode::ExprNode &) {
        ++expression_calls;
        gcode::RuntimeResult<double> result;
        result.status = gcode::RuntimeCallStatus::Ready;
        result.value = 9.5;
        return result;
      });

  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation);

  ASSERT_TRUE(engine.pushChunk("R1 = R2 + 1\n"));
  const auto finished = engine.finish();
  EXPECT_EQ(finished.status, gcode::StepStatus::Completed);
  EXPECT_EQ(expression_calls, 1);
  EXPECT_EQ(io_runtime.write_calls, 1);
  EXPECT_EQ(io_runtime.last_name, "R1");
  EXPECT_DOUBLE_EQ(io_runtime.last_value, 9.5);
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
      [](std::string_view) {
        gcode::RuntimeResult<double> result;
        result.status = gcode::RuntimeCallStatus::Error;
        result.error_message = "not used";
        return result;
      },
      [](std::string_view, double) {
        gcode::RuntimeResult<void> result;
        result.status = gcode::RuntimeCallStatus::Ready;
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
