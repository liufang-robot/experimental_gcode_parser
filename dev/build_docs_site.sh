#!/usr/bin/env bash
set -euo pipefail

if command -v apt-get >/dev/null 2>&1; then
  if ! dpkg -s libgtest-dev >/dev/null 2>&1; then
    sudo apt-get update
    sudo apt-get install -y libgtest-dev
  fi
fi

cmake -S . -B build
cmake --build build -j --target gcode_execution_contract_review

./build/gcode_execution_contract_review \
  --fixtures-root testdata/execution_contract/core \
  --output-root output/execution_contract_review \
  --publish-root docs/src/generated/execution-contract-review

mdbook build docs
