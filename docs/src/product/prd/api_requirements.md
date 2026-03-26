# PRD: API Requirements

## 4. API Product Requirements

### 4.1 Main API Shape

Preferred public integration shape:

- a class facade as the main surface
- free-function entry points retained for compatibility

Target facade:

- `GCodeParser::parseText`
- `GCodeParser::parseFile`
- `GCodeParser::parseAndLowerText`
- `GCodeParser::parseAndLowerFile`
- streaming variants with callback or pull APIs
- cancellation and early-stop support

### 4.2 File Input Requirement

- library API must support reading directly from file paths
- file read failures must return diagnostics rather than crash

### 4.3 Output Requirement

Parser output:

- `ParseResult`
  - AST
  - diagnostics

Lowering output:

- `MessageResult`
  - typed messages
  - diagnostics
  - `rejected_lines`

Output access points:

- library return values
- optional JSON conversion helpers
- CLI output via `gcode_parse --format debug|json`
