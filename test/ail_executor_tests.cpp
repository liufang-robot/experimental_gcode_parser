#include <string>

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

} // namespace
