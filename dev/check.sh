#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="full"

if [[ "${1:-}" == "--fast" ]]; then
  MODE="fast"
  shift
fi

if [[ $# -gt 0 ]]; then
  echo "usage: $0 [--fast]" >&2
  exit 1
fi

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

echo "==> cmake build"
cmake --build "$ROOT_DIR/build" -j

echo "==> clang-tidy"
tidy_targets=("${format_targets[@]}")
if [[ "$MODE" == "fast" ]]; then
  tidy_targets=()
  declare -A seen_targets=()
  changed_files=()
  while IFS= read -r rel; do
    changed_files+=("$rel")
  done < <(git -C "$ROOT_DIR" diff --name-only --diff-filter=ACMRTUXB HEAD --)
  while IFS= read -r rel; do
    changed_files+=("$rel")
  done < <(git -C "$ROOT_DIR" ls-files --others --exclude-standard -- src test)

  for rel in "${changed_files[@]}"; do
    [[ -z "$rel" ]] && continue
    case "$rel" in
    src/*.cpp | src/*.h | test/*.cpp | test/*.h) ;;
    *) continue ;;
    esac
    abs="$ROOT_DIR/$rel"
    [[ -f "$abs" ]] || continue
    if [[ -z "${seen_targets["$abs"]+x}" ]]; then
      tidy_targets+=("$abs")
      seen_targets["$abs"]=1
    fi
  done
fi

if [[ ${#tidy_targets[@]} -eq 0 ]]; then
  if [[ "$MODE" == "fast" ]]; then
    echo "no changed C++ files under src/ or test/; skipping clang-tidy"
  fi
else
  if command -v nproc >/dev/null 2>&1; then
    tidy_jobs="$(nproc)"
  elif command -v sysctl >/dev/null 2>&1; then
    tidy_jobs="$(sysctl -n hw.ncpu)"
  else
    tidy_jobs=4
  fi
  tidy_jobs="${CLANG_TIDY_JOBS:-$tidy_jobs}"
  printf '%s\0' "${tidy_targets[@]}" |
    xargs -0 -n 1 -P "$tidy_jobs" clang-tidy -p "$ROOT_DIR/build"
fi

echo "==> ctest"
ctest --test-dir "$ROOT_DIR/build" --output-on-failure

echo "==> ctest fuzz smoke"
ctest --test-dir "$ROOT_DIR/build" --output-on-failure -R FuzzSmokeTest

echo "==> cmake configure (sanitizers)"
cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build-sanitize" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"

echo "==> cmake build (sanitizers)"
cmake --build "$ROOT_DIR/build-sanitize" -j

echo "==> ctest (sanitizers)"
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir "$ROOT_DIR/build-sanitize" --output-on-failure

echo "==> ctest fuzz smoke (sanitizers)"
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir "$ROOT_DIR/build-sanitize" --output-on-failure -R FuzzSmokeTest
