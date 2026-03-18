#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build
cmake --build build -j --target gcode_execution_contract_review

./build/gcode_execution_contract_review \
  --fixtures-root testdata/execution_contract/core \
  --output-root output/execution_contract_review \
  --publish-root docs/src/generated/execution-contract-review

mdbook build docs
