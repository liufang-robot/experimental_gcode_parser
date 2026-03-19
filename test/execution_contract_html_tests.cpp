#include <filesystem>
#include <fstream>
#include <string>

#include "gtest/gtest.h"

#include "execution_contract_html.h"

namespace {

std::string readFile(const std::filesystem::path &path) {
  std::ifstream input(path);
  std::stringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

TEST(ExecutionContractHtmlTest, WritesIndexAndCasePage) {
  const auto temp_root =
      std::filesystem::temp_directory_path() / "execution_contract_html_test";
  std::filesystem::remove_all(temp_root);
  std::filesystem::create_directories(temp_root / "core");

  const auto program_path = temp_root / "core" / "linear_move_completed.ngc";
  {
    std::ofstream out(program_path);
    out << "N10 G1 X10 Y20 F100\n";
  }

  gcode::ExecutionContractCaseReport report;
  report.suite_name = "core";
  report.case_name = "linear_move_completed";
  report.description = "simple linear move";
  report.program_path = program_path.string();
  report.reference_path =
      "testdata/execution_contract/core/linear_move_completed.events.yaml";
  report.actual_path =
      "output/execution_contract_review/core/linear_move_completed.actual.yaml";
  report.matches_reference = false;
  report.reference_trace.name = "linear_move_completed";
  report.reference_trace.options.tool_change_mode =
      gcode::ToolChangeMode::DeferredM6;
  report.reference_trace.driver = {
      {gcode::ExecutionContractDriverAction::Finish},
      {gcode::ExecutionContractDriverAction::ResumeBlocked}};
  report.actual_trace.name = "linear_move_completed";

  const auto output_root = temp_root / "site";
  std::filesystem::remove_all(output_root);

  gcode::writeExecutionContractHtmlSite({report}, output_root.string());

  const auto index_path = output_root / "index.html";
  const auto case_path = output_root / "core" / "linear_move_completed.html";
  ASSERT_TRUE(std::filesystem::exists(index_path));
  ASSERT_TRUE(std::filesystem::exists(case_path));

  const auto index_html = readFile(index_path);
  const auto case_html = readFile(case_path);
  EXPECT_NE(index_html.find("linear_move_completed"), std::string::npos);
  EXPECT_NE(index_html.find("Mismatch"), std::string::npos);
  EXPECT_NE(case_html.find("Input G-code"), std::string::npos);
  EXPECT_NE(case_html.find("Options"), std::string::npos);
  EXPECT_NE(case_html.find("Driver"), std::string::npos);
  EXPECT_NE(case_html.find("Expected Trace"), std::string::npos);
  EXPECT_NE(case_html.find("Actual Trace"), std::string::npos);
  EXPECT_NE(case_html.find("N10 G1 X10"), std::string::npos);
  EXPECT_NE(case_html.find("deferred_m6"), std::string::npos);
  EXPECT_NE(case_html.find("resume_blocked"), std::string::npos);
}

} // namespace
