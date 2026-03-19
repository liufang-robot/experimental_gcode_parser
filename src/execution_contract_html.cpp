#include "execution_contract_html.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace gcode {
namespace {

std::string readFileIfExists(const std::string &path) {
  std::ifstream input(path);
  if (!input) {
    return {};
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

std::string escapeHtml(const std::string &text) {
  std::string escaped;
  escaped.reserve(text.size());
  for (const char ch : text) {
    switch (ch) {
    case '&':
      escaped += "&amp;";
      break;
    case '<':
      escaped += "&lt;";
      break;
    case '>':
      escaped += "&gt;";
      break;
    case '"':
      escaped += "&quot;";
      break;
    default:
      escaped.push_back(ch);
      break;
    }
  }
  return escaped;
}

void writeFile(const std::filesystem::path &path, const std::string &contents) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path);
  out << contents;
}

std::string prettyJson(const nlohmann::ordered_json &j) { return j.dump(2); }

std::string casePageHtml(const ExecutionContractCaseReport &report) {
  std::ostringstream out;
  const auto reference_json =
      executionContractTraceToJson(report.reference_trace);
  out << "<!doctype html><html><head><meta charset=\"utf-8\">";
  out << "<title>" << escapeHtml(report.case_name) << "</title>";
  out << "<style>body{font-family:sans-serif;max-width:1200px;margin:2rem "
         "auto;padding:0 "
         "1rem;}pre{background:#f5f5f5;padding:1rem;overflow:auto;}table{"
         "border-collapse:collapse;}td,th{border:1px "
         "solid #ddd;padding:0.5rem;} .status-match{color:#0a7d2c;} "
         ".status-mismatch{color:#b00020;}</style></head><body>";
  out << "<h1>" << escapeHtml(report.case_name) << "</h1>";
  out << "<p>" << escapeHtml(report.description) << "</p>";
  out << "<p>Status: <strong class=\""
      << (report.matches_reference ? "status-match\">Match"
                                   : "status-mismatch\">Mismatch")
      << "</strong></p>";
  out << "<h2>Input G-code</h2><pre>"
      << escapeHtml(readFileIfExists(report.program_path)) << "</pre>";
  out << "<h2>Options</h2><pre>"
      << escapeHtml(prettyJson(reference_json["options"])) << "</pre>";
  if (reference_json.contains("driver") &&
      reference_json["driver"].is_array()) {
    out << "<h2>Driver</h2><pre>"
        << escapeHtml(prettyJson(reference_json["driver"])) << "</pre>";
  }
  if (reference_json.contains("runtime") &&
      !reference_json["runtime"].is_null()) {
    out << "<h2>Runtime Inputs</h2><pre>"
        << escapeHtml(prettyJson(reference_json["runtime"])) << "</pre>";
  }
  out << "<h2>Expected Trace</h2><pre>"
      << escapeHtml(serializeExecutionContractTrace(report.reference_trace))
      << "</pre>";
  out << "<h2>Actual Trace</h2><pre>"
      << escapeHtml(serializeExecutionContractTrace(report.actual_trace))
      << "</pre>";
  out << "</body></html>";
  return out.str();
}

std::string
indexPageHtml(const std::vector<ExecutionContractCaseReport> &cases) {
  std::ostringstream out;
  out << "<!doctype html><html><head><meta charset=\"utf-8\">";
  out << "<title>Execution Contract Review</title>";
  out << "<style>body{font-family:sans-serif;max-width:1200px;margin:2rem "
         "auto;padding:0 1rem;}table{border-collapse:collapse;width:100%;}"
         "td,th{border:1px solid #ddd;padding:0.5rem;text-align:left;}"
         ".status-match{color:#0a7d2c;} .status-mismatch{color:#b00020;}"
         "</style></head><body>";
  out << "<h1>Execution Contract Review</h1>";
  out << "<table><thead><tr><th>Suite</th><th>Case</th><th>Description</th>"
      << "<th>Status</th></tr></thead><tbody>";
  for (const auto &report : cases) {
    out << "<tr><td>" << escapeHtml(report.suite_name) << "</td><td><a href=\""
        << escapeHtml(report.suite_name + "/" + report.case_name + ".html")
        << "\">" << escapeHtml(report.case_name) << "</a></td><td>"
        << escapeHtml(report.description) << "</td><td class=\""
        << (report.matches_reference ? "status-match\">Match"
                                     : "status-mismatch\">Mismatch")
        << "</td></tr>";
  }
  out << "</tbody></table></body></html>";
  return out.str();
}

} // namespace

void writeExecutionContractHtmlSite(
    const std::vector<ExecutionContractCaseReport> &cases,
    const std::string &output_root) {
  const std::filesystem::path root(output_root);
  writeFile(root / "index.html", indexPageHtml(cases));
  for (const auto &report : cases) {
    const auto case_dir = root / report.suite_name;
    writeFile(case_dir / (report.case_name + ".html"), casePageHtml(report));
    writeFile(case_dir / (report.case_name + ".ngc"),
              readFileIfExists(report.program_path));
    writeFile(case_dir / (report.case_name + ".events.yaml"),
              serializeExecutionContractTrace(report.reference_trace));
    writeFile(case_dir / (report.case_name + ".actual.yaml"),
              serializeExecutionContractTrace(report.actual_trace));
  }
}

} // namespace gcode
