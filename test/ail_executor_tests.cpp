#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "ail.h"
#include "tool_policy.h"

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

TEST(AilExecutorTest, ResolvesDirectionalGotoByLineNumber) {
  const auto lowered = gcode::parseAndLowerAil(
      "N100 G1 X1\nGOTOF N300\nN200 G1 X2\nN300 G1 X3\n");
  gcode::AilExecutor exec(lowered.instructions);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver)); // N100 move
  ASSERT_EQ(exec.state().pc, 1u);
  ASSERT_TRUE(exec.step(0, resolver)); // GOTOF N300
  ASSERT_EQ(exec.state().pc, 3u);
}

TEST(AilExecutorTest, WarnsWhenDuplicateLineNumbersAreAmbiguous) {
  const auto lowered = gcode::parseAndLowerAil(
      "N100 G1 X1\nN200 GOTO N300\nN300 G1 X2\nN300 G1 X3\n");
  gcode::AilExecutor exec(lowered.instructions);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver)); // N100 move
  ASSERT_TRUE(exec.step(0, resolver)); // GOTO N300 (ambiguous forward)
  ASSERT_FALSE(exec.diagnostics().empty());
  EXPECT_EQ(exec.diagnostics().back().severity,
            gcode::Diagnostic::Severity::Warning);
  EXPECT_NE(
      exec.diagnostics().back().message.find("multiple forward blocks match"),
      std::string::npos);
}

TEST(AilExecutorTest, KnownMFunctionAdvancesWithoutFault) {
  const auto lowered = gcode::parseAndLowerAil("M3\nG1 X1\n");
  gcode::AilExecutor exec(lowered.instructions);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver)); // M3
  EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::Ready);
  ASSERT_TRUE(exec.step(0, resolver)); // G1
  ASSERT_TRUE(exec.step(0, resolver)); // complete
  EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::Completed);
}

TEST(AilExecutorTest, UnknownMFunctionFaultsByDefaultPolicy) {
  const auto lowered = gcode::parseAndLowerAil("M123456\n");
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
  EXPECT_NE(exec.diagnostics().back().message.find("unsupported M function"),
            std::string::npos);
}

TEST(AilExecutorTest, UnknownMFunctionWarningPolicyContinuesExecution) {
  const auto lowered = gcode::parseAndLowerAil("M123456\nG1 X1\n");
  gcode::AilExecutor exec(lowered.instructions, gcode::ErrorPolicy::Warning);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver)); // M123456
  EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::Ready);
  ASSERT_FALSE(exec.diagnostics().empty());
  EXPECT_EQ(exec.diagnostics().back().severity,
            gcode::Diagnostic::Severity::Warning);
  ASSERT_TRUE(exec.step(0, resolver)); // G1
  ASSERT_TRUE(exec.step(0, resolver)); // complete
  EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::Completed);
}

TEST(AilExecutorTest, UnknownMFunctionIgnorePolicySuppressesDiagnostic) {
  const auto lowered = gcode::parseAndLowerAil("M123456\n");
  gcode::AilExecutor exec(lowered.instructions, gcode::ErrorPolicy::Ignore);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver));
  ASSERT_TRUE(exec.step(0, resolver));
  EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::Completed);
  EXPECT_TRUE(exec.diagnostics().empty());
}

TEST(AilExecutorTest, TracksRapidModeStateFromRTLIONAndRTLIOF) {
  const auto lowered =
      gcode::parseAndLowerAil("G0 X1\nRTLIOF\nG0 X2\nRTLION\nG0 X3\n");
  gcode::AilExecutor exec(lowered.instructions);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver)); // G0 X1
  EXPECT_FALSE(exec.state().rapid_mode_current.has_value());

  ASSERT_TRUE(exec.step(0, resolver)); // RTLIOF
  ASSERT_TRUE(exec.state().rapid_mode_current.has_value());
  EXPECT_EQ(*exec.state().rapid_mode_current,
            gcode::RapidInterpolationMode::NonLinear);

  ASSERT_TRUE(exec.step(0, resolver)); // G0 X2
  ASSERT_TRUE(exec.state().rapid_mode_current.has_value());
  EXPECT_EQ(*exec.state().rapid_mode_current,
            gcode::RapidInterpolationMode::NonLinear);

  ASSERT_TRUE(exec.step(0, resolver)); // RTLION
  ASSERT_TRUE(exec.state().rapid_mode_current.has_value());
  EXPECT_EQ(*exec.state().rapid_mode_current,
            gcode::RapidInterpolationMode::Linear);
}

TEST(AilExecutorTest, TracksToolRadiusCompStateFromG40G41G42) {
  const auto lowered = gcode::parseAndLowerAil("G41\nG1 X1\nG42\nG40\n");
  gcode::AilExecutor exec(lowered.instructions);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver)); // G41
  ASSERT_TRUE(exec.state().tool_radius_comp_current.has_value());
  EXPECT_EQ(*exec.state().tool_radius_comp_current,
            gcode::ToolRadiusCompMode::Left);

  ASSERT_TRUE(exec.step(0, resolver)); // G1
  ASSERT_TRUE(exec.state().tool_radius_comp_current.has_value());
  EXPECT_EQ(*exec.state().tool_radius_comp_current,
            gcode::ToolRadiusCompMode::Left);

  ASSERT_TRUE(exec.step(0, resolver)); // G42
  ASSERT_TRUE(exec.state().tool_radius_comp_current.has_value());
  EXPECT_EQ(*exec.state().tool_radius_comp_current,
            gcode::ToolRadiusCompMode::Right);

  ASSERT_TRUE(exec.step(0, resolver)); // G40
  ASSERT_TRUE(exec.state().tool_radius_comp_current.has_value());
  EXPECT_EQ(*exec.state().tool_radius_comp_current,
            gcode::ToolRadiusCompMode::Off);
}

TEST(AilExecutorTest, TracksWorkingPlaneStateFromG17G18G19) {
  const auto lowered = gcode::parseAndLowerAil("G17\nG1 X1\nG18\nG19\n");
  gcode::AilExecutor exec(lowered.instructions);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver)); // G17
  ASSERT_TRUE(exec.state().working_plane_current.has_value());
  EXPECT_EQ(*exec.state().working_plane_current, gcode::WorkingPlane::XY);

  ASSERT_TRUE(exec.step(0, resolver)); // G1
  ASSERT_TRUE(exec.state().working_plane_current.has_value());
  EXPECT_EQ(*exec.state().working_plane_current, gcode::WorkingPlane::XY);

  ASSERT_TRUE(exec.step(0, resolver)); // G18
  ASSERT_TRUE(exec.state().working_plane_current.has_value());
  EXPECT_EQ(*exec.state().working_plane_current, gcode::WorkingPlane::ZX);

  ASSERT_TRUE(exec.step(0, resolver)); // G19
  ASSERT_TRUE(exec.state().working_plane_current.has_value());
  EXPECT_EQ(*exec.state().working_plane_current, gcode::WorkingPlane::YZ);
}

TEST(AilExecutorTest, DeferredToolModeUsesPendingSelectionUntilM6) {
  gcode::LowerOptions options;
  options.tool_change_mode = gcode::ToolChangeMode::DeferredM6;
  const auto lowered = gcode::parseAndLowerAil("T12\nM6\n", options);
  gcode::AilExecutor exec(lowered.instructions);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver)); // T12
  EXPECT_FALSE(exec.state().active_tool_selection.has_value());
  ASSERT_TRUE(exec.state().pending_tool_selection.has_value());
  EXPECT_EQ(exec.state().pending_tool_selection->selector_value, "12");

  ASSERT_TRUE(exec.step(0, resolver)); // M6
  ASSERT_TRUE(exec.state().active_tool_selection.has_value());
  EXPECT_EQ(exec.state().active_tool_selection->selector_value, "12");
  EXPECT_FALSE(exec.state().pending_tool_selection.has_value());
}

TEST(AilExecutorTest, DeferredToolModeUsesLastSelectionBeforeM6) {
  gcode::LowerOptions options;
  options.tool_change_mode = gcode::ToolChangeMode::DeferredM6;
  const auto lowered = gcode::parseAndLowerAil("T12\nT7\nM6\n", options);
  gcode::AilExecutor exec(lowered.instructions);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver)); // T12
  ASSERT_TRUE(exec.state().pending_tool_selection.has_value());
  EXPECT_EQ(exec.state().pending_tool_selection->selector_value, "12");

  ASSERT_TRUE(exec.step(0, resolver)); // T7
  ASSERT_TRUE(exec.state().pending_tool_selection.has_value());
  EXPECT_EQ(exec.state().pending_tool_selection->selector_value, "7");

  ASSERT_TRUE(exec.step(0, resolver)); // M6
  ASSERT_TRUE(exec.state().active_tool_selection.has_value());
  EXPECT_EQ(exec.state().active_tool_selection->selector_value, "7");
  EXPECT_FALSE(exec.state().pending_tool_selection.has_value());
}

TEST(AilExecutorTest, DirectToolModeActivatesImmediatelyOnToolSelect) {
  gcode::LowerOptions options;
  options.tool_change_mode = gcode::ToolChangeMode::DirectT;
  const auto lowered = gcode::parseAndLowerAil("T12\n", options);
  gcode::AilExecutor exec(lowered.instructions);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver)); // T12
  ASSERT_TRUE(exec.state().active_tool_selection.has_value());
  EXPECT_EQ(exec.state().active_tool_selection->selector_value, "12");
  EXPECT_FALSE(exec.state().pending_tool_selection.has_value());
}

TEST(AilExecutorTest, M6WithoutPendingSelectionFaultsByDefault) {
  gcode::LowerOptions options;
  options.tool_change_mode = gcode::ToolChangeMode::DeferredM6;
  const auto lowered = gcode::parseAndLowerAil("M6\n", options);
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
  EXPECT_NE(exec.diagnostics().back().message.find("no pending tool selection"),
            std::string::npos);
}

TEST(AilExecutorTest, M6WithoutPendingSelectionWarningPolicyContinues) {
  gcode::LowerOptions options;
  options.tool_change_mode = gcode::ToolChangeMode::DeferredM6;
  const auto lowered = gcode::parseAndLowerAil("M6\nG1 X1\n", options);
  gcode::AilExecutor exec(lowered.instructions, gcode::ErrorPolicy::Error,
                          gcode::ErrorPolicy::Warning);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver)); // M6
  EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::Ready);
  ASSERT_FALSE(exec.diagnostics().empty());
  EXPECT_EQ(exec.diagnostics().back().severity,
            gcode::Diagnostic::Severity::Warning);
  ASSERT_TRUE(exec.step(0, resolver)); // G1
  ASSERT_TRUE(exec.step(0, resolver)); // complete
  EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::Completed);
}

TEST(AilExecutorTest, ToolPolicySubstitutesPendingSelectionDuringM6) {
  gcode::ToolPolicyOptions policy_options;
  policy_options.allow_substitution = true;
  policy_options.substitution_map["12"] = "7";
  const auto policy =
      std::make_shared<gcode::DefaultToolPolicy>(policy_options);

  gcode::LowerOptions options;
  options.tool_change_mode = gcode::ToolChangeMode::DeferredM6;
  const auto lowered = gcode::parseAndLowerAil("T12\nM6\n", options);
  gcode::AilExecutor exec(lowered.instructions, gcode::ErrorPolicy::Error,
                          gcode::ErrorPolicy::Error, policy);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver)); // T12
  ASSERT_TRUE(exec.step(0, resolver)); // M6
  ASSERT_TRUE(exec.state().active_tool_selection.has_value());
  EXPECT_EQ(exec.state().active_tool_selection->selector_value, "7");
  ASSERT_FALSE(exec.diagnostics().empty());
  EXPECT_EQ(exec.diagnostics().back().severity,
            gcode::Diagnostic::Severity::Warning);
}

TEST(AilExecutorTest, ToolPolicyFallbackResolvesUnresolvedSelection) {
  gcode::ToolPolicyOptions policy_options;
  policy_options.fallback_selection =
      gcode::ToolSelectionState{std::nullopt, "7"};
  const auto policy =
      std::make_shared<gcode::DefaultToolPolicy>(policy_options);

  gcode::LowerOptions options;
  options.tool_change_mode = gcode::ToolChangeMode::DeferredM6;
  const auto lowered = gcode::parseAndLowerAil("T999991\nM6\n", options);
  gcode::AilExecutor exec(lowered.instructions, gcode::ErrorPolicy::Error,
                          gcode::ErrorPolicy::Error, policy);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver));
  ASSERT_TRUE(exec.step(0, resolver));
  ASSERT_TRUE(exec.state().active_tool_selection.has_value());
  EXPECT_EQ(exec.state().active_tool_selection->selector_value, "7");
  ASSERT_FALSE(exec.diagnostics().empty());
  EXPECT_EQ(exec.diagnostics().back().severity,
            gcode::Diagnostic::Severity::Warning);
}

TEST(AilExecutorTest, ToolPolicyAmbiguousSelectionFaultsByDefault) {
  const auto policy = std::make_shared<gcode::DefaultToolPolicy>();

  gcode::LowerOptions options;
  options.tool_change_mode = gcode::ToolChangeMode::DirectT;
  const auto lowered = gcode::parseAndLowerAil("T999992\n", options);
  gcode::AilExecutor exec(lowered.instructions, gcode::ErrorPolicy::Error,
                          gcode::ErrorPolicy::Error, policy);

  const auto resolver = [](const gcode::Condition &,
                           const gcode::SourceInfo &) {
    gcode::ConditionResolution r;
    r.kind = gcode::ConditionResolutionKind::False;
    return r;
  };

  ASSERT_TRUE(exec.step(0, resolver));
  EXPECT_EQ(exec.state().status, gcode::ExecutorStatus::Fault);
  ASSERT_FALSE(exec.diagnostics().empty());
  EXPECT_NE(exec.diagnostics().back().message.find("ambiguous"),
            std::string::npos);
}

} // namespace
