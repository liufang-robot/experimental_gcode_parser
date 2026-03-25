#include "gcode/gcode_parser.h"

int main() {
  const auto result = gcode::parse("G1 X1\n");
  return result.diagnostics.empty() ? 0 : 1;
}
