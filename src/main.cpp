#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "ast_printer.h"
#include "gcode_parser.h"

int main(int argc, const char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: gcode_parse <file>\n";
    return 2;
  }

  std::ifstream input_file(argv[1], std::ios::in);
  if (!input_file) {
    std::cerr << "Failed to open file: " << argv[1] << "\n";
    return 2;
  }

  std::stringstream buffer;
  buffer << input_file.rdbuf();
  auto result = gcode::parse(buffer.str());
  std::cout << gcode::format(result);
  return result.diagnostics.empty() ? 0 : 1;
}
