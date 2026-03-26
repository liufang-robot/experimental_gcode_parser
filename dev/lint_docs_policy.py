#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs" / "src"

ALLOWED_ROOT_MD = {"README.md", "AGENTS.md", "CHANGELOG_AGENT.md"}
EXPECTED_TOP_LEVEL = [
    "- [Overview](index.md)",
    "- [Requirements](requirements/index.md)",
    "- [Product Reference](product/index.md)",
    "- [Execution Workflow](execution_workflow.md)",
    "- [Execution Contract Review](execution_contract_review.md)",
    "- [Development Reference](development/index.md)",
    "- [Project Planning](project/index.md)",
]
PRODUCT_FORBIDDEN_PATTERNS = [
    r"test/messages_tests\.cpp",
    r"test/streaming_tests\.cpp",
    r"testdata/execution_contract/core/",
]
MAX_AGENTS_LINES = 80
MAX_INDEX_LINES = 120
MAX_DOC_LINES = 450
SIZE_EXCEPTIONS = {
    DOCS / "development" / "design" / "implementation_plan_from_requirements.md",
    DOCS / "development" / "design" / "streaming_execution_architecture.md",
}


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def fail(errors: list[str], msg: str) -> None:
    errors.append(msg)


def line_count(path: Path) -> int:
    return read(path).count("\n") + 1


def check_root_markdown(errors: list[str]) -> None:
    for path in sorted(ROOT.glob("*.md")):
        if path.name not in ALLOWED_ROOT_MD:
            fail(errors, f"root markdown file not allowed by docs policy: {path.name}")


def check_agents(errors: list[str]) -> None:
    path = ROOT / "AGENTS.md"
    text = read(path)
    if line_count(path) > MAX_AGENTS_LINES:
        fail(errors, f"AGENTS.md exceeds {MAX_AGENTS_LINES} lines")
    required = [
        "startup map, not the full documentation body",
        "developer",
        "integration tester",
        "./dev/check.sh",
        "docs/src/index.md",
    ]
    for needle in required:
        if needle not in text:
            fail(errors, f"AGENTS.md missing required guidance: {needle}")


def check_summary(errors: list[str]) -> None:
    path = DOCS / "SUMMARY.md"
    text = read(path)
    positions = []
    for line in EXPECTED_TOP_LEVEL:
        pos = text.find(line)
        if pos == -1:
            fail(errors, f"SUMMARY.md missing required top-level entry: {line}")
        positions.append(pos)
    if any(a >= b for a, b in zip(positions, positions[1:])):
        fail(errors, "SUMMARY.md top-level chapter order does not match docs policy")


def check_required_pages(errors: list[str]) -> None:
    required = [
        DOCS / "index.md",
        DOCS / "product" / "index.md",
        DOCS / "development" / "index.md",
        DOCS / "development" / "docs_policy.md",
        DOCS / "requirements" / "index.md",
        ROOT / ".agents" / "skills" / "write-docs" / "SKILL.md",
    ]
    for path in required:
        if not path.exists():
            fail(errors, f"required docs page missing: {path.relative_to(ROOT)}")


def check_product_content(errors: list[str]) -> None:
    product_dir = DOCS / "product" / "program_reference"
    for path in product_dir.rglob("*.md"):
        text = read(path)
        for pattern in PRODUCT_FORBIDDEN_PATTERNS:
            if re.search(pattern, text):
                fail(errors, f"program reference contains maintainer/test-inventory content: {path.relative_to(ROOT)}")


def check_sizes(errors: list[str]) -> None:
    for path in DOCS.rglob("*.md"):
        count = line_count(path)
        if path.name == "index.md" and count > MAX_INDEX_LINES:
            fail(errors, f"index page exceeds {MAX_INDEX_LINES} lines: {path.relative_to(ROOT)}")
        elif path not in SIZE_EXCEPTIONS and count > MAX_DOC_LINES:
            fail(errors, f"doc exceeds {MAX_DOC_LINES} lines and should likely be split: {path.relative_to(ROOT)}")


def main() -> int:
    errors: list[str] = []
    check_root_markdown(errors)
    check_agents(errors)
    check_summary(errors)
    check_required_pages(errors)
    check_product_content(errors)
    check_sizes(errors)
    if errors:
        print("docs policy lint failed:", file=sys.stderr)
        for err in errors:
            print(f"- {err}", file=sys.stderr)
        return 1
    print("docs policy lint passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
