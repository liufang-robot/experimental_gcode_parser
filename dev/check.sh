#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found in PATH" >&2
  exit 1
fi

if ! command -v clang-tidy >/dev/null 2>&1; then
  echo "clang-tidy not found in PATH" >&2
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

echo "==> clang-tidy"
for file in "${format_targets[@]}"; do
  clang-tidy -p "$ROOT_DIR/build" "$file"
done

echo "==> cmake build"
cmake --build "$ROOT_DIR/build" -j

echo "==> ctest"
ctest --test-dir "$ROOT_DIR/build" --output-on-failure

echo "==> cmake configure (sanitizers)"
cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build-sanitize" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"

echo "==> cmake build (sanitizers)"
cmake --build "$ROOT_DIR/build-sanitize" -j

echo "==> ctest (sanitizers)"
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir "$ROOT_DIR/build-sanitize" --output-on-failure
