#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "gtest/gtest.h"

#ifdef __unix__
#include <sys/wait.h>
#endif

namespace {

int decodeExitCode(int status) {
#ifdef __unix__
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
#endif
  return status;
}

std::string readFile(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::in);
  std::stringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

struct CommandResult {
  int exit_code = 0;
  std::string stdout_text;
  std::string stderr_text;
};

CommandResult runCommand(const std::string &command) {
  const auto temp_dir = std::filesystem::temp_directory_path();
  const auto stdout_path = temp_dir / "gcode_cli_test_stdout.txt";
  const auto stderr_path = temp_dir / "gcode_cli_test_stderr.txt";

  const std::string full_command = command + " >\"" + stdout_path.string() +
                                   "\" 2>\"" + stderr_path.string() + "\"";
  const int status = std::system(full_command.c_str());

  CommandResult result;
  result.exit_code = decodeExitCode(status);
  result.stdout_text = readFile(stdout_path);
  result.stderr_text = readFile(stderr_path);
  return result;
}

CommandResult runCommandInDir(const std::filesystem::path &dir,
                              const std::string &command) {
  const std::string full_command =
      std::string("cd \"") + dir.string() + "\" && " + command;
  return runCommand(full_command);
}

TEST(CliFormatTest, DebugFormatMatchesExistingGoldenOutput) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto input_path = source_dir / "testdata" / "g2g3_samples.ngc";
  const auto golden_path = source_dir / "testdata" / "g2g3_samples.golden.txt";

  const std::string command = std::string("\"") + GCODE_PARSE_BIN +
                              "\" --format debug \"" + input_path.string() +
                              "\"";
  const auto result = runCommand(command);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_EQ(result.stdout_text, readFile(golden_path));
  EXPECT_TRUE(result.stderr_text.empty());
}

TEST(CliFormatTest, JsonFormatOutputsStableSchema) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto input_path = source_dir / "testdata" / "g2g3_samples.ngc";

  const std::string command = std::string("\"") + GCODE_PARSE_BIN +
                              "\" --format json \"" + input_path.string() +
                              "\"";
  const auto result = runCommand(command);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());

  const auto parsed = nlohmann::json::parse(result.stdout_text);
  EXPECT_EQ(parsed.value("schema_version", 0), 1);
  ASSERT_TRUE(parsed.contains("program"));
  ASSERT_TRUE(parsed["program"].contains("lines"));
  ASSERT_TRUE(parsed["program"]["lines"].is_array());
  EXPECT_FALSE(parsed["program"]["lines"].empty());
  ASSERT_TRUE(parsed.contains("diagnostics"));
  EXPECT_TRUE(parsed["diagnostics"].is_array());
  EXPECT_TRUE(parsed["diagnostics"].empty());
}

TEST(CliFormatTest, LowerJsonOutputsMessageResultSchema) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto input_path = source_dir / "testdata" / "messages" / "g4_dwell.ngc";

  const std::string command = std::string("\"") + GCODE_PARSE_BIN +
                              "\" --mode lower --format json \"" +
                              input_path.string() + "\"";
  const auto result = runCommand(command);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());

  const auto parsed = nlohmann::json::parse(result.stdout_text);
  EXPECT_EQ(parsed.value("schema_version", 0), 1);
  ASSERT_TRUE(parsed.contains("messages"));
  ASSERT_TRUE(parsed["messages"].is_array());
  EXPECT_EQ(parsed["messages"].size(), 3u);
  ASSERT_TRUE(parsed["messages"][0].contains("modal"));
  EXPECT_EQ(parsed["messages"][0]["modal"]["group"], "GGroup1");
  ASSERT_TRUE(parsed.contains("rejected_lines"));
  EXPECT_TRUE(parsed["rejected_lines"].is_array());
}

TEST(CliFormatTest, LowerDebugOutputsSummaryLines) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto input_path = source_dir / "testdata" / "messages" / "g4_dwell.ngc";

  const std::string command = std::string("\"") + GCODE_PARSE_BIN +
                              "\" --mode lower --format debug \"" +
                              input_path.string() + "\"";
  const auto result = runCommand(command);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());
  EXPECT_NE(result.stdout_text.find("MSG line=1 n=10"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("type=G1"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("group=GGroup1"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("type=G4"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("group=GGroup2"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("SUMMARY messages=3 rejected=0"),
            std::string::npos);
}

TEST(CliFormatTest, AilJsonOutputsInstructionSchema) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto input_path = source_dir / "testdata" / "messages" / "g4_dwell.ngc";

  const std::string command = std::string("\"") + GCODE_PARSE_BIN +
                              "\" --mode ail --format json \"" +
                              input_path.string() + "\"";
  const auto result = runCommand(command);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());

  const auto parsed = nlohmann::json::parse(result.stdout_text);
  EXPECT_EQ(parsed.value("schema_version", 0), 1);
  ASSERT_TRUE(parsed.contains("instructions"));
  ASSERT_TRUE(parsed["instructions"].is_array());
  ASSERT_EQ(parsed["instructions"].size(), 4u);
  EXPECT_EQ(parsed["instructions"][0]["kind"], "m_function");
  EXPECT_EQ(parsed["instructions"][1]["kind"], "motion_linear");
  EXPECT_EQ(parsed["instructions"][1]["modal"]["group"], "GGroup1");
  EXPECT_EQ(parsed["instructions"][2]["kind"], "dwell");
}

TEST(CliFormatTest, AilDebugOutputsSummaryLines) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto input_path = source_dir / "testdata" / "messages" / "g4_dwell.ngc";

  const std::string command = std::string("\"") + GCODE_PARSE_BIN +
                              "\" --mode ail --format debug \"" +
                              input_path.string() + "\"";
  const auto result = runCommand(command);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());
  EXPECT_NE(result.stdout_text.find("AIL line=1"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("kind=m_function"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("kind=motion_linear"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("kind=dwell"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("SUMMARY instructions=4"),
            std::string::npos);
}

TEST(CliFormatTest, AilModeOutputsToolSelectAndToolChangeSchema) {
  const auto temp_file =
      std::filesystem::temp_directory_path() / "gcode_cli_tool_test.ngc";
  {
    std::ofstream out(temp_file, std::ios::out | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out << "N10 T12\nN20 M6\n";
  }

  const std::string json_cmd = std::string("\"") + GCODE_PARSE_BIN +
                               "\" --mode ail --format json \"" +
                               temp_file.string() + "\"";
  const auto json_result = runCommand(json_cmd);
  EXPECT_EQ(json_result.exit_code, 0);
  EXPECT_TRUE(json_result.stderr_text.empty());
  const auto parsed = nlohmann::json::parse(json_result.stdout_text);
  ASSERT_EQ(parsed["instructions"].size(), 2u);
  EXPECT_EQ(parsed["instructions"][0]["kind"], "tool_select");
  EXPECT_EQ(parsed["instructions"][0]["selector_value"], "12");
  EXPECT_EQ(parsed["instructions"][0]["timing"], "deferred_until_m6");
  EXPECT_EQ(parsed["instructions"][1]["kind"], "tool_change");
  EXPECT_EQ(parsed["instructions"][1]["opcode"], "M6");

  const std::string debug_cmd = std::string("\"") + GCODE_PARSE_BIN +
                                "\" --mode ail --format debug \"" +
                                temp_file.string() + "\"";
  const auto debug_result = runCommand(debug_cmd);
  std::error_code ec;
  std::filesystem::remove(temp_file, ec);
  EXPECT_EQ(debug_result.exit_code, 0);
  EXPECT_TRUE(debug_result.stderr_text.empty());
  EXPECT_NE(debug_result.stdout_text.find("kind=tool_select"),
            std::string::npos);
  EXPECT_NE(debug_result.stdout_text.find("kind=tool_change"),
            std::string::npos);
}

TEST(CliFormatTest, StreamingExecDebugOutputsLinearMoveEventSequence) {
  const auto temp_file =
      std::filesystem::temp_directory_path() / "gcode_stream_exec_test.ngc";
  {
    std::ofstream out(temp_file, std::ios::out | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out << "N10 G1 X10 Y20 F100\n";
  }

  const std::string command = std::string("\"") + GCODE_STREAM_EXEC_BIN +
                              "\" --format debug \"" + temp_file.string() +
                              "\"";
  const auto result = runCommand(command);

  std::error_code ec;
  std::filesystem::remove(temp_file, ec);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());
  EXPECT_NE(result.stdout_text.find("line 1 n=10"), std::string::npos);
  EXPECT_NE(result.stdout_text.find(
                "  emit linear_move: motion=G1 plane=xy comp=off"),
            std::string::npos);
  EXPECT_NE(result.stdout_text.find("  target: x=10 y=20 feed=100"),
            std::string::npos);
  EXPECT_NE(result.stdout_text.find("  runtime: submit_linear_move"),
            std::string::npos);
  EXPECT_NE(result.stdout_text.find("engine.completed"), std::string::npos);
}

TEST(CliFormatTest, StreamingExecDebugOutputsGroupedMixedFixture) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto input_path = source_dir / "testdata" / "messages" / "g4_dwell.ngc";

  const std::string command = std::string("\"") + GCODE_STREAM_EXEC_BIN +
                              "\" --format debug \"" + input_path.string() +
                              "\"";
  const auto result = runCommand(command);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());
  EXPECT_NE(result.stdout_text.find("line 1 n=10"), std::string::npos);
  EXPECT_NE(result.stdout_text.find(
                "  emit linear_move: motion=G1 plane=xy comp=off"),
            std::string::npos);
  EXPECT_NE(result.stdout_text.find("  target: z=-5 feed=200"),
            std::string::npos);
  EXPECT_NE(result.stdout_text.find("line 2 n=20"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("  emit dwell: mode=seconds value=3"),
            std::string::npos);
  EXPECT_NE(result.stdout_text.find("line 4 n=40"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("  emit dwell: mode=revolutions value=30"),
            std::string::npos);
  EXPECT_NE(result.stdout_text.find("engine.completed"), std::string::npos);
}

TEST(CliFormatTest, StreamingExecJsonOutputsDeterministicEventLog) {
  const auto temp_file =
      std::filesystem::temp_directory_path() / "gcode_stream_exec_json.ngc";
  {
    std::ofstream out(temp_file, std::ios::out | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out << "G4 F3\n";
  }

  const std::string command = std::string("\"") + GCODE_STREAM_EXEC_BIN +
                              "\" --format json \"" + temp_file.string() + "\"";
  const auto result = runCommand(command);

  std::error_code ec;
  std::filesystem::remove(temp_file, ec);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());

  std::stringstream lines(result.stdout_text);
  std::string first;
  std::string second;
  std::string third;
  ASSERT_TRUE(std::getline(lines, first));
  ASSERT_TRUE(std::getline(lines, second));
  ASSERT_TRUE(std::getline(lines, third));

  const auto first_json = nlohmann::json::parse(first);
  const auto second_json = nlohmann::json::parse(second);
  const auto third_json = nlohmann::json::parse(third);

  EXPECT_EQ(first_json["event"], "sink.dwell");
  EXPECT_EQ(second_json["event"], "runtime.submit_dwell");
  EXPECT_EQ(third_json["event"], "engine.completed");
}

TEST(CliFormatTest, StreamingExecFixtureShowsWorkingPlaneTransitions) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto input_path =
      source_dir / "testdata" / "execution" / "plane_switch.ngc";

  const std::string command = std::string("\"") + GCODE_STREAM_EXEC_BIN +
                              "\" --format json \"" + input_path.string() +
                              "\"";
  const auto result = runCommand(command);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());

  std::stringstream lines(result.stdout_text);
  std::string line;
  std::vector<nlohmann::json> events;
  while (std::getline(lines, line)) {
    if (line.empty()) {
      continue;
    }
    events.push_back(nlohmann::json::parse(line));
  }

  ASSERT_EQ(events.size(), 7u);
  EXPECT_EQ(events[0]["event"], "sink.linear_move");
  EXPECT_EQ(events[1]["event"], "runtime.submit_linear_move");
  EXPECT_EQ(events[2]["event"], "sink.linear_move");
  EXPECT_EQ(events[3]["event"], "runtime.submit_linear_move");
  EXPECT_EQ(events[4]["event"], "sink.linear_move");
  EXPECT_EQ(events[5]["event"], "runtime.submit_linear_move");
  EXPECT_EQ(events[6]["event"], "engine.completed");

  EXPECT_EQ(events[0]["line"], 2);
  EXPECT_EQ(events[0]["params"]["source"]["line_number"], 10);
  EXPECT_EQ(events[0]["params"]["effective"]["working_plane"], "xy");

  EXPECT_EQ(events[2]["line"], 4);
  EXPECT_EQ(events[2]["params"]["source"]["line_number"], 20);
  EXPECT_EQ(events[2]["params"]["effective"]["working_plane"], "zx");

  EXPECT_EQ(events[4]["line"], 6);
  EXPECT_EQ(events[4]["params"]["source"]["line_number"], 30);
  EXPECT_EQ(events[4]["params"]["effective"]["working_plane"], "yz");
}

TEST(CliFormatTest, ExecutionSessionCliReportsRecoverableRejectedState) {
  const auto temp_file =
      std::filesystem::temp_directory_path() / "gcode_exec_session_reject.ngc";
  {
    std::ofstream out(temp_file, std::ios::out | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out << "G1 X10\nG1 G2 X15\nG1 X20\n";
  }

  const std::string command = std::string("\"") + GCODE_EXEC_SESSION_BIN +
                              "\" --format debug \"" + temp_file.string() +
                              "\"";
  const auto result = runCommand(command);

  std::error_code ec;
  std::filesystem::remove(temp_file, ec);

  EXPECT_EQ(result.exit_code, 1);
  EXPECT_TRUE(result.stderr_text.empty());
  EXPECT_NE(result.stdout_text.find("rejected_line line=2"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("session.rejected"), std::string::npos);
}

TEST(CliFormatTest, ExecutionSessionCliCanApplyEditableSuffixReplacement) {
  const auto temp_file =
      std::filesystem::temp_directory_path() / "gcode_exec_session_input.ngc";
  const auto replace_file =
      std::filesystem::temp_directory_path() / "gcode_exec_session_replace.ngc";
  {
    std::ofstream out(temp_file, std::ios::out | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out << "G1 X10\nG1 G2 X15\nG1 X20\n";
  }
  {
    std::ofstream out(replace_file, std::ios::out | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out << "G1 X15\nG1 X20\n";
  }

  const std::string command = std::string("\"") + GCODE_EXEC_SESSION_BIN +
                              "\" --format debug --replace-editable-suffix \"" +
                              replace_file.string() + "\" \"" +
                              temp_file.string() + "\"";
  const auto result = runCommand(command);

  std::error_code ec;
  std::filesystem::remove(temp_file, ec);
  std::filesystem::remove(replace_file, ec);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());
  EXPECT_NE(result.stdout_text.find("session.editable_suffix_replaced"),
            std::string::npos);
  EXPECT_NE(result.stdout_text.find("target: x=15"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("session.completed"), std::string::npos);
}

TEST(CliFormatTest, StreamingExecDebugOutputsToolChangeEventSequence) {
  const auto temp_file =
      std::filesystem::temp_directory_path() / "gcode_stream_exec_tool.ngc";
  {
    std::ofstream out(temp_file, std::ios::out | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out << "T12\nM6\n";
  }

  const std::string command = std::string("\"") + GCODE_STREAM_EXEC_BIN +
                              "\" --format debug \"" + temp_file.string() +
                              "\"";
  const auto result = runCommand(command);

  std::error_code ec;
  std::filesystem::remove(temp_file, ec);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());
  EXPECT_NE(result.stdout_text.find("line 2"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("  emit tool_change: target=12"),
            std::string::npos);
  EXPECT_NE(result.stdout_text.find("selected=12"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("  runtime: submit_tool_change"),
            std::string::npos);
  EXPECT_NE(result.stdout_text.find("engine.completed"), std::string::npos);
}

TEST(CliFormatTest, AilModeHandlesAssignmentExpression) {
  const auto temp_file =
      std::filesystem::temp_directory_path() / "gcode_cli_assign_test.ngc";
  {
    std::ofstream out(temp_file, std::ios::out | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out << "R1 = $P_ACT_X + 2*R2\n";
  }

  const std::string command = std::string("\"") + GCODE_PARSE_BIN +
                              "\" --mode ail --format json \"" +
                              temp_file.string() + "\"";
  const auto result = runCommand(command);

  std::error_code ec;
  std::filesystem::remove(temp_file, ec);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());
  const auto parsed = nlohmann::json::parse(result.stdout_text);
  ASSERT_EQ(parsed["instructions"].size(), 1u);
  EXPECT_EQ(parsed["instructions"][0]["kind"], "assign");
  EXPECT_EQ(parsed["instructions"][0]["lhs"], "R1");
}

TEST(CliFormatTest, AilModeOutputsSubprogramCallSchema) {
  const auto temp_file =
      std::filesystem::temp_directory_path() / "gcode_cli_call_test.ngc";
  {
    std::ofstream out(temp_file, std::ios::out | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out << "L1000 P2\n";
  }

  const std::string command = std::string("\"") + GCODE_PARSE_BIN +
                              "\" --mode ail --format json \"" +
                              temp_file.string() + "\"";
  const auto result = runCommand(command);

  std::error_code ec;
  std::filesystem::remove(temp_file, ec);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());
  const auto parsed = nlohmann::json::parse(result.stdout_text);
  ASSERT_EQ(parsed["instructions"].size(), 1u);
  EXPECT_EQ(parsed["instructions"][0]["kind"], "subprogram_call");
  EXPECT_EQ(parsed["instructions"][0]["target"], "L1000");
  EXPECT_EQ(parsed["instructions"][0]["repeat_count"], 2);
}

TEST(CliFormatTest, PacketJsonOutputsPacketSchema) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto input_path = source_dir / "testdata" / "messages" / "g4_dwell.ngc";

  const std::string command = std::string("\"") + GCODE_PARSE_BIN +
                              "\" --mode packet --format json \"" +
                              input_path.string() + "\"";
  const auto result = runCommand(command);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());

  const auto parsed = nlohmann::json::parse(result.stdout_text);
  EXPECT_EQ(parsed.value("schema_version", 0), 1);
  ASSERT_TRUE(parsed.contains("packets"));
  ASSERT_TRUE(parsed["packets"].is_array());
  ASSERT_EQ(parsed["packets"].size(), 3u);
  EXPECT_EQ(parsed["packets"][0]["type"], "linear_move");
  EXPECT_EQ(parsed["packets"][1]["type"], "dwell");
}

TEST(CliFormatTest, PacketDebugOutputsSummaryLines) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto input_path = source_dir / "testdata" / "messages" / "g4_dwell.ngc";

  const std::string command = std::string("\"") + GCODE_PARSE_BIN +
                              "\" --mode packet --format debug \"" +
                              input_path.string() + "\"";
  const auto result = runCommand(command);

  EXPECT_EQ(result.exit_code, 0);
  EXPECT_TRUE(result.stderr_text.empty());
  EXPECT_NE(result.stdout_text.find("PACKET id=1"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("type=linear_move"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("type=dwell"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("SUMMARY packets=3 rejected=0"),
            std::string::npos);
}

TEST(CliFormatTest, UnsupportedModeReturnsUsageError) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto input_path = source_dir / "testdata" / "g1_samples.ngc";

  const std::string command = std::string("\"") + GCODE_PARSE_BIN +
                              "\" --mode nope \"" + input_path.string() + "\"";
  const auto result = runCommand(command);

  EXPECT_EQ(result.exit_code, 2);
  EXPECT_TRUE(result.stdout_text.empty());
  EXPECT_NE(result.stderr_text.find("Unsupported mode"), std::string::npos);
}

TEST(CliFormatTest, StageOutputsMatchGoldens) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto golden_dir = source_dir / "testdata" / "cli";

  struct Case {
    std::string mode;
    std::string format;
    std::string golden_file;
  };

  const std::vector<Case> cases = {
      {"parse", "debug", "parse_debug.golden.txt"},
      {"parse", "json", "parse_json.golden.json"},
      {"ail", "debug", "ail_debug.golden.txt"},
      {"ail", "json", "ail_json.golden.json"},
      {"packet", "debug", "packet_debug.golden.txt"},
      {"packet", "json", "packet_json.golden.json"},
      {"lower", "debug", "lower_debug.golden.txt"},
      {"lower", "json", "lower_json.golden.json"},
  };

  for (const auto &tc : cases) {
    const std::string command = std::string("\"") + GCODE_PARSE_BIN +
                                "\" --mode " + tc.mode + " --format " +
                                tc.format + " testdata/messages/g4_dwell.ngc";
    const auto result = runCommandInDir(source_dir, command);
    EXPECT_EQ(result.exit_code, 0)
        << "mode=" << tc.mode << " format=" << tc.format;
    EXPECT_TRUE(result.stderr_text.empty());

    const auto golden = readFile(golden_dir / tc.golden_file);
    EXPECT_EQ(result.stdout_text, golden)
        << "golden mismatch for mode=" << tc.mode << " format=" << tc.format;
  }
}

} // namespace
