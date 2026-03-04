#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! command -v clang-tidy >/dev/null 2>&1; then
  echo "clang-tidy not found in PATH" >&2
  exit 1
fi

scope="${1:-all}"          # all | changed
build_dir="${2:-$ROOT_DIR/build}"
jobs="${CLANG_TIDY_JOBS:-$(nproc)}"

if [[ "$scope" != "all" && "$scope" != "changed" ]]; then
  echo "unsupported clang-tidy scope: $scope (expected all|changed)" >&2
  exit 1
fi

cd "$ROOT_DIR"

collect_all_targets() {
  find src test -type f \( -name "*.cpp" -o -name "*.h" \) | sort
}

resolve_base_ref() {
  if [[ -n "${GITHUB_BASE_REF:-}" ]]; then
    if ! git rev-parse --verify --quiet "origin/${GITHUB_BASE_REF}" >/dev/null; then
      git fetch --no-tags --depth=1 origin "${GITHUB_BASE_REF}" || true
    fi
    if git rev-parse --verify --quiet "origin/${GITHUB_BASE_REF}" >/dev/null; then
      echo "origin/${GITHUB_BASE_REF}"
      return 0
    fi
  fi

  if git rev-parse --verify --quiet origin/main >/dev/null; then
    echo "origin/main"
    return 0
  fi

  if git rev-parse --verify --quiet HEAD~1 >/dev/null; then
    echo "HEAD~1"
    return 0
  fi

  echo ""
}

collect_changed_targets() {
  local base_ref
  base_ref="$(resolve_base_ref)"
  if [[ -z "$base_ref" ]]; then
    return 0
  fi
  git diff --name-only "${base_ref}...HEAD" -- src test \
    | rg -n ".*\\.(cpp|h)$" --no-line-number \
    | while IFS= read -r file; do
        [[ -f "$file" ]] && printf "%s\n" "$file"
      done \
    | sort -u
}

mapfile -t targets < <(
  if [[ "$scope" == "changed" ]]; then
    collect_changed_targets
  else
    collect_all_targets
  fi
)

if [[ "${#targets[@]}" -eq 0 ]]; then
  if [[ "$scope" == "changed" ]]; then
    echo "==> clang-tidy (changed): no changed C++ targets, skipping"
    exit 0
  fi
  echo "no C++ targets found for clang-tidy" >&2
  exit 1
fi

echo "==> clang-tidy (${scope}, files=${#targets[@]}, jobs=${jobs})"
printf '%s\0' "${targets[@]}" | xargs -0 -n1 -P "$jobs" clang-tidy -p "$build_dir"

