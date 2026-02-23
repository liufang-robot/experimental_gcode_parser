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

} // namespace
