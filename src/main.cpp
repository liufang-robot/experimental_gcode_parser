#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "ast_printer.h"
#include "gcode_parser.h"

int main(int argc, const char **argv) {
  std::string format = "debug";
  std::string file_path;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--format") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --format (expected json|debug)\n";
        return 2;
      }
      format = argv[++i];
      if (format != "json" && format != "debug") {
        std::cerr << "Unsupported format: " << format
                  << " (expected json|debug)\n";
        return 2;
      }
      continue;
    }

    if (!file_path.empty()) {
      std::cerr << "Unexpected extra positional argument. Usage: gcode_parse "
                   "[--format json|debug] <file>\n";
      return 2;
    }
    file_path = arg;
  }

  if (file_path.empty()) {
    std::cerr << "Usage: gcode_parse [--format json|debug] <file>\n";
    return 2;
  }

  std::ifstream input_file(file_path, std::ios::in);
  if (!input_file) {
    std::cerr << "Failed to open file: " << file_path << "\n";
    return 2;
  }

  std::stringstream buffer;
  buffer << input_file.rdbuf();
  auto result = gcode::parse(buffer.str());
  if (format == "json") {
    std::cout << gcode::formatJson(result);
  } else {
    std::cout << gcode::format(result);
  }
  return result.diagnostics.empty() ? 0 : 1;
}
