#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found in PATH" >&2
  exit 1
fi

format_targets=()
while IFS= read -r -d '' file; do
  format_targets+=("$file")
done < <(find "$ROOT_DIR/src" "$ROOT_DIR/test" -type f \( -name "*.cpp" -o -name "*.h" \) -print0)

echo "==> clang-format (check)"
clang-format --dry-run -Werror "${format_targets[@]}"

echo "==> cmake configure (build)"
cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build"

echo "==> cmake build"
cmake --build "$ROOT_DIR/build" -j

"$ROOT_DIR/dev/clang_tidy_targets.sh" changed "$ROOT_DIR/build"

echo "==> ctest"
ctest --test-dir "$ROOT_DIR/build" --output-on-failure

echo "==> ctest fuzz smoke"
ctest --test-dir "$ROOT_DIR/build" --output-on-failure -R FuzzSmokeTest
