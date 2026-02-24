# SPEC — CNC G-code Parser Library (v0.1)

## 1. Purpose
Parse G-code text (string or file) into a simple, syntax-level JSON model
focused on G1 (linear) and G2/G3 (circular) motion commands.

v0.1 is **syntax-only** (no execution, no full modal semantics).

## 2. Input / Output

### 2.1 Input
- UTF-8 text
- Line endings: `\n` or `\r\n`
- Tabs are whitespace
- Comments:
  - Semicolon: `; ...` to end-of-line
  - Parentheses: `( ... )` with no nesting

### 2.2 Output (Library + CLI)
The parser returns:

- AST (program → lines → items)
- Diagnostics with line/column (1-based)

Optional downstream stage:
- AST can be lowered into typed queue messages.
- v0 supports `G1Message`, `G2Message`, and `G3Message` emission.
- Message results support JSON conversion (`toJson`/`fromJson`) for transport,
  fixtures, and debugging.

The `gcode_parse` CLI supports:
- `--format debug` (default): stable, line-oriented debug format used by
  parser golden tests.
- `--format json`: machine-readable JSON with top-level keys:
  `schema_version`, `program`, and `diagnostics`.

AST shape (v0.1):
- Program: ordered list of `Line`
- Line:
  - `block_delete` (optional `/`)
  - `line_number` (`N` + integer)
  - `items` (ordered `Word` or `Comment`)
- Word:
  - `text` (raw token)
  - `head` (uppercased word head, e.g. `G`, `X`, `AP`, `I1`, `CIP`)
  - `value` (optional string; numeric or `AC(...)`)
  - `has_equal` (whether `=` was present)
- Comment:
  - `text` (raw token)

## 3. Supported Syntax (from testdata)

### 3.1 Line structure
- Optional block delete: `/`
- Optional line number: `N` + integer
- One motion command per line (GGroup1)
- Word letters are case-insensitive (`x` == `X`, `g1` == `G1`).

### 3.2 Numbers
- Integers: `0`, `10`, `-3`, `+7`
- Decimals: `1.0`, `-2.5`, `.5`, `5.`, `+.25`
- No scientific notation in v0.1

### 3.3 G1 (linear interpolation)
- Motion word: `G1`
- Cartesian endpoint: `X`, `Y`, `Z`, optional `A`
- Polar endpoint: `AP=...` and `RP=...`
- Feedrate: `F...`
- Mixed Cartesian + polar in one line is an error

Examples (from `testdata/g1_samples.ngc`):
```
G1 Z-2 F40
G1 X100 Y100 Z200
G1 AP=90 RP=10 F40
G1 G94 X100 Y20 Z30 A40 F100
G1 X10 AP=90 RP=10   ; mixed modes should error
```

### 3.4 G2 / G3 (circular interpolation)
- Motion words: `G2` (CW), `G3` (CCW)
- Cartesian endpoint: `X`, `Y`, `Z`
- Center point: `I`, `J`, `K`
  - Optional `=` (e.g., `I-43` or `I=-43`)
  - Absolute form with `AC(...)` (e.g., `I=AC(90)`)
- Radius: `CR=...`
- Opening angle: `AR=...`
- Polar endpoint: `AP=... RP=...`
- Intermediate point: `CIP` with `I1/J1/K1`
  - Optional `=` and `AC(...)`
- Tangential circle: `CT` with `X/Y/Z`

Examples (from `testdata/g2g3_samples.ngc`):
```
N30 G2 X115 Y113.3 I=AC(90) J=AC(70)
N30 G2 X115 Y113.3 CR=-50
N30 G2 AR=269.31 I-43 J25.52
N30 G2 AR=269.31 X115 Y113.3
G2 AP=90 RP=10
G2 CIP X10 Y5 Z0 I1=AC(1) J1=AC(2) K1=AC(3)
G2 CT X10 Y5 Z0
```

## 4. Non-goals (v0.1)
- Full modal group validation beyond GGroup1 single-motion rule
- Units/distance-mode state
- Full RS274NGC expressions/macros
- Subprograms and flow control

## 5. Diagnostics (v0.1)
- Syntax errors reported by the lexer/parser with line/column.
  - Messages should be actionable and prefixed with `syntax error:`.
- Semantic errors:
  - Mixed Cartesian (`X/Y/Z/A`) + polar (`AP/RP`) words in a `G1` line.
  - Multiple motion commands (`G1/G2/G3`) in a single line.
  - Semantic messages should include an explicit correction hint (for example,
    "choose one coordinate mode" or "choose only one of G1/G2/G3").

## 6. Message Lowering (v0.1)
- Standalone lowering stage: AST + parser diagnostics -> queue-ready messages +
  diagnostics.
- `G1Message` fields:
  - Source: optional filename, physical line, optional `N` line number
  - Target pose: `X/Y/Z/A/B/C` (optional numeric fields)
  - Feed: `F` (optional numeric field)
- `G2Message` / `G3Message` fields:
  - Source: optional filename, physical line, optional `N` line number
  - Target pose: `X/Y/Z/A/B/C` (optional numeric fields)
  - Arc parameters: `I/J/K/R` (optional numeric fields; `CR` maps to `R`)
  - Feed: `F` (optional numeric field)
- Error-line emission policy (v0):
  - If a line has error diagnostics, do not emit motion message for that line;
    the line is reported in `rejected_lines` with reasons.
  - Lowering is fail-fast: stop lowering when the first error line is encountered.
- Severity mapping policy (v0):
  - `error`: syntax/semantic violations that reject a line and trigger fail-fast stop.
  - `warning`: non-fatal lowering limits (for example unsupported arc words);
    the message is still emitted from supported fields.
- Resume session API (v0):
  - Provide session-level edit + resume from line API for interactive workflows.
  - After line edit, reparsing preserves deterministic message ordering and
    stable source line mapping.
- Queue diff/apply API (v0):
  - Provide line-keyed message diff with `added`, `updated`, and
    `removed_lines`.
  - Provide apply helper that applies a diff to an existing message queue and
    preserves deterministic line order.
- JSON schema notes:
  - Include top-level `schema_version` (current value: `1`).
  - Include `messages`, `diagnostics`, and `rejected_lines`.
  - `G1Message` JSON carries `type`, `source`, `target_pose`, and `feed`.
  - `G2Message` / `G3Message` JSON additionally carry `arc` with
    `i/j/k/r` optional numeric fields.

## 7. Testing Expectations
- Golden tests for all examples in `SPEC.md`.
- Unit test framework: GoogleTest (`gtest`) is the required framework for new
  unit tests.
- Add message-output JSON golden tests for representative queue outputs.
  - Message fixture naming convention:
    - input: `<name>.ngc`
    - golden: `<name>.golden.json`
    - for no-filename lowering cases, use `nofilename_<name>.ngc`.
- Property/fuzz testing: parser must never crash, hang, or use unbounded memory.
  - Minimum smoke gate: deterministic corpus + fixed-seed generated inputs in
    `test/fuzz_smoke_tests.cpp`, bounded to max input length 256 and 3000
    generated cases.
  - Runtime budget for CI smoke gate: under 5 seconds per test invocation.
- Regression tests: every fixed bug must get a test that fails first, then passes.
  - Regression test naming convention:
    `Regression_<bug_or_issue_id>_<short_behavior>`.

## 8. Code Architecture Requirements
- Use object-oriented module design for parser/lowering function families.
- Each major function family (`G1`, `G2/G3`, and future families) must be
  implemented in separate classes/files.
- Do not implement all parser/lowering family behavior in a single monolithic
  source file.
- New function families must be added as new modules rather than extending one
  large file with long branching chains.
