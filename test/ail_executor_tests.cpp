#include <algorithm>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "ail.h"

namespace {

TEST(AilExecutorTest, ResolvesGotoAndCompletes) {
  const auto lowered = gcode::parseAndLowerAil("L1:\nGOTO L2\nL2:\n");
  gcode::AilExecutor exec(lowered.instructions);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver)); // label
  ASSERT_TRUE(exec.step(0, resolver)); // goto L2
  ASSERT_TRUE(exec.step(0, resolver)); // label L2
  ASSERT_TRUE(exec.step(0, resolver)); // complete
  EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::Completed);
}

TEST(AilExecutorTest, GotocDoesNotFaultWhenTargetMissing) {
  const auto lowered = gcode::parseAndLowerAil("GOTOC MISSING\n");
  gcode::AilExecutor exec(lowered.instructions);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver));
  ASSERT_TRUE(exec.step(0, resolver));
  EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::Completed);
}

TEST(AilExecutorTest, GotoFaultsWhenTargetMissing) {
  const auto lowered = gcode::parseAndLowerAil("GOTO MISSING\n");
  gcode::AilExecutor exec(lowered.instructions);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver));
  EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::Fault);
  ASSERT_FALSE(exec.diagnostics().empty());
}

TEST(AilExecutorTest, BranchCanBlockAndResumeOnEvent) {
  const auto lowered = gcode::parseAndLowerAil(
      "IF R1 == 1 GOTOF TARGET\nGOTO END\nTARGET:\nEND:\n");
  gcode::AilExecutor exec(lowered.instructions);

  int calls = 0;
  const auto resolver = [&calls](const gcode::Condition &,
                                 const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    ++calls;
    if (calls == 1) {
      r.kind = gcode::ConditionResolutionKind::Pending;
      r.wait_key = "sensor_ready";
      r.retry_at_ms = 100;
      return r;
    }
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver));
  EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::BlockedOnCondition);

  EXPECT_FALSE(exec.step(10, resolver));
  EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::BlockedOnCondition);

  exec.notifyEvent("sensor_ready");
  ASSERT_TRUE(exec.step(10, resolver));
  EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::Ready);
}

TEST(AilExecutorTest, StructuredIfElseExecutesSingleBranchAtRuntime) {
  const auto lowered =
      gcode::parseAndLowerAil("IF R1 == 1\nG1 X1\nELSE\nG1 X2\nENDIF\n");
  ASSERT_TRUE(lowered.diagnostics.empty());

  const auto collectVisited =
      [&](gcode::ConditionResolutionKind kind) -> std::vector<size_t> {
    gcode::AilExecutor exec(lowered.instructions);
    std::vector<size_t> visited;
    const auto resolver = [kind](const gcode::Condition &,
                                 const gcode::SourceInfo &) {
      gcode::ConditionResolution r;
      r.kind = kind;
      return r;
    };

    int guard = 0;
    while (exec.state().status != gcode::ExecutorStatus::Completed &&
           exec.state().status != gcode::ExecutorStatus::Fault && guard < 64) {
      visited.push_back(exec.state().pc);
      if (!exec.step(0, resolver)) {
        ADD_FAILURE() << "executor unexpectedly stopped while collecting path";
        break;
      }
      ++guard;
    }
    EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::Completed);
    return visited;
  };

  const auto visited_true =
      collectVisited(gcode::ConditionResolutionKind::True);
  const auto visited_false =
      collectVisited(gcode::ConditionResolutionKind::False);

  // With lowered layout, instruction index 5 is ELSE-body motion.
  EXPECT_EQ(std::find(visited_true.begin(), visited_true.end(), 5u),
            visited_true.end());
  EXPECT_NE(std::find(visited_false.begin(), visited_false.end(), 5u),
            visited_false.end());
}

} // namespace
