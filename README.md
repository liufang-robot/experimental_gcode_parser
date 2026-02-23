# G1 Parser (C++ / ANTLR4)

This directory contains a minimal G1-focused grammar and a small example
executable that parses a test file and prints the parse tree.

## Generate C++ sources

The build will run `antlr4` automatically. You can also generate manually:

```bash
export ANTLR4_TOOLS_ANTLR_VERSION=4.13.2
antlr4 -Dlanguage=Cpp -visitor -listener -o build/generated grammar/GCode.g4
```

## Build (requires ANTLR4 C++ runtime)

You need the ANTLR4 C++ runtime built or installed. Provide these variables if
the runtime isn't in a system path:

- `ANTLR4_RUNTIME_INCLUDE_DIR`
- `ANTLR4_RUNTIME_LIB`

Example (runtime installed under `/home/liufang/optcnc/install/`):

```bash
cmake -S . -B build \
  -DANTLR4_RUNTIME_INCLUDE_DIR=/home/liufang/optcnc/install/include/antlr4-runtime \
  -DANTLR4_RUNTIME_LIB=/home/liufang/optcnc/install/lib/libantlr4-runtime.a
cmake --build build
```

## Run example

```bash
./build/gcode_parse testdata/g1_samples.ngc
./build/gcode_parse --format debug testdata/g1_samples.ngc
./build/gcode_parse --format json testdata/g1_samples.ngc
```

## Tests

```bash
./dev/check.sh
```

Regression tests policy:
- Every fixed bug must add one regression test first (failing before the fix).
- Regression test names must use:
  - `Regression_<bug_or_issue_id>_<short_behavior>`
- Keep regression tests in `test/regression_tests.cpp` unless a dedicated suite
  is required for scale.
