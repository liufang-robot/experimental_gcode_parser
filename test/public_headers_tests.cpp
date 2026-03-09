#include <type_traits>

#include "gtest/gtest.h"

#include "gcode/ail.h"
#include "gcode/condition_runtime.h"
#include "gcode/execution_commands.h"
#include "gcode/execution_interfaces.h"
#include "gcode/execution_runtime.h"
#include "gcode/gcode_parser.h"
#include "gcode/message_diff.h"
#include "gcode/messages.h"
#include "gcode/packet.h"
#include "gcode/policy_types.h"
#include "gcode/runtime_status.h"
#include "gcode/session.h"
#include "gcode/streaming_execution_engine.h"

TEST(PublicHeadersTest, PublicFacadeHeadersCompileAndExposeKeyTypes) {
  static_assert(std::is_class_v<gcode::ParseResult>);
  static_assert(std::is_class_v<gcode::MessageResult>);
  static_assert(std::is_class_v<gcode::AilResult>);
  static_assert(std::is_class_v<gcode::PacketResult>);
  static_assert(std::is_class_v<gcode::StreamingExecutionEngine>);
  static_assert(std::is_class_v<gcode::IExecutionSink>);
  static_assert(std::is_class_v<gcode::IRuntime>);
  static_assert(std::is_class_v<gcode::IExecutionRuntime>);
  static_assert(std::is_class_v<gcode::IConditionResolver>);
  static_assert(std::is_class_v<gcode::WaitToken>);
  static_assert(std::is_class_v<gcode::AilExecutorOptions>);
  static_assert(std::is_class_v<gcode::ToolSelectionState>);
  static_assert(std::is_enum_v<gcode::ToolChangeMode>);
  static_assert(std::is_enum_v<gcode::ErrorPolicy>);
}
