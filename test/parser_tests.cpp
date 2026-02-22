#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ast_printer.h"
#include "gcode_parser.h"

namespace {

struct TestFailure {
  std::string message;
};

bool expectEqual(const std::string &label, const std::string &actual,
                 const std::string &expected,
                 std::vector<TestFailure> &failures) {
  if (actual == expected) {
    return true;
  }
  std::ostringstream out;
  out << label << " failed\n";
  out << "Expected:\n" << expected << "\n";
  out << "Actual:\n" << actual << "\n";
  failures.push_back({out.str()});
  return false;
}

std::string readFile(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::in);
  std::stringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

bool runGolden(const std::filesystem::path &input_path,
               const std::filesystem::path &golden_path,
               std::vector<TestFailure> &failures) {
  auto input = readFile(input_path);
  auto expected = readFile(golden_path);
  auto result = gcode::parse(input);
  auto actual = gcode::format(result);
  return expectEqual("golden " + input_path.filename().string(), actual,
                     expected, failures);
}

} // namespace

int main() {
  std::vector<TestFailure> failures;

  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto testdata = source_dir / "testdata";

  runGolden(testdata / "g1_samples.ngc", testdata / "g1_samples.golden.txt",
            failures);
  runGolden(testdata / "g2g3_samples.ngc", testdata / "g2g3_samples.golden.txt",
            failures);
  runGolden(testdata / "sample1.ngc", testdata / "sample1.golden.txt",
            failures);

  if (!failures.empty()) {
    for (const auto &failure : failures) {
      std::cerr << failure.message << "\n";
    }
    return 1;
  }

  return 0;
}
