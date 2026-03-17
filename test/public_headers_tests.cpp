#include <type_traits>

#include "gtest/gtest.h"

#include "gcode/ail.h"
#include "gcode/condition_runtime.h"
#include "gcode/execution_commands.h"
#include "gcode/execution_interfaces.h"
#include "gcode/execution_runtime.h"
#include "gcode/execution_session.h"
#include "gcode/gcode_parser.h"
#include "gcode/lowering_types.h"
#include "gcode/policy_types.h"
#include "gcode/runtime_status.h"

TEST(PublicHeadersTest, PublicFacadeHeadersCompileAndExposeKeyTypes) {
  static_assert(std::is_class_v<gcode::ParseResult>);
  static_assert(std::is_class_v<gcode::AilResult>);
  static_assert(std::is_class_v<gcode::ExecutionSession>);
  static_assert(std::is_class_v<gcode::IExecutionSink>);
  static_assert(std::is_class_v<gcode::IRuntime>);
  static_assert(std::is_class_v<gcode::ModalUpdateEvent>);
  static_assert(std::is_class_v<gcode::IExecutionRuntime>);
  static_assert(std::is_class_v<gcode::FunctionExecutionRuntime>);
  static_assert(std::is_class_v<gcode::IConditionResolver>);
  static_assert(std::is_class_v<gcode::WaitToken>);
  static_assert(std::is_class_v<gcode::RejectedState>);
  static_assert(std::is_class_v<gcode::AilExecutorOptions>);
  static_assert(std::is_class_v<gcode::ToolSelectionState>);
  static_assert(std::is_class_v<gcode::SourceInfo>);
  static_assert(std::is_class_v<gcode::LowerOptions>);
  static_assert(std::is_class_v<gcode::RejectedLine>);
  static_assert(std::is_enum_v<gcode::ToolChangeMode>);
  static_assert(std::is_enum_v<gcode::ErrorPolicy>);
  static_assert(static_cast<int>(gcode::EngineState::Rejected) >= 0);
  static_assert(static_cast<int>(gcode::StepStatus::Rejected) >= 0);
  static_assert(std::is_same_v<decltype(gcode::StepResult{}.rejected),
                               std::optional<gcode::RejectedState>>);
}
