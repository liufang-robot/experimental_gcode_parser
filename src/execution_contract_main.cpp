#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "execution_contract_fixture.h"
#include "execution_contract_html.h"
#include "execution_contract_runner.h"

namespace {

void printUsage() {
  std::cerr << "Usage: gcode_execution_contract_review "
               "[--fixtures-root <dir>] [--output-root <dir>] "
               "[--publish-root <dir>]\n";
}

std::string stripEventsSuffix(const std::string &filename) {
  const std::string suffix = ".events.yaml";
  if (filename.size() >= suffix.size() &&
      filename.compare(filename.size() - suffix.size(), suffix.size(),
                       suffix) == 0) {
    return filename.substr(0, filename.size() - suffix.size());
  }
  return filename;
}

} // namespace

int main(int argc, char **argv) {
  std::string fixtures_root = "testdata/execution_contract/core";
  std::string output_root = "output/execution_contract_review";
  std::string publish_root;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--fixtures-root") {
      if (i + 1 >= argc) {
        printUsage();
        return 2;
      }
      fixtures_root = argv[++i];
      continue;
    }
    if (arg == "--output-root") {
      if (i + 1 >= argc) {
        printUsage();
        return 2;
      }
      output_root = argv[++i];
      continue;
    }
    if (arg == "--publish-root") {
      if (i + 1 >= argc) {
        printUsage();
        return 2;
      }
      publish_root = argv[++i];
      continue;
    }
    printUsage();
    return 2;
  }

  std::vector<gcode::ExecutionContractCaseReport> reports;
  bool all_match = true;

  for (const auto &entry : std::filesystem::directory_iterator(fixtures_root)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto filename = entry.path().filename().string();
    if (entry.path().extension() != ".yaml" ||
        filename.find(".events.yaml") == std::string::npos) {
      continue;
    }

    const std::string case_name = stripEventsSuffix(filename);
    const std::string program_path =
        (entry.path().parent_path() / (case_name + ".ngc")).string();
    const auto reference =
        gcode::loadExecutionContractTrace(entry.path().string());
    const auto actual = gcode::runExecutionContractFixture(
        program_path, reference,
        (std::filesystem::path(output_root) /
         entry.path().parent_path().filename() / (case_name + ".actual.yaml"))
            .string());
    const bool matches =
        gcode::executionContractTracesEqual(actual.actual_trace, reference);
    all_match = all_match && matches;

    gcode::ExecutionContractCaseReport report;
    report.suite_name = entry.path().parent_path().filename().string();
    report.case_name = case_name;
    report.description = reference.description.value_or("");
    report.program_path = program_path;
    report.reference_path = entry.path().string();
    report.actual_path = actual.actual_output_path;
    report.matches_reference = matches;
    report.reference_trace = reference;
    report.actual_trace = actual.actual_trace;
    reports.push_back(std::move(report));
  }

  const std::filesystem::path site_root =
      publish_root.empty() ? std::filesystem::path(output_root) / "site"
                           : std::filesystem::path(publish_root);
  gcode::writeExecutionContractHtmlSite(reports, site_root.string());

  std::cout << "Generated execution contract review for " << reports.size()
            << " cases at " << site_root << "\n";
  return all_match ? 0 : 1;
}
