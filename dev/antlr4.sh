#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${ANTLR4_JAR:-}" ]]; then
  echo "ANTLR4_JAR is not set" >&2
  exit 1
fi

exec java -jar "$ANTLR4_JAR" "$@"
