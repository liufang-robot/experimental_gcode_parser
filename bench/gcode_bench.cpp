#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "gcode_parser.h"
#include "messages.h"

namespace {

struct BenchScenarioResult {
  std::string name;
  size_t lines = 0;
  size_t bytes = 0;
  int iterations = 0;
  double parse_ms_avg = 0.0;
  double parse_and_lower_ms_avg = 0.0;
  double parse_lines_per_sec = 0.0;
  double parse_and_lower_lines_per_sec = 0.0;
  double parse_bytes_per_sec = 0.0;
  double parse_and_lower_bytes_per_sec = 0.0;
};

std::string makeProgram(size_t line_count) {
  std::string text;
  text.reserve(line_count * 18);
  for (size_t i = 0; i < line_count; ++i) {
    text += "N";
    text += std::to_string(i + 1);
    text += " G1 X1.0 Y2.0 F3\n";
  }
  return text;
}

size_t countLines(const std::string &text) {
  size_t lines = 0;
  for (char c : text) {
    if (c == '\n') {
      ++lines;
    }
  }
  return lines;
}

BenchScenarioResult runScenario(const std::string &name,
                                const std::string &input, int iterations) {
  BenchScenarioResult result;
  result.name = name;
  result.lines = countLines(input);
  result.bytes = input.size();
  result.iterations = iterations;

  double parse_total_ms = 0.0;
  double parse_and_lower_total_ms = 0.0;

  for (int i = 0; i < iterations; ++i) {
    const auto parse_start = std::chrono::steady_clock::now();
    const auto parsed = gcode::parse(input);
    const auto parse_end = std::chrono::steady_clock::now();
    parse_total_ms +=
        std::chrono::duration<double, std::milli>(parse_end - parse_start)
            .count();

    const auto lower_start = std::chrono::steady_clock::now();
    const auto lowered = gcode::parseAndLower(input);
    const auto lower_end = std::chrono::steady_clock::now();
    parse_and_lower_total_ms +=
        std::chrono::duration<double, std::milli>(lower_end - lower_start)
            .count();

    if (!parsed.diagnostics.empty()) {
      std::cerr << "benchmark warning: parse diagnostics count="
                << parsed.diagnostics.size() << "\n";
    }
    if (!lowered.diagnostics.empty()) {
      std::cerr << "benchmark warning: lower diagnostics count="
                << lowered.diagnostics.size() << "\n";
    }
  }

  result.parse_ms_avg = parse_total_ms / static_cast<double>(iterations);
  result.parse_and_lower_ms_avg =
      parse_and_lower_total_ms / static_cast<double>(iterations);

  const double lines = static_cast<double>(result.lines);
  const double bytes = static_cast<double>(result.bytes);
  const double parse_sec = result.parse_ms_avg / 1000.0;
  const double parse_lower_sec = result.parse_and_lower_ms_avg / 1000.0;

  result.parse_lines_per_sec = parse_sec > 0.0 ? lines / parse_sec : 0.0;
  result.parse_and_lower_lines_per_sec =
      parse_lower_sec > 0.0 ? lines / parse_lower_sec : 0.0;
  result.parse_bytes_per_sec = parse_sec > 0.0 ? bytes / parse_sec : 0.0;
  result.parse_and_lower_bytes_per_sec =
      parse_lower_sec > 0.0 ? bytes / parse_lower_sec : 0.0;

  return result;
}

void writeResultJson(const std::string &out_path,
                     const BenchScenarioResult &scenario) {
  nlohmann::json j;
  j["schema_version"] = 1;
  j["scenarios"] = nlohmann::json::array();

  nlohmann::json s;
  s["name"] = scenario.name;
  s["iterations"] = scenario.iterations;
  s["lines"] = scenario.lines;
  s["bytes"] = scenario.bytes;
  s["parse_ms_avg"] = scenario.parse_ms_avg;
  s["parse_and_lower_ms_avg"] = scenario.parse_and_lower_ms_avg;
  s["parse_lines_per_sec"] = scenario.parse_lines_per_sec;
  s["parse_and_lower_lines_per_sec"] = scenario.parse_and_lower_lines_per_sec;
  s["parse_bytes_per_sec"] = scenario.parse_bytes_per_sec;
  s["parse_and_lower_bytes_per_sec"] = scenario.parse_and_lower_bytes_per_sec;
  j["scenarios"].push_back(s);

  if (!out_path.empty()) {
    std::filesystem::path output_path(out_path);
    std::filesystem::create_directories(output_path.parent_path());
    std::ofstream out(output_path, std::ios::out | std::ios::trunc);
    out << j.dump(2) << "\n";
  }

  std::cout << j.dump(2) << "\n";
}

} // namespace

int main(int argc, char **argv) {
  int iterations = 5;
  size_t lines = 10000;
  std::string out_path = "output/bench/latest.json";

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--iterations" && i + 1 < argc) {
      iterations = std::stoi(argv[++i]);
    } else if (arg == "--lines" && i + 1 < argc) {
      lines = static_cast<size_t>(std::stoul(argv[++i]));
    } else if (arg == "--output" && i + 1 < argc) {
      out_path = argv[++i];
    }
  }

  if (iterations <= 0 || lines == 0) {
    std::cerr << "invalid benchmark parameters" << std::endl;
    return 1;
  }

  const auto scenario =
      runScenario("synthetic_g1_10k", makeProgram(lines), iterations);
  writeResultJson(out_path, scenario);
  return 0;
}
