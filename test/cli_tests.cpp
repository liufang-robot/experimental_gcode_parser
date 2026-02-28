#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

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
  ASSERT_EQ(parsed["instructions"].size(), 3u);
  EXPECT_EQ(parsed["instructions"][0]["kind"], "motion_linear");
  EXPECT_EQ(parsed["instructions"][0]["modal"]["group"], "GGroup1");
  EXPECT_EQ(parsed["instructions"][1]["kind"], "dwell");
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
  EXPECT_NE(result.stdout_text.find("kind=motion_linear"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("kind=dwell"), std::string::npos);
  EXPECT_NE(result.stdout_text.find("SUMMARY instructions=3"),
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

} // namespace
