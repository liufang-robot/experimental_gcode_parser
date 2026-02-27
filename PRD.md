# PRD — CNC G-code Parser Library

## 1. Product Purpose
Provide a production-usable C++ library that converts G-code text/files into:
- syntax/semantic diagnostics with precise source locations
- typed queue-ready messages for downstream motion systems

Primary users:
- CNC application/backend developers integrating parsing into planners,
  simulators, editors, and execution pipelines.

## 2. Product Goals
- Deterministic parsing and lowering results for identical input.
- Actionable diagnostics (line/column + correction hints).
- Stable API surface for integrators.
- Test-first delivery: golden + regression + fuzz smoke.

## 3. Non-Goals
- Executing machine motion.
- Full controller runtime semantics (override, real-time spindle dynamics).
- Full macro/subprogram/control-flow implementation in v0.

## 4. API Product Requirements

### 4.1 Main API Shape
Recommendation:
- expose a class facade as the main public integration surface, while keeping
  free-function entry points for backward compatibility.

Required facade (target shape):
- `class GCodeParser`
  - `ParseResult parseText(std::string_view input, const ParseOptions&)`
  - `ParseResult parseFile(const std::string& path, const ParseOptions&)`
  - `MessageResult parseAndLowerText(std::string_view input, const LowerOptions&)`
  - `MessageResult parseAndLowerFile(const std::string& path, const LowerOptions&)`

Compatibility policy:
- Existing free APIs remain supported unless a major-version change is approved.

### 4.2 File Input Requirement
- Library API must support reading directly from file path (not only string).
- File read failures must return diagnostics (not crashes).

### 4.3 Output Requirement
Parser output:
- `ParseResult`
  - AST (`Program -> Line -> Item`)
  - diagnostics (`severity`, `message`, `location`)

Lowering output:
- `MessageResult`
  - typed `messages` queue entries (`G1/G2/G3/G4` currently)
  - accumulated diagnostics
  - `rejected_lines` for fail-fast policy

Output access points:
- library return values from parse/lower APIs
- optional JSON via message JSON conversion (`toJsonString` / `fromJsonString`)
- CLI output via `gcode_parse --format debug|json`

## 5. Functional Scope

### 5.1 Current Scope (v0)
- `G1`, `G2`, `G3`, `G4` syntax + diagnostics + lowering to typed messages.
- fail-fast lowering at first error line.
- JSON round-trip support for message results.

### 5.2 Next Scope (v0.2+)
- prioritized extension of additional G/M families.
- improved syntax coverage and diagnostics detail.
- API facade hardening and file-based API convenience.

## 6. Command Family Policy
- `G` codes:
  - Implement only command families explicitly listed in SPEC + backlog tasks.
- `M` codes:
  - parse visibility is allowed; lowering behavior must be explicitly specified
    before implementation.
- Vendor/extend codes:
  - require explicit spec entry + tests before treated as supported behavior.

## 7. Quality Requirements
- Must not crash/hang on malformed input.
- Must keep diagnostics and queue ordering deterministic.
- Must pass `./dev/check.sh` for merge.
- Must include regression tests for every bug fix.

## 8. Documentation and Traceability
- `PRD.md`: product goals, scope, API contract, prioritization.
- `SPEC.md`: exact syntax/diagnostics/lowering behavior contract.
- `PROGRAM_REFERENCE.md`: implementation snapshot (`Implemented/Partial/Planned`).
- `BACKLOG.md`: executable tasks and acceptance criteria.

Every feature PR must update all impacted docs in the same PR.

## 9. Release Acceptance (v0)
- API usable for text and file workflows.
- Supported function families documented and test-proven.
- No known P0/P1 correctness or crash issues open.
- CI and `./dev/check.sh` green.
