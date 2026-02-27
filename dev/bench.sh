#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build"
cmake --build "$ROOT_DIR/build" -j
"$ROOT_DIR/build/gcode_bench" --iterations 5 --lines 10000 \
  --output "$ROOT_DIR/output/bench/latest.json"
