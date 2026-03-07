#CHANGELOG_AGENT

## 2026-03-07 (T-050 slice 40: bare repeat-call executor case parity)
- Added executor coverage for lowercase and mixed-case bare alphabetic
  subprogram calls with repeat counts.
- Added executor coverage for lowercase and mixed-case bare zero-repeat calls
  to preserve the existing warning-and-ignore behavior.
- No behavior change in this slice; this locks runtime case parity for bare
  alphabetic repeat-call targets.

SPEC sections / tests:
- SPEC: Section 6.1
- Tests: `test/ail_executor_tests.cpp`

Known limitations:
- Inline subprogram call arguments remain compatibility-only syntax and are not
  modeled in baseline AIL.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-07 (T-050 slice 39: bare repeat-call case-parity coverage)
- Added parser coverage for lowercase and mixed-case bare alphabetic
  subprogram calls using both trailing and leading repeat-count forms.
- Added AIL coverage showing those forms normalize to the existing uppercase
  target representation while preserving `repeat_count`.
- No behavior change in this slice; this locks case-insensitive repeat-call
  parity for bare alphabetic targets.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`

Known limitations:
- Inline subprogram call arguments remain compatibility-only syntax and are not
  modeled in baseline AIL.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-07 (T-050 slice 38: quoted repeat-call executor coverage)
- Added executor coverage for quoted subprogram calls with repeat counts.
- Added executor coverage for quoted zero-repeat calls to preserve the existing
  warning-and-ignore behavior.
- No behavior change in this slice; this locks quoted repeat execution parity
  with unquoted call targets.

SPEC sections / tests:
- SPEC: Section 6.1
- Tests: `test/ail_executor_tests.cpp`

Known limitations:
- Inline subprogram call arguments remain compatibility-only syntax and are not
  modeled in baseline AIL.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-07 (T-050 slice 37: quoted repeat-call coverage)
- Added parser coverage for quoted subprogram calls using both trailing and
  leading repeat-count forms.
- Added AIL coverage for quoted repeat-call forms to preserve the existing
  target string and explicit `repeat_count`.
- No behavior change in this slice; this locks the current quoted repeat-call
  baseline.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`

Known limitations:
- Inline subprogram call arguments remain compatibility-only syntax and are not
  modeled in baseline AIL.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-07 (T-050 slice 36: bare subprogram call case-parity coverage)
- Added parser coverage for lowercase and mixed-case bare alphabetic
  subprogram call targets.
- Added AIL coverage showing lowercase and mixed-case bare alphabetic targets
  normalize to the existing uppercase call target representation.
- No behavior change in this slice; this captures the current
  case-insensitive direct-call baseline.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`

Known limitations:
- Inline subprogram call arguments remain compatibility-only syntax and are not
  modeled in baseline AIL.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-07 (T-050 slice 35: subprogram call whitespace case-parity coverage)
- Added AIL coverage for lowercase and mixed-case whitespace-separated direct
  subprogram call suffix forms.
- Empty ` ()` remains accepted without warnings, and non-empty ` ( ... )`
  remains ignored with deterministic call-arguments warnings across case
  variants.
- No behavior change in this slice; this captures existing case-insensitive
  lowering behavior for whitespace-separated direct-call compatibility forms.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/ail_tests.cpp`

Known limitations:
- Inline subprogram call arguments remain compatibility-only syntax and are not
  modeled in baseline AIL.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 34: quoted subprogram call whitespace coverage)
- Added AIL coverage for whitespace-separated quoted direct subprogram call
  suffix forms.
- Quoted targets now have explicit regression coverage for both compatibility
  forms: empty ` ()` accepted without warnings and non-empty ` ( ... )`
  continuing to emit deterministic call-arguments warnings.
- No behavior change in this slice; this captures existing lowering behavior
  for quoted whitespace-separated compatibility forms.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/ail_tests.cpp`

Known limitations:
- Inline subprogram call arguments remain compatibility-only syntax and are not
  modeled in baseline AIL.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 33: quoted subprogram call suffix coverage)
- Added AIL coverage for quoted direct subprogram calls with inline
  parenthesized suffixes.
- Empty `()` suffix remains accepted without warnings, and non-empty `(...)`
  remains ignored with the deterministic call-arguments warning for quoted
  targets as well.
- No behavior change in this slice; this captures existing lowering behavior
  for quoted direct-call compatibility forms.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/ail_tests.cpp`

Known limitations:
- Inline subprogram call arguments are still not modeled in baseline AIL and
  remain compatibility-only syntax when present.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 32: subprogram call suffix case-parity coverage)
- Added AIL coverage for lowercase and mixed-case direct subprogram calls with
  inline parenthesized suffixes.
- Empty `()` suffix remains accepted without warnings, and non-empty `(...)`
  remains ignored with deterministic call-arguments warnings across case
  variants.
- No behavior change in this slice; this captures existing case-insensitive
  lowering contract for baseline direct calls.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/ail_tests.cpp`

Known limitations:
- Inline subprogram call arguments are still not modeled in baseline AIL; they
  remain compatibility-only syntax with warnings when non-empty.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (docs: add dedicated G-code text flow page)
- Added `docs/src/development/gcode_text_flow.md` to explain the end-to-end path
  from input G-code text through parse, semantic rules, AIL, executor, packet,
  and message outputs.
- Linked the new page from the development section landing page and mdBook
  sidebar.
- No behavior change; this is documentation-only clarification.

SPEC sections / tests:
- SPEC: Section 9
- Validation: `mdbook build docs`, `./dev/check.sh`

Known limitations:
- The page summarizes current implementation boundaries and intentionally links
  to deeper design docs instead of duplicating all stage details.

How to reproduce locally (commands):
- `mdbook build docs`
- `./dev/check.sh`

## 2026-03-06 (docs: restructure development reference section)
- Moved developer architecture/design pages under `docs/src/development/design/`
  so they live under the development manual instead of as flat top-level
  mdBook entries.
- Replaced the single `development_reference.md` page with a structured
  development section (`development/index.md`, `development/workflow.md`,
  nested architecture/design index).
- Updated mdBook navigation and repo docs references to the new development
  subtree layout.

SPEC sections / tests:
- SPEC: Section 9
- Validation: `mdbook build docs`, `./dev/check.sh`

Known limitations:
- Existing root changelog history still references the old paths for historical
  PR entries.

How to reproduce locally (commands):
- `mdbook build docs`
- `./dev/check.sh`

## 2026-03-06 (docs: enable Mermaid rendering in mdBook)
- Configured `docs/book.toml` to run the `mdbook-mermaid` preprocessor and load
  Mermaid runtime assets in generated HTML output.
- Updated CI docs jobs to install `mdbook-mermaid` before `mdbook build docs`
  so published docs render diagrams correctly.
- Documented the local `mdbook-mermaid` prerequisite in `README.md`.

SPEC sections / tests:
- SPEC: Section 9
- Validation: `mdbook build docs`, `./dev/check.sh`

Known limitations:
- Mermaid rendering still depends on the `mdbook-mermaid` binary being
  available in local developer environments.

How to reproduce locally (commands):
- `cargo install mdbook mdbook-mermaid`
- `mdbook build docs`
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 31: case-parity PROC signature suffix coverage)
- Added AIL coverage showing lowercase and mixed-case `PROC` declarations keep
  the same inline signature-suffix behavior as uppercase baseline forms.
- Empty `()` suffix remains accepted without warnings; non-empty `(...)`
  suffix still lowers the label and emits the deterministic unsupported-signature
  warning.
- No behavior change in this slice; this captures existing case-insensitive
  lowering contract.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/ail_tests.cpp`

Known limitations:
- Baseline still ignores modeled `PROC` signature parameters and only preserves
  declaration labels.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 30: mixed-case quoted PROC equal-form syntax coverage)
- Added parser coverage for mixed-case quoted equal-form procedural input:
  `PrOc=\"DIR/SPF1000\"` currently surfaces a parse syntax error at the quote
  position.
- Added AIL coverage for the same mixed-case quoted equal-form input to lock
  the current fail-fast syntax diagnostic behavior.
- No behavior change in this slice; this captures current parser contract.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 29: lowercase quoted PROC equal-form syntax coverage)
- Added parser coverage for lowercase quoted equal-form procedural input:
  `proc=\"DIR/SPF1000\"` currently surfaces a parse syntax error at the quote
  position.
- Added AIL coverage for the same lowercase quoted equal-form input to lock the
  current fail-fast syntax diagnostic behavior.
- No behavior change in this slice; this captures current parser contract.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 28: quoted PROC equal-form syntax coverage)
- Added parser coverage for quoted equal-form `PROC` declaration input:
  `PROC=\"DIR/SPF1000\"` currently surfaces a parse syntax error at the quote
  position.
- Added AIL coverage for the same quoted equal-form input to lock current
  fail-fast syntax diagnostic behavior.
- No behavior change in this slice; this captures current parser contract.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 27: mixed-case PROC compatibility coverage)
- Added parser compatibility coverage for mixed-case `PrOc` declaration
  surface syntax and malformed mixed-case equal-form diagnostics.
- Added AIL coverage confirming mixed-case `PrOc` declarations lower to
  baseline label form and malformed equal-form shape errors match baseline.
- No behavior change in this slice; this is case-normalization contract
  hardening.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 26: lowercase PROC equal-form coverage)
- Added parser coverage for malformed lowercase `proc` equal-form declaration:
  `proc=main` emits deterministic malformed `PROC <name>` diagnostic.
- Added AIL coverage for malformed lowercase `proc=...` declaration shape,
  matching uppercase behavior and location contract.
- No behavior change in this slice;
this is case -compatibility contract hardening.

    SPEC sections /
    tests:
- SPEC: Section 3.9
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 25: lowercase PROC compatibility coverage)
- Added parser compatibility coverage for lowercase `proc` declaration surface
  syntax and malformed lowercase extra-word shape diagnostics.
- Added AIL lowering coverage confirming lowercase `proc` declaration lowers to
  baseline label instruction form.
- No behavior change in this slice;
this hardens keyword case -compatibility contract coverage.

    SPEC sections /
    tests:
- SPEC: Section 3.9
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 24: parser malformed PROC diagnostic location coverage)
- Added parser diagnostic-location assertions for malformed `PROC` declaration
  forms to lock deterministic first-offending-token reporting:
  - `PROC=MAIN` -> column 1
  - `PROC` -> column 1
  - `PROC G1` -> column 6
  - `PROC MAIN P2` -> column 11
- No runtime behavior change; this is parser-contract hardening coverage.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/parser_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 23: parse-stage malformed PROC shape parity)
- Extended parse-stage semantic checks for `PROC` declaration shape to match
  lowering behavior:
  - missing target (`PROC`)
  - invalid target word (`PROC G1`)
  - extra trailing word (`PROC MAIN P2`)
- Kept deterministic diagnostic text unchanged:
  `malformed PROC declaration; expected PROC <name>`.
- Added parser coverage for the malformed shape variants above.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 22: parse-stage PROC-headed malformed diagnostic)
- Added semantic parse-rule coverage for malformed PROC-headed forms with `=`
  (for example `PROC=MAIN`), emitting deterministic declaration-shape diagnostic
  at parse stage.
- Added parser test coverage for malformed `PROC=...` syntax.
- Kept lowering-stage guard in place for defense-in-depth consistency.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 21: malformed PROC-headed equal form diagnostic)
- Extended malformed `PROC` declaration detection to cover PROC-headed equal
  forms (for example `PROC=MAIN`) as deterministic declaration-shape errors.
- Added AIL test coverage for malformed `PROC=...` diagnostic behavior.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/ail_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 20: precise malformed PROC diagnostic location)
- Improved malformed `PROC` declaration diagnostic location precision in AIL
  lowering:
  - points to first offending token (invalid target word or extra trailing word)
    instead of always the `PROC` keyword
- Added AIL tests covering location columns for:
  - extra trailing token (`PROC MAIN P2`, `PROC "DIR/SPF1000" P2`)
  - invalid target word shape (`PROC G1`)

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/ail_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 19: malformed quoted PROC declaration coverage)
- Added explicit AIL coverage for malformed quoted PROC declaration shape:
  - `PROC "DIR/SPF1000" P2` deterministically errors with expected baseline
    declaration-shape diagnostic
- No runtime behavior change in this slice; this hardens malformed-shape
  contract coverage for quoted declaration forms.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/ail_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 18: parser coverage for quoted PROC declaration syntax)
- Added parser baseline coverage for quoted PROC declaration surface syntax:
  - `PROC "DIR/SPF1000"` parses as keyword + quoted target word
- No runtime behavior changes in this slice; this is syntax-contract test
  coverage hardening for Section 3.9.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/parser_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 17: quoted PROC declaration compatibility coverage)
- Added explicit baseline coverage for quoted PROC declaration targets:
  - AIL lowering emits declaration label for `PROC "DIR/SPF1000"`
  - executor resolves quoted declaration target with quoted call form
- Updated SPEC wording to clarify quoted declaration-target compatibility.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/ail_tests.cpp`, `test/ail_executor_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 16: deterministic error for malformed `PROC` declaration shapes)
- Tightened malformed `PROC` declaration detection:
  - any `PROC ...` line that does not match baseline `PROC <name>` declaration
    shape now emits deterministic lowering error
  - prevents silent acceptance of malformed forms like `PROC MAIN P2`
- Added AIL test coverage for malformed declaration shape diagnostic.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/ail_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 15: deterministic error for malformed `PROC` declaration)
- Added AIL-lowering guard for malformed declaration keyword form `PROC`
  without target name:
  - emits deterministic error diagnostic: `PROC declaration requires target name`
  - prevents accidental fallback interpretation as standalone subprogram call
- Added AIL test coverage for malformed `PROC` line behavior.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/ail_tests.cpp`

Known limitations:
- Baseline still does not parse structured procedural parameter lists into AST.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 14: whitespace-separated procedural suffix compatibility)
- Extended inline procedural suffix detection to support whitespace-separated
  forms after declaration/call targets:
  - no-warning no-arg compatibility: `PROC MAIN ()`, `MAIN ()`
  - deterministic warnings for non-empty suffixes also apply with whitespace:
    `PROC MAIN (R1)`, `MAIN (R1)`
- Added AIL tests for contiguous and whitespace-separated suffix handling.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/ail_tests.cpp`

Known limitations:
- Parenthesized procedural signatures/arguments are still not parsed into a
  structured AST model in this slice.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 13: accept empty procedural suffix `()` without warning)
- Refined inline procedural suffix handling in AIL lowering:
  - `PROC <name>()` is accepted as an explicit no-argument compatibility form
    without warning
  - `<subprogram_name>()` is accepted as an explicit no-argument call form
    without warning
  - non-empty `(...)` suffixes still emit deterministic warnings and are
    ignored in baseline model
- Added AIL tests for empty-suffix no-warning behavior and non-empty-suffix
  warning behavior.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/ail_tests.cpp`

Known limitations:
- Parenthesized procedural signatures/arguments are still not parsed into a
  structured AST model in this slice.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-06 (T-050 slice 12: explicit warnings for ignored inline procedural suffixes)
- Added AIL-lowering warnings for baseline procedural compatibility forms that
  currently parse `(...)` as inline comments:
  - `PROC <name>(...)` now emits declaration label plus warning
  - `<subprogram_name>(...)` now emits direct call plus warning
- Kept baseline behavior non-breaking: call/declaration targets still lower and
  execute as before; warning makes unsupported argument/signature modeling
  explicit.
- Added AIL tests covering warning emission for both declaration and call forms.

SPEC sections / tests:
- SPEC: Section 3.9
- Tests: `test/ail_tests.cpp`

Known limitations:
- Parenthesized procedural signatures/arguments are still not parsed into a
  structured AST model in this slice.
- The warning path currently triggers for inline contiguous `(...)` suffixes
  only.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-050 slice 11: baseline PROC declaration compatibility)
- Added lowering support for `PROC <name>` declaration lines:
  - lowers to `AilLabelInstruction` marker (non-motion declaration boundary)
  - enables runtime subprogram-call target resolution against PROC-declared
    names without introducing executable side effects
- Added parser/AIL/executor tests for PROC declaration surface syntax and
  call/return flow resolving against PROC declaration labels.

SPEC sections / tests:
- SPEC: Section 3.9, Section 6.1
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`,
  `test/ail_executor_tests.cpp`

Known limitations:
- Only baseline `PROC <name>` shape is covered in this slice; typed parameter
  signatures and call-argument parsing remain planned.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-050 slice 10: broaden direct call target shape baseline)
- Updated subprogram-call target detection to accept standalone unquoted
  identifier targets like `ALIAS` as direct calls (while still rejecting
  reserved boundary keywords such as `RET`/`RTLION`/`RTLIOF`).
- Added parser/AIL coverage for unquoted alphabetic target call form.
- Updated alias-map executor test to use non-`L` prefixed identifier names,
  verifying alias-map behavior with broader target-shape baseline.

SPEC sections / tests:
- SPEC: Section 3.9, Section 6.1
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`,
  `test/ail_executor_tests.cpp`

Known limitations:
- Target-shape acceptance is still lexical/heuristic in this slice; no full
  symbol-table name classification is implemented yet.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-050 slice 9: default subprogram alias-map policy)
- Extended `DefaultSubprogramPolicy` with `alias_map` support:
  - requested target can be deterministically remapped to another in-program
    label target (`requested -> resolved`)
- Added runtime diagnostic message when alias-map resolution is used.
- Added executor test coverage for default alias-map resolution flow.

SPEC sections / tests:
- SPEC: Section 6.1 (subprogram call runtime boundary)
- Tests: `test/ail_executor_tests.cpp`

Known limitations:
- Alias-map currently resolves only to labels in the current lowered program;
  no multi-file MPF/SPF program lookup in this slice.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-050 slice 8: quoted subprogram call compatibility form)
- Added parser support for quoted subprogram-call target items:
  - `"SUBPROGRAM_NAME"`
  - quoted text is normalized as target text without quote delimiters
- Added AIL lowering support so quoted call form emits `subprogram_call`
  instruction like existing direct call forms.
- Added parser/AIL/executor tests for quoted call parsing and runtime fallback
  behavior with `ExactThenBareName`.

SPEC sections / tests:
- SPEC: Section 3.9, Section 6.1
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`,
  `test/ail_executor_tests.cpp`

Known limitations:
- Quoted targets are currently supported for direct call item form only; full
  procedural `PROC` signature/argument parsing remains out of scope.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-050 slice 7: pluggable subprogram policy baseline)
- Added `SubprogramPolicy` interface and `DefaultSubprogramPolicy`:
  - target-resolution hook for subprogram calls
  - unresolved-target policy hook (`error|warning|ignore`)
- Updated `AilExecutor` to use policy-driven subprogram target resolution while
  keeping existing constructor options backward-compatible.
- Added executor test coverage for explicit policy override of call-target
  resolution.

SPEC sections / tests:
- SPEC: Section 6.1 (subprogram call runtime boundary)
- Tests: `test/ail_executor_tests.cpp`

Known limitations:
- Default policy still resolves against in-program labels only;
  multi - file MPF / SPF program -
          manager lookup remains out of scope
                  .

              How to reproduce locally(commands)
      : - `./ dev
              /
              check.sh`

              ##2026 -
          03 - 04(T - 050 slice 6 : subprogram search - policy baseline) -
          Added executor subprogram search policy control
      : - `SubprogramSearchPolicy::ExactOnly` (
            default) - `SubprogramSearchPolicy::ExactThenBareName` (fallback
                                                                        from
    `DIR / SPF1000` -> `SPF1000`) -
        Added runtime warning when bare -
        name fallback path is used to resolve a target.-
        Added executor unit test covering fallback resolution
      and call /
                  return flow.

                  SPEC sections
                  /
                  tests : -SPEC
      : Section 6.1(subprogram call runtime boundary)-Tests : `test
      /
      ail_executor_tests.cpp`

      Known limitations : -Search policy currently resolves only in
              - program labels; multi-file MPF/SPF
  filesystem lookup remains out of scope for this slice.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-050 slice 5: ISO M98 gating + unresolved-call policy)
- Added ISO-compatible subprogram call gating for `M98 P...`:
  - parser reports deterministic diagnostic unless ISO mode is enabled
    (`ParseOptions.enable_iso_m98_calls`)
  - AIL lowering emits `subprogram_call` for `M98 P<id>` when enabled
    (`LowerOptions.enable_iso_m98_calls`)
- Added executor unresolved-subprogram policy control:
  `error|warning|ignore` (default `error`).
- Added parser/AIL/executor tests for ISO gating and unresolved-call warning
  continuation behavior.

SPEC sections / tests:
- SPEC: Section 3.9, Section 6.1
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`,
  `test/ail_executor_tests.cpp`

Known limitations:
- ISO `M98` target is currently modeled as in-program call target ID only; no
  external program-file lookup integration in this slice.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (Dev tooling: fast local clang-tidy mode)
- Updated `dev/check.sh` to support `--fast` mode for local development.
- In `--fast` mode, `clang-tidy` runs only on changed/untracked C++ files under
  `src/` and `test/` (instead of the full repository set).
- Added parallel `clang-tidy` execution (configurable via
  `CLANG_TIDY_JOBS`, defaulting to CPU count) to reduce local wait time.

SPEC sections / tests:
- SPEC: no behavior contract change (tooling-only update)
- Tests: manual script validation (`./dev/check.sh --fast`)

Known limitations:
- `--fast` mode intentionally trades coverage for speed and should not replace
  full checks before merge/CI.

How to reproduce locally (commands):
- `./dev/check.sh --fast`

## 2026-03-04 (T-050 slice 4: runtime repeat_count semantics for subprogram_call)
- Implemented runtime handling for `subprogram_call.repeat_count`:
  - call frame tracks remaining repeats
  - `RET`/`M17` re-enters target until repeat count is exhausted, then returns
    to caller
- Added runtime policy for non-positive repeat counts:
  - `repeat_count <= 0` is ignored with warning and execution continues
- Added executor tests for repeat-loop behavior and zero-repeat warning flow.

SPEC sections / tests:
- SPEC: Section 3.9, Section 6.1
- Tests: `test/ail_executor_tests.cpp`

Known limitations:
- Subprogram target resolution remains label-based within current lowered
  program; multi-file MPF/SPF search-path resolution is still pending.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-050 slice 3: executor call-stack baseline for call/return)
- Added executor call-stack runtime behavior for subprogram boundaries:
  - `subprogram_call` resolves to label target, pushes return PC, and jumps
  - `return_boundary` (`RET`/`M17`) pops and resumes caller
- Added runtime fault behavior:
  - unresolved subprogram call target
  - return boundary with empty call stack
- Added executor state visibility for `call_stack_depth` and tests for
  call/return flow and fault cases.

SPEC sections / tests:
- SPEC: Section 3.9, Section 6.1
- Tests: `test/ail_executor_tests.cpp`

Known limitations:
- Subprogram target resolution is currently label-based within current lowered
  program only (no multi-file MPF/SPF search-path resolution yet).

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-050 slice 2: explicit subprogram call instruction baseline)
- Added explicit AIL `subprogram_call` instruction emission for direct Siemens
  call forms:
  - `<subprogram_name>`
  - `<subprogram_name> P<count>`
  - `P=<count> <subprogram_name>`
- Wired `subprogram_call` through AIL JSON/debug output, executor stepping, and
  packet lowering (no standalone motion packet/warning).
- Added parser/AIL/executor/packet tests for call syntax and call instruction
  baseline behavior.

SPEC sections / tests:
- SPEC: Section 3.9, Section 6.1
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`,
  `test/ail_executor_tests.cpp`, `test/packet_tests.cpp`

Known limitations:
- Subprogram target resolution and call-stack execution/return semantics are
  not implemented in this slice.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-050 slice 1: explicit return-boundary instruction for RET/M17)
- Added explicit AIL `return_boundary` instruction emission for `RET` and
  `M17` (instead of representing `M17` as generic `m_function`).
- Wired `return_boundary` through AIL JSON/debug output, executor stepping, and
  packet lowering (no standalone motion packet/warning).
- Added unit tests covering lowering/JSON shape, executor progression, and
  packet behavior for return-boundary instructions.

SPEC sections / tests:
- SPEC: Section 3.9, Section 6.1
- Tests: `test/ail_tests.cpp`, `test/ail_executor_tests.cpp`,
  `test/packet_tests.cpp`

Known limitations:
- Call-stack return semantics (matching call/return frames, unresolved return
  diagnostics/policies) are not implemented in this slice.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-054 follow-up: remove magic marker policy behavior)
- Removed deterministic magic marker handling from `DefaultToolPolicy`
  (`999991`/`999992` and marker parsing paths).
- Kept default policy behavior focused on:
  - direct resolution
  - optional substitution map
  - unresolved fallback for empty selection values
- Reworked tool-policy executor tests to use explicit stub policies for
  substitution/fallback/ambiguous outcomes, improving clarity and reducing
  test-only behavior in production policy code.

SPEC sections / tests:
- SPEC: no contract change (clarity/implementation cleanup)
- Tests: `test/ail_executor_tests.cpp`

Known limitations:
- Default policy remains a baseline resolver; machine-specific ambiguity
  detection/resolution still requires specialized policy implementations.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-054 slice: tool policy resolver/substitution integration)
- Added tool policy interface + default implementation:
  - `ToolPolicy` / `DefaultToolPolicy`
  - resolution outcomes: `resolved|unresolved|ambiguous`
  - options for substitution map, ambiguous/unresolved handling, and fallback
    selection
- Integrated tool policy into `AilExecutor` tool activation path for both
  direct (`T...`) and deferred (`T...` + `M6`) execution.
- Added executor integration tests for:
  - substitution during deferred `M6` activation
  - unresolved selection with fallback selection
  - ambiguous selection fault behavior by policy
- Updated SPEC Sections 3.13 and 6.1 for current policy/runtime boundary.

SPEC sections / tests:
- SPEC: Section 3.13, Section 6.1
- Tests: `test/ail_executor_tests.cpp`

Known limitations:
- Default policy includes deterministic placeholder unresolved/ambiguous marker
  handling; production controller resolver backends remain machine-specific.
- No hardware adapter/PLC integration in this slice.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-053 slice: executor tool pending/active semantics)
- Added runtime tool state to `AilExecutor`:
  - `active_tool_selection`
  - `pending_tool_selection`
- Implemented `tool_select` / `tool_change` execution baseline:
  - deferred mode: `T...` updates pending selection, `M6` activates it
  - direct mode: `T...` activates immediately
  - repeated deferred `T...` before `M6`: last selection wins
- Added configurable policy for `M6` with no pending selection:
  `error|warning|ignore` (default `error`).
- Added executor tests for deferred/direct behavior and no-pending-`M6` policy.
- Updated SPEC Sections 3.13 and 6.1 for current runtime boundary.

SPEC sections / tests:
- SPEC: Section 3.13, Section 6.1
- Tests: `test/ail_executor_tests.cpp`

Known limitations:
- No tool resolver/substitution policy integration yet (follow-up `T-054`).
- No hardware adapter/actuation backend in this slice; executor models runtime
  state transitions and diagnostics only.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (Docs alignment: executable-action instruction contract)
- Clarified cross-document contract that machine-carried actions must be
  represented as explicit executable instructions in IR (AIL).
- Updated PRD/SPEC/architecture docs to distinguish execution path from
  packetization:
  - motion instructions may emit packets
  - non-motion control instructions execute via runtime control path
- Updated tool-change design and backlog `T-053` acceptance to require explicit
  execution-path wiring for `tool_select`/`tool_change`.

SPEC sections / tests:
- SPEC: Section 2.2, Section 3.13, Section 6.1
- Tests: no code-path change in this slice (documentation/planning update only)

Known limitations:
- Runtime hardware actuation path for tool instructions is still pending
  follow-up implementation in `T-053`/`T-054`.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-052 slice: AIL tool_select/tool_change instruction split)
- Added explicit AIL instruction types for tool semantics:
  - `tool_select` (selector index/value + timing metadata)
  - `tool_change` (`M6` trigger + timing metadata)
- Updated lowering to emit tool instructions from `T...` selectors and `M6`,
  with timing determined by `LowerOptions.tool_change_mode` (default deferred).
- Updated AIL JSON/debug output and packet behavior to treat tool instructions
  as non-motion control state (no standalone packets/warnings).
- Added tests for AIL, CLI JSON/debug schema, and packet behavior.
- Updated SPEC Section 3.13 to include AIL tool instruction contract.
- Marked `T-052` as done in backlog as local/unmerged.

SPEC sections / tests:
- SPEC: Section 3.13
- Tests: `test/ail_tests.cpp`, `test/cli_tests.cpp`, `test/packet_tests.cpp`

Known limitations:
- Executor pending-tool state and direct/deferred execution semantics are not
  implemented in this slice (follow-up `T-053`).

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-051 slice: tool selector normalization + mode-aware validation)
- Added parser option `tool_management` and semantic validation for normalized
  Siemens `T...` selector forms.
- Added baseline selector validation rules:
  - `tool_management=false`: `T<number>`, `T=<number>`, `T<n>=<number>`
  - `tool_management=true`: `T=<location|name>`, `T<n>=<location|name>`
- Added parser tests for accepted/rejected forms in both modes.
- Updated SPEC with baseline tool selector syntax contract (Section 3.13).
- Marked `T-051` as done in backlog as local/unmerged.

SPEC sections / tests:
- SPEC: Section 3.13
- Tests: `test/parser_tests.cpp`

Known limitations:
- This slice validates selector syntax only; no `M6`/pending-tool runtime
  behavior is implemented here.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-038 design: tool-change architecture)
- Added a dedicated architecture page for Siemens tool-change semantics:
  - direct `T` change vs deferred `T`+`M6` execution model
  - with/without tool-management selector forms (number/location/name)
  - runtime pending-selection state and policy hooks for resolver/substitution
- Defined parser/lowering/executor/policy boundaries and output schema
  expectations for `tool_select`/`tool_change`.
- Added staged implementation slices and a generated follow-up backlog set
  (`T-051`..`T-054`) with required tests/doc updates per slice.
- Linked the design page into docs navigation and root architecture.
- Marked `T-038` as done in backlog as local/unmerged.

SPEC sections / tests:
- SPEC target: tool command syntax/runtime sections (future implementation)
- This PR is architecture/docs only; no parser/runtime behavior change.

Known limitations:
- No new parser/lowering/executor implementation in this slice.
- Tool database/life management and PLC integration remain out of scope.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-042 design: dimensions and units architecture)
- Added a dedicated architecture page for dimensions/units semantics:
  - Group 14 modal distance model (`G90/G91`)
  - Group 13 unit-scope model (`G70/G71/G700/G710`)
  - value-level overrides (`AC/IC`), rotary targeting (`DC/ACP/ACN`), and
    turning diameter/radius policy hooks (`DIAM*`, `DAC/DIC/RAC/RIC`)
- Defined precedence, unit-scope boundaries, and integration points with feed,
  plane/compensation, and work-offset states.
- Added implementation slices and test matrix for follow-up behavior PRs.
- Linked the design page into docs navigation and root architecture.
- Marked `T-042` as done in backlog as local/unmerged.

SPEC sections / tests:
- SPEC target: dimensions/units/modal behavior sections (future implementation)
- This PR is architecture/docs only; no parser/runtime behavior change.

Known limitations:
- No new parser/lowering/executor implementation in this slice.
- Full machine-data/kinematic internals remain out of scope.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-043 design: work-offset architecture)
- Added a dedicated architecture page for Siemens work-offset semantics:
  - Group 8 modal selection model (`G500`, `G54..G57`, `G505..G599`)
  - non-modal suppression context model (`G53`, `G153`, `SUPA`)
  - profile-driven offset-range/variant configuration hooks
- Defined precedence/scope rules for modal selection vs block suppression and
  outlined coordinate-pipeline integration boundaries.
- Added implementation slices and test matrix for follow-up behavior PRs.
- Linked the design page into docs navigation and root architecture.
- Marked `T-043` as done in backlog as local/unmerged.

SPEC sections / tests:
- SPEC target: work-offset syntax/runtime sections (future implementation)
- This PR is architecture/docs only; no parser/runtime behavior change.

Known limitations:
- No new parser/lowering/executor implementation in this slice.
- Frame-chain math and machine-specific transform internals remain out of scope.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-044 design: exact-stop and continuous-path architecture)
- Added a dedicated architecture page for Groups 10/11/12:
  - Group 10 modal family (`G60`, `G64..G645`)
  - Group 11 non-modal exact-stop (`G9`) block-scope precedence
  - Group 12 exact-stop criteria (`G601/G602/G603`) applicability model
  - `G641` coupling with `ADIS`/`ADISPOS`
- Defined precedence/applicability rules and runtime output expectations.
- Added implementation slices and test matrix for follow-up PRs.
- Linked the design page into docs navigation and root architecture.
- Marked `T-044` as done in backlog as local/unmerged.

SPEC sections / tests:
- SPEC target: transition mode syntax/runtime sections (follow-up implementation)
- This PR is architecture/docs only; no runtime behavior change.

Known limitations:
- No new modal-engine or executor behavior implemented in this slice.
- Servo/lookahead internals remain out of scope.

## 2026-03-04 (T-046 design: Siemens M-code model and execution boundaries)
- Added a dedicated architecture page for Siemens M functions:
  - syntax/validation model for `M<value>` and `M<ext>=<value>`
  - runtime classification model (program-flow/spindle/tool/gearbox/custom)
  - post-motion timing model for `M0/M1/M2/M17/M30`
  - machine profile/policy hooks and output schema expectations
- Added implementation slice plan and test matrix for follow-up PRs.
- Linked the new design page into docs navigation and root architecture
  references.
- Marked `T-046` as done in backlog as local/unmerged.

SPEC sections / tests:
- SPEC target: M-function syntax/runtime sections (future implementation slices)
- This PR is architecture/docs only; no runtime behavior change.

Known limitations:
- No new parser/executor implementation in this slice.
- PLC or machine-I/O integration remains out of scope.

## 2026-03-04 (T-045 design: rapid traverse architecture)
- Added a dedicated architecture page for rapid traverse semantics:
  - state model for `RTLION`/`RTLIOF`
  - declared vs effective rapid behavior for `G0`
  - forced-linear override precedence model
  - machine profile/policy hooks for runtime resolution
- Added implementation slices and test matrix for follow-up behavior PRs.
- Linked the new design page into docs navigation and root architecture.
- Marked `T-045` as done in backlog as local/unmerged.

SPEC sections / tests:
- SPEC target: rapid-traverse syntax/runtime sections (follow-up implementation)
- This PR is architecture/docs only; no parser/runtime behavior change.

Known limitations:
- No new runtime override implementation in this slice.
- Dynamics/planner internals remain out of scope.

## 2026-03-04 (T-047 design: incremental parse session API architecture)
- Added a dedicated design doc for incremental `ParseSession` API:
  - line-edit contract (`replace/insert/delete`)
  - resume selectors (`from_line`, `from_first_error`)
- deterministic prefix/suffix merge semantics for diagnostics and outputs
  - fail-fast and first-error-line recovery workflow
- Linked the new design page into docs navigation and architecture references.
- Marked `T-047` as done in backlog as local/unmerged.

SPEC sections / tests:
- SPEC target: Section 6 (incremental/resume contract)
- This PR is architecture/docs only; implementation tests are listed as follow-up
  slices in the new design document.

Known limitations:
- No parser-internal incremental parse-tree reuse optimization yet.
- No runtime/API code changes in this slice.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-04 (T-040 slice 3: carry effective working plane on arc outputs)
- Added `plane_effective` on AIL arc instructions and packet arc payloads.
- `plane_effective` is derived from active/same-block `G17/G18/G19` state in
  lowering and propagated to JSON/debug outputs.
- Added tests for AIL and packet plane metadata on arcs.
- Updated SPEC/program-reference docs for arc `plane_effective` output.

SPEC sections / tests:
- SPEC: Section 3.4
- Tests: `test/ail_tests.cpp`, `test/packet_tests.cpp`, `./dev/check.sh`

Known limitations:
- Arc geometry execution is still baseline; `plane_effective` is metadata for
  downstream runtime/planner consumers.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"AilTest|PacketTest\"`
- `./dev/check.sh`

## 2026-03-04 (T-040 slice 2: enforce G2/G3 center words by working plane)
- Added lowering-stage validation that `G2/G3` center words match the
  effective working plane (`G17`: `I/J`, `G18`: `I/K`, `G19`: `J/K`).
- Validation uses modal plane state with same-block plane overrides supported
  (e.g., `G18 G2 ... I... K...`).
- Added message-level tests for valid/invalid combinations and updated JSON
  roundtrip and golden fixture inputs for plane-compatible arc-center words.
- Updated SPEC/program-reference docs to reflect the new validation behavior.

SPEC sections / tests:
- SPEC: Section 3.12
- Tests: `test/messages_tests.cpp`, `test/messages_json_tests.cpp`,
  `testdata/messages/lowering_g2g3_failfast.*`, `./dev/check.sh`

Known limitations:
- Arc geometry remapping by plane is still limited to center-word validation;
  full plane-dependent arc computation behavior is not implemented yet.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"MessagesTest|MessagesJsonTest\"`
- `./dev/check.sh`

## 2026-03-04 (T-040 slice 1: Group 6 working-plane AIL/executor baseline)
- Added AIL instruction emission for `G17/G18/G19` as `working_plane` with
  normalized plane values (`xy|zx|yz`).
- Added `AilExecutor` state tracking via `working_plane_current`.
- Updated AIL JSON/debug output and packet behavior so working-plane
  instructions are preserved in AIL and ignored as standalone packet events.
- Added tests for AIL emission, executor state tracking, and packet behavior.
- Updated SPEC/program-reference docs for current Group 6 baseline scope.

SPEC sections / tests:
- SPEC: Section 3.12
- Tests: `test/ail_tests.cpp`, `test/ail_executor_tests.cpp`,
  `test/packet_tests.cpp`, `./dev/check.sh`

Known limitations:
- No plane-dependent geometric remapping is implemented in v0.
- Packet output remains motion/dwell only; Group 6 is currently modal state
  metadata.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"AilTest|AilExecutorTest|PacketTest\"`
- `./dev/check.sh`

## 2026-03-04 (T-039 slice 1: Group 7 tool-radius-comp AIL/executor baseline)
- Added AIL instruction emission for `G40/G41/G42` as `tool_radius_comp` with
  normalized mode values (`off|left|right`).
- Added `AilExecutor` state tracking via `tool_radius_comp_current`.
- Updated AIL JSON/debug output and packet behavior so tool-radius-comp
  instructions are preserved in AIL and ignored as standalone packet events.
- Added tests for AIL emission, executor state tracking, and packet behavior.
- Updated SPEC/program-reference docs for current Group 7 baseline scope.

SPEC sections / tests:
- SPEC: Section 3.11
- Tests: `test/ail_tests.cpp`, `test/ail_executor_tests.cpp`,
  `test/packet_tests.cpp`, `./dev/check.sh`

Known limitations:
- No geometric cutter compensation path adjustment is implemented in v0.
- Packet output still carries only motion/dwell packets; Group 7 remains modal
  state metadata.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"AilTest|AilExecutorTest|PacketTest\"`
- `./dev/check.sh`

## 2026-03-03 (T-045 slice 5: packetization ignores standalone rapid_mode)
- Updated packetization behavior for `rapid_mode` instructions to avoid
  emitting skip-warning diagnostics for standalone `RTLION`/`RTLIOF`.
- `rapid_mode` still does not emit standalone packets; its effect remains
  represented through `rapid_mode_effective` on `G0` linear payloads.
- Updated packet tests and program-reference docs to reflect warning-free
  packetization behavior for rapid-mode instructions.

SPEC sections / tests:
- SPEC: no behavior-contract section change required (packet warning policy
  adjustment only)
- Tests: `test/packet_tests.cpp`, `./dev/check.sh`

Known limitations:
- Packet layer still does not emit dedicated packets for `rapid_mode`; only
  `G0` payload metadata carries effective rapid mode.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"PacketTest\"`
- `./dev/check.sh`

## 2026-03-03 (T-045 slice 4: executor tracks rapid-mode state)
- Added `ExecutorState.rapid_mode_current` and executor handling for
  `rapid_mode` instructions so `AilExecutor` now tracks current `RTLION`/`RTLIOF`
  mode during step execution.
- Added executor test coverage for rapid-mode state transitions and persistence
  across `G0` instructions.
- Updated SPEC/program-reference docs to reflect current runtime boundary:
  state tracking implemented, machine-actuation override semantics still
  pending.

SPEC sections / tests:
- SPEC: Section 3.3
- Tests: `test/ail_executor_tests.cpp`, `./dev/check.sh`

Known limitations:
- Runtime machine-actuation interpolation override behavior is not implemented;
  tracked rapid mode is currently execution state metadata.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"AilExecutorTest\"`
- `./dev/check.sh`

## 2026-03-03 (T-045 slice 3: propagate effective rapid mode onto G0 outputs)
- Added `rapid_mode_effective` metadata on AIL linear instructions and applied
  current `RTLION`/`RTLIOF` state to subsequent `G0` instructions during
  lowering.
- Propagated `rapid_mode_effective` to packet linear payload output.
- Updated AIL/packet JSON and debug formatting to expose effective rapid mode
  when present.
- Added tests for AIL rapid-mode propagation and packet payload propagation.
- Updated SPEC and program-reference docs for current behavior/limitations.

SPEC sections / tests:
- SPEC: Section 3.3
- Tests: `test/ail_tests.cpp`, `test/packet_tests.cpp`, `./dev/check.sh`

Known limitations:
- Runtime execution still does not apply interpolation override behavior
  directly; rapid mode is currently modeled as output metadata.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"AilTest|PacketTest\"`
- `./dev/check.sh`

## 2026-03-03 (T-045 slice 2: RTLION/RTLIOF AIL rapid-mode baseline)
- Added AIL rapid interpolation mode instruction emission for `RTLION` and
  `RTLIOF` (`rapid_mode` with `opcode` + `mode`), preserving these commands in
  lowering output instead of dropping them.
- Updated AIL JSON/debug output to serialize `rapid_mode` instructions.
- Updated packetization to skip `rapid_mode` as non-motion with warning
  diagnostics.
- Added tests for AIL rapid-mode emission and packet skip-warning behavior.
- Updated SPEC and program-reference docs for current rapid-mode baseline and
  remaining runtime/packet limitations.

SPEC sections / tests:
- SPEC: Section 3.3
- Tests: `test/ail_tests.cpp`, `test/packet_tests.cpp`, `./dev/check.sh`

Known limitations:
- `RTLION`/`RTLIOF` semantics are represented in AIL only in this slice;
  runtime interpolation override behavior is not implemented yet.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"AilTest|PacketTest\"`
- `./dev/check.sh`

## 2026-03-03 (T-045 slice 1: G0 rapid baseline in lower/AIL/packet pipeline)
- Added `G0` motion-family lowering (`G0Lowerer`) and message support
  (`G0Message`) so `G0` now emits in lower JSON/debug output alongside existing
  motion families.
- Updated AIL linear move instruction model to carry explicit `opcode`
  (`G0`/`G1`) and propagated this through AIL JSON/debug and packetization so
  downstream consumers can distinguish rapid vs feed linear intent via modal
  metadata.
- Added tests for G0 lowering, message JSON round-trip, AIL opcode emission,
  packet emission, and motion-family factory registration.
- Updated `SPEC.md`, `PROGRAM_REFERENCE.md`, and
  `docs/src/program_reference.md` to reflect `G0` support and current
  limitations.

SPEC sections / tests:
- SPEC: Section 1, Section 2.2, Section 3.3
- Tests: `test/lowering_family_g0_tests.cpp`, `test/messages_tests.cpp`,
  `test/messages_json_tests.cpp`, `test/ail_tests.cpp`,
  `test/packet_tests.cpp`, `test/lowering_family_factory_tests.cpp`,
  `./dev/check.sh`

Known limitations:
- Siemens `RTLION`/`RTLIOF` rapid interpolation mode semantics and override
  precedence are not implemented in this slice.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R "G0LowererTest|MessagesTest|MessagesJsonTest|AilTest|PacketTest|LoweringFamilyFactoryTest"`
- `./dev/check.sh`

## 2026-03-03 (CI speedup slice 4: benchmark gating for PR latency)
- Added `workflow_dispatch` trigger to CI so benchmark can be run manually.
- Gated `benchmark-smoke` to skip on `pull_request` events and run on
  `main` push/manual dispatch only.
- This removes benchmark runtime from PR critical path while preserving regular
  benchmark signal on mainline.

SPEC sections / tests:
- SPEC: not applicable (CI/process change only)
- Validation: `./dev/check.sh`

Known limitations:
- PRs no longer execute benchmark-smoke by default; benchmark coverage is on
  main pushes/manual runs.

How to reproduce locally (commands):
- `sed -n '1,220p' .github/workflows/ci.yml`

## 2026-03-03 (CI speedup slice 3: ccache for CI builds)
- Added `ccache` to CI dependencies for `build-test` and `benchmark-smoke`.
- Added cache persistence for `.ccache` across runs using `actions/cache`.
- Wired compiler launcher to ccache:
  - `build-test`: via `CMAKE_CXX_COMPILER_LAUNCHER=ccache` environment
  - `benchmark-smoke`: via cmake configure flag
- Added post-job ccache stats output to track hit/miss behavior.

SPEC sections / tests:
- SPEC: not applicable (CI/process change only)
- Validation: `./dev/check.sh`

Known limitations:
- First run after cache key change is cold and may not speed up.
- Cache efficiency depends on source churn and branch reuse patterns.

How to reproduce locally (commands):
- `ccache -s`
- `sed -n '1,280p' .github/workflows/ci.yml`

## 2026-03-03 (CI speedup slice 2: PR fast-check path)
- Added `dev/check_pr.sh` to run a faster PR gate:
  - clang-format check
  - configure/build
  - test suite
  - fuzz smoke test
- Updated CI `build-test` job to use:
  - `dev/check_pr.sh` for `pull_request`
  - `dev/check.sh` for `push` on `main`
- This keeps full heavy validation on mainline while reducing PR latency.

SPEC sections / tests:
- SPEC: not applicable (CI/process change only)
- Validation: `./dev/check.sh`

Known limitations:
- PR fast path skips clang-tidy and sanitizer stages; those remain in full
  `./dev/check.sh` on `main` pushes.

How to reproduce locally (commands):
- `./dev/check_pr.sh`
- `./dev/check.sh`

## 2026-03-03 (CI speedup slice 1: trigger/concurrency/cache)
- Updated CI triggers to avoid duplicate branch + PR runs:
  - `push` now runs only on `main`
  - `pull_request` continues for PR validation
- Added workflow-level concurrency cancellation so superseded runs on the same
  ref are auto-canceled.
- Added ANTLR tool caching (`.tools/antlr-4.10.1-complete.jar`) and conditional
  download in `build-test` and `benchmark-smoke`.

SPEC sections / tests:
- SPEC: not applicable (CI orchestration change only)
- Validation: YAML sanity via local inspection; runtime validation occurs in GitHub Actions.

Known limitations:
- Full `build-test` runtime remains dominated by `./dev/check.sh` (format/tidy,
  sanitizer build/tests).

How to reproduce locally (commands):
- `sed -n '1,260p' .github/workflows/ci.yml`

## 2026-03-03 (T-046 slice 3: M-function executor boundary policy)
- Added `AilExecutor` handling for `m_function` instructions with configurable
  unknown-M policy (`error`, `warning`, `ignore`).
- Added known predefined Siemens M-value classification in executor
  (`M0/M1/M2/M3/M4/M5/M6/M17/M19/M30/M40..M45/M70`) so these advance without
  machine-actuation side effects in v0.
- Added executor tests for known M behavior and unknown-M policy outcomes.
- Updated SPEC and program-reference docs for current executor boundary.

SPEC sections / tests:
- SPEC: Section 3.10, Section 6.1
- Tests: `test/ail_executor_tests.cpp`, `./dev/check.sh`

Known limitations:
- No machine actuation mapping yet for M functions (spindle/coolant/tool I/O is
  not executed in this slice).

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"AilExecutorTest\"`
- `./dev/check.sh`

## 2026-03-03 (T-046 slice 2: M-function AIL emission baseline)
- Added `AilMCodeInstruction` and lowering logic to emit `m_function`
  instructions from parsed M words (`M<value>` and `M<ext>=<value>`).
- Added AIL JSON/debug output support for `m_function` instructions with
  `value` and optional `address_extension`.
- Added AIL tests covering emission order, line-number propagation, and JSON
  shape for multiple M functions in one program.
- Updated SPEC and program-reference docs to reflect parse/validation + AIL
  support status for M functions.

SPEC sections / tests:
- SPEC: Section 3.10, Section 6
- Tests: `test/ail_tests.cpp`, `./dev/check.sh`

Known limitations:
- Runtime machine action mapping/execution semantics for M functions are not
  implemented in this slice.
- Packet stage continues to skip `m_function` (non-motion) instructions.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"AilTest\"`
- `./dev/check.sh`

## 2026-03-03 (T-046 slice 1: M-function parse/validation baseline)
- Added semantic validation baseline for M functions:
  - accepts `M<value>` and `M<ext>=<value>` parse forms
  - enforces integer range `0..2147483647`
  - rejects extended form for `M0/M1/M2/M17/M30`
- Added parser tests for valid baseline forms and invalid range/prohibited
  extended-address cases.
- Updated SPEC and program-reference docs to mark M-function support as
  parse+diagnostics partial (runtime actuation pending).

SPEC sections / tests:
- SPEC: Section 3.10
- Tests: `test/parser_tests.cpp`, `./dev/check.sh`

Known limitations:
- Runtime machine action mapping for M functions is not implemented in this
  slice.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"ParserSyntaxBaselineTest\"`
- `./dev/check.sh`

## 2026-03-03 (docs: publish split design pages in mdBook)
- Added mdBook design section with split pages:
  - `docs/src/design/index.md`
  - `docs/src/design/pipeline.md`
  - `docs/src/design/modal_strategy.md`
  - `docs/src/design/implementation_plan.md`
- Updated `docs/src/SUMMARY.md`, `docs/src/index.md`, and
  `docs/src/development_reference.md` to link to design pages and avoid a
  monolithic single-page design dump.
- Synced mdBook structure with new SPEC policy requiring design docs under
  `docs/src/` and doc splitting for long pages.

SPEC sections / tests:
- SPEC: Section 9 (Documentation Policy)
- Tests: `./dev/check.sh`

Known limitations:
- Design pages are synchronized summaries of root docs; deep implementation
  details still primarily live in `ARCHITECTURE.md` and `IMPLEMENTATION_PLAN.md`.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-03-03 (T-037 slice 2: optional // comment mode)
- Added parse option `ParseOptions.enable_double_slash_comments`.
- Added `// ...` comment token support with config-gated semantic behavior:
  rejected by default, accepted when the option is enabled.
- Added parser tests for disabled/enabled `//` behavior and preserved comment
  text.
- Added SPEC policy reminder that feature/function PRs must update program
  reference docs in the same PR (`PROGRAM_REFERENCE.md` and/or
  `docs/src/program_reference.md`).
- Synced both program-reference documents to current implemented behavior for
  comments, line numbers, skip levels, control-flow/runtime boundaries, and
  parse/lower options.
- Added SPEC documentation-policy rules requiring design docs to be synced into
  `docs/src/` and long documentation pages to be split into smaller mdBook
  pages linked via `docs/src/SUMMARY.md`.

SPEC sections / tests:
- SPEC: Section 2.1
- Tests: `test/parser_tests.cpp`, `test/semantic_rules_tests.cpp`,
  `./dev/check.sh`

Known limitations:
- `//` compatibility mode is parser-only in this slice and not yet exposed as a
  CLI flag.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"ParserSyntaxBaselineTest|SemanticRulesTest\"`
- `./dev/check.sh`

## 2026-03-03 (T-037 slice 1: Siemens block-comment parsing)
- Added parser grammar support for Siemens block comments
  `(* ... *)`, including multi-line comment text.
- Added parser tests covering multi-line block-comment capture and unclosed
  block-comment diagnostics.
- Updated SPEC input-comment section to mark `(* ... *)` as implemented in the
  current v0 subset.

SPEC sections / tests:
- SPEC: Section 2.1
- Tests: `test/parser_tests.cpp`, `./dev/check.sh`

Known limitations:
- Optional Siemens `//` comment mode remains planned and config-gated.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"ParserSyntaxBaselineTest\"`
- `./dev/check.sh`

## 2026-03-03 (T-049 slice 3: assignment-shape baseline checks)
- Added parser semantic rule enforcing `=` for multi-letter or
  numeric-extension address values (for example rejects `AP90`, accepts
  `AP=90` and `X1=10`).
- Added parser tests for invalid missing-`=` multi-letter value form and valid
  numeric-extension `X1=10` parsing.
- Updated SPEC assignment-shape baseline notes to reflect implemented behavior.

SPEC sections / tests:
- SPEC: Section 3.6
- Tests: `test/parser_tests.cpp`, `./dev/check.sh`

Known limitations:
- Full Siemens expression-value assignment shapes are still planned.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"ParserSyntaxBaselineTest|ParserExpressionTest\"`
- `./dev/check.sh`

## 2026-03-03 (T-049 slice 2: skip-level execution policy in lowering)
- Added skip-level execution policy support in `LowerOptions` via
  `active_skip_levels`.
- Message/AIL/streaming lowering now skips lines with block-delete markers when
  the corresponding skip level is active (default `/` maps to level `0`).
- Added coverage tests for message lowering, AIL lowering, and streaming mode
  skip behavior.

SPEC sections / tests:
- SPEC: Section 6.1
- Tests: `test/messages_tests.cpp`, `test/ail_tests.cpp`,
  `test/streaming_tests.cpp`, `./dev/check.sh`

Known limitations:
- Skip-level behavior is currently configured per lowering call and is not yet
  wired to machine-profile or session-level UI state.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"MessagesTest|AilTest|StreamingTest\"`
- `./dev/check.sh`

## 2026-03-03 (T-049 slice 1: skip-level parsing + block-length diagnostics)
- Added parser support for Siemens-style block-delete skip levels (`/0.. /9`)
  and captured skip level metadata in AST lines.
- Added semantic validation for skip-level range with actionable diagnostics for
  invalid values (for example `/10`).
- Added parser diagnostics when a block exceeds 512 characters (including
  end-of-block LF), plus parser tests for new behavior.

SPEC sections / tests:
- SPEC: Section 3.1, Section 5
- Tests: `test/parser_tests.cpp`, `./dev/check.sh`

Known limitations:
- Block length uses fixed Siemens baseline (512) and is not yet profile-configurable.
- Skip-level metadata is parse-stage only in this slice (runtime skip-policy
  execution wiring comes in follow-up slices).

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"ParserSyntaxBaselineTest|ParserControlFlowTest\"`
- `./dev/check.sh`

## 2026-03-03 (T-041 foundation scaffolding: machine profile + modal registry)
- Added `MachineProfile` baseline scaffolding (`src/machine_profile.*`) with
  controller/tool-mode/error-policy fields and a Siemens 840D baseline preset.
- Added `ModalRegistry` baseline scaffolding (`src/modal_registry.*`) with
  Siemens group-keyed state, block lifecycle (`beginBlock`), and conflict checks.
- Added unit tests for profile defaults/range handling and modal persistence,
  block-scope behavior, and conflict detection.

SPEC sections / tests:
- SPEC: no user-visible behavior change in this slice (internal scaffolding only)
- Tests: `test/machine_profile_tests.cpp`, `test/modal_registry_tests.cpp`,
  `./dev/check.sh`

Known limitations:
- Registry is not yet wired into parser/lowering/runtime pipelines.
- Group conflict policies currently implement error-default only; warning/ignore
  handling will be added in later slices.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"MachineProfileTest|ModalRegistryTest\"`
- `./dev/check.sh`

## 2026-03-03 (docs architecture review alignment)
- Added architecture documentation baseline files:
  `ARCHITECTURE.md` (pipeline/modules/policy model with Mermaid diagrams) and
  `IMPLEMENTATION_PLAN.md` (phased delivery/dependency order/test matrix).
- Expanded PRD/backlog coverage for Siemens-focused requirement planning
  (`T-037`..`T-047`) and added integrated acceptance fixture
  `testdata/integration/simple_integrated_case.ngc`.
- Applied review fixes for documentation consistency:
  included Group 6 in modal strategy, documented `ParseSession` incremental
  resume design, aligned backlog selection policy with implementation dependency
  order, and clarified `SPEC.md` current vs planned comment syntax modes.

SPEC sections / tests:
- SPEC: Section 2.1 (comment mode clarification), Section 6 (resume contract)
- Tests: not applicable (docs + fixture planning update only)

Known limitations:
- This slice is documentation/planning only; Siemens feature families remain to
  be implemented task-by-task.
- `SPEC.md` still reflects current implemented parser behavior for many syntax
  families and intentionally marks Siemens extensions as planned.

How to reproduce locally (commands):
- `git diff -- PRD.md SPEC.md BACKLOG.md ARCHITECTURE.md IMPLEMENTATION_PLAN.md CHANGELOG_AGENT.md`
- `nl -ba ARCHITECTURE.md | sed -n '16,180p'`
- `nl -ba SPEC.md | sed -n '12,40p'`
## 2026-03-03 (docs: system variables + user-defined variables requirement)
- Added PRD requirement section for variable handling that explicitly covers both
  user-defined variables (`R...`) and Siemens-style system variables (`$...`),
  including parse/runtime responsibility boundaries.
- Updated SPEC to clarify current v0 variable scope and planned Siemens
  selector-form extensions (for example `$P_UIFR[1,X,TR]`, `$A_IN[1]`).
- Added backlog task `T-048` for implementation planning/execution of variable
  reference modeling and runtime resolver semantics.
- Added Siemens Chapter-2 baseline requirement capture for:
  - NC naming compatibility
  - block formatting and block-length diagnostics
  - assignment `=` rules and numeric-extension disambiguation
  - skip-level syntax `/0.. /9` and parse/runtime execution boundary
  via new backlog task `T-049`.
- Added Siemens subprogram requirement capture:
  - `.MPF/.SPF` organization
  - baseline `M17` return and optional `RET`
  - direct/quoted name calls, repeat calls (`P` / `P=` forms)
  - resolver policy for same-folder vs qualified-path lookup
  via new backlog task `T-050`.
- Updated reference docs to mark subprogram features as `Planned` and added
  development-architecture notes for resolver/call-stack/policy boundaries.

SPEC sections / tests:
- SPEC: Section 3.1, Section 3.6, Section 3.8, Section 3.9, Section 5, Section 6.1
- Tests: not applicable (docs-only requirement/reference update)

Known limitations:
- No parser/runtime behavior change in this slice; this update is requirement
  capture and planning only.

How to reproduce locally (commands):
- `git diff -- PRD.md SPEC.md BACKLOG.md CHANGELOG_AGENT.md`

## 2026-03-02 (T-036 Siemens-style line-number handling)
- Added semantic diagnostics for `N`-address usage:
  invalid `N` token forms and misplaced `N` words not at block start.
- Added duplicate `N`-address warning in parse diagnostics to flag ambiguous
  line-number jumps.
- Added executor warning for ambiguous runtime `N`/numeric target resolution
  (nearest-match policy by search direction), plus coverage tests.

SPEC sections / tests:
- SPEC: Section 3.1, Section 3.7, Section 6.1
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`,
  `test/ail_executor_tests.cpp`, `./dev/check.sh`

Known limitations:
- Duplicate `N`-address handling is warning-only (allowed by parser).
- `CALL`/subprogram line-number semantics remain out of scope.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"ParserControlFlowTest.*LineNumber|AilTest.KeepsLineNumberInInstructionSourceAndGotoTargetKind|AilExecutorTest.*LineNumber|AilExecutorTest.WarnsWhenDuplicateLineNumbersAreAmbiguous\"`
- `./dev/check.sh`

## 2026-03-02 (T-035 structured IF/ELSE/ENDIF lowering to executable AIL)
- Added structured control-flow lowering for `IF ... ELSE ... ENDIF` blocks in
  AIL lowering, using internal labels/gotos and `branch_if` so runtime executes
  only one branch body.
- Added AIL diagnostics for malformed structured blocks during lowering
  (`ELSE`/`ENDIF` mismatch and missing `ENDIF`).
- Added tests for lowered instruction shape and executor branch-path behavior
  proving single-branch execution.

SPEC sections / tests:
- SPEC: Section 6.1 (structured IF lowering/runtime semantics)
- Tests: `test/ail_tests.cpp`, `test/ail_executor_tests.cpp`, `./dev/check.sh`

Known limitations:
- Structured `WHILE/FOR/REPEAT/LOOP` lowering is still not implemented.
- Internal generated labels are implementation details and not stable API.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"AilTest.StructuredIfElseLowersToBranchGotoAndLabels|AilExecutorTest.StructuredIfElseExecutesSingleBranchAtRuntime\"`
- `./dev/check.sh`

## 2026-03-02 (T-034 control-flow AND condition parsing)
- Extended control-flow condition grammar to accept logical `AND` chains and
  parenthesized condition terms such as
  `IF (R1 == 1) AND (R2 > 10) ...`.
- Added condition metadata fields (`raw`, `has_logical_and`, `and_terms`) to
  AST/AIL JSON output so runtime condition resolvers can evaluate complex
  expressions externally.
- Added parser + AIL tests that lock `AND` condition parsing/lowering behavior.

SPEC sections / tests:
- SPEC: Section 3.7 (condition composition support)
- Tests: `test/parser_tests.cpp`, `test/ail_tests.cpp`, `./dev/check.sh`

Known limitations:
- Logical composition currently supports `AND` only in parser grammar.
- Parenthesized condition groups are preserved as raw terms and are not yet
  lowered into a fully typed boolean expression tree.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"ParserControlFlowTest|AilTest.BranchIfJsonIncludesLogicalAndConditionMetadata\"`
- `./dev/check.sh`

## 2026-03-02 (T-032 runtime control-flow executor core)
- Added control-flow AIL instruction lowering for parsed `label`, `goto`, and
  `if ... goto` statements.
- Added runtime `AilExecutor` with async condition-callback contract
  (`true|false|pending|error`) and executor states
  (`ready|blocked_on_condition|completed|fault`).
- Added event/retry unblock behavior for pending branch conditions and
  `GOTOF/GOTOB/GOTO/GOTOC` target-resolution behavior in executor runtime.
- Added unit tests for executor completion, unresolved-target fault behavior,
  non-faulting `GOTOC`, and pending-to-event resume flow.

SPEC sections / tests:
- SPEC: Section 6.1 (new control-flow executor runtime notes)
- Tests: `test/ail_tests.cpp`, `test/ail_executor_tests.cpp`, `./dev/check.sh`

Known limitations:
- Structured block control statements (`IF/ELSE/ENDIF`, `WHILE`, `FOR`,
  `REPEAT`, `LOOP`) are parse-supported but not yet lowered/executed as runtime
  control-flow instructions.
- Condition values are callback-resolved; builtin expression evaluator/runtime
  variable store is not included in this slice.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"AilTest.LowersControlFlowInstructions|AilExecutorTest\"`
- `./dev/check.sh`

## 2026-03-02 (T-033 expand parse-only control-flow syntax coverage)
- Extended grammar/parser/AST for Siemens-style control-flow syntax:
  `GOTOF/GOTOB/GOTO/GOTOC`, richer goto targets, structured
  `IF/ELSE/ENDIF`, `WHILE/ENDWHILE`, `FOR/ENDFOR`, `REPEAT/UNTIL`,
  and `LOOP/ENDLOOP`.
- Added condition-operator parsing for control-flow conditions:
  `==`, `>`, `<`, `>=`, `<=`, `<>`.
- Expanded parser tests for jump opcode/target kinds and structured
  control-flow statement parsing.

SPEC sections / tests:
- SPEC: Section 3.7 (expanded control-flow syntax), Section 4 wording update
- Tests: `test/parser_tests.cpp`, `./dev/check.sh`

Known limitations:
- Control flow remains parse-only in v0 (no runtime branch execution, no label
  resolution, no packet-level branch semantics).
- `FOR` syntax currently assumes implicit step `+1` (no explicit `STEP` token).

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R ParserControlFlowTest`
- `./dev/check.sh`

## 2026-03-02 (T-031 parse-only control flow syntax)
- Added parser support for control-flow statements: label declarations
  (`<label>:`), `GOTO <label>`, and
  `IF <expr> THEN GOTO <label> [ELSE GOTO <label>]`.
- Extended AST with explicit control-flow nodes and surfaced them in parse
  debug/JSON output.
- Expanded parser tests to cover `GOTO`, label parsing with underscores, and
  `IF/THEN/ELSE` jump syntax.

SPEC sections / tests:
- SPEC: Section 3.7 (new), Section 4 (non-goals wording update)
- Tests: `test/parser_tests.cpp`, `./dev/check.sh`

Known limitations:
- Control flow is parse-only in v0; labels are not resolved and no execution
  or branch semantics are applied in lowering.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R ParserControlFlowTest`
- `./dev/check.sh`

## 2026-02-28 (chore ignore generated output directory)
- Updated `.gitignore` to ignore the full `output/` directory (not just
  `output/bench`) so generated artifacts do not pollute working tree status.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: not applicable (ignore rule update only)

Known limitations:
- Existing untracked files remain on local disk until manually removed.

How to reproduce locally (commands):
- `git status --short`
- `cat .gitignore`

## 2026-02-28 (chore remove irrelevant reference/hello)
- Removed `reference/hello/` sample assets (ANTLR hello example and related
  prompt scaffolding) because they are not used by this project.
- Kept `reference/gcode/` intact.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: not applicable (reference cleanup only)

Known limitations:
- None; this change only removes unused reference material.

How to reproduce locally (commands):
- `test -d reference/hello && echo exists || echo removed`
- `git status --short`

## 2026-02-28 (T-027 AIL golden fixtures)
- Added dedicated AIL lowering golden test target
  (`test/ail_lowering_tests.cpp`) and fixture corpus under `testdata/ail/`.
- Added representative AIL goldens for supported motion subset and fail-fast
  error behavior.
- Updated CMake to include `ail_lowering_tests` in the test matrix.

SPEC sections / tests:
- SPEC: Section 7 (AIL fixture expectations)
- Tests: `test/ail_lowering_tests.cpp`, `testdata/ail/*.golden.json`,
  `./dev/check.sh`

Known limitations:
- Current AIL fixture corpus is minimal (core motion + fail-fast); additional
  edge-case fixtures can be added incrementally.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R AilLoweringGoldenTest`
- `./dev/check.sh`

## 2026-02-28 (backlog sync after T-030 merge)
- Updated `BACKLOG.md` to move `T-030` out of Ready/In Progress and into Done
  with `PR #40`.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: not applicable (backlog state sync only)

Known limitations:
- `T-027` remains queued until explicit AIL golden-fixture scope is closed.

How to reproduce locally (commands):
- `sed -n '1,220p' BACKLOG.md`

## 2026-02-28 (T-030 CLI stage-output goldens)
- Added stage-level CLI golden fixtures under `testdata/cli/` covering
  `parse|ail|packet|lower` in both `debug` and `json` formats.
- Added `CliFormatTest.StageOutputsMatchGoldens` to lock deterministic CLI
  outputs against those fixtures.
- Synced backlog state to mark T-028 done (PR #39) and set T-030 in progress.

SPEC sections / tests:
- SPEC: Section 7 (CLI stage golden fixture policy)
- Tests: `test/cli_tests.cpp`, `testdata/cli/*`, `./dev/check.sh`

Known limitations:
- Golden fixtures currently use one representative input file
  (`testdata/messages/g4_dwell.ngc`); broader CLI fixture coverage can be
  expanded incrementally.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R \"CliFormatTest.StageOutputsMatchGoldens|CliFormatTest\"`
- `./dev/check.sh`

## 2026-02-28 (T-028 AIL to MotionPacket stage)
- Added packet model + conversion stage (`parseLowerAndPacketize`) from AIL
  into deterministic motion packets with 1-based `packet_id`.
- Added packet JSON serialization (`packetToJsonString`) and packet golden
  fixtures under `testdata/packets/`.
- Added CLI packet stage mode: `gcode_parse --mode packet --format json|debug`.

SPEC sections / tests:
- SPEC: Section 2.2 (CLI output modes), Section 6 (packet stage),
  Section 7 (packet goldens + CLI coverage)
- Tests: `test/packet_tests.cpp`, `test/cli_tests.cpp`,
  `testdata/packets/*.golden.json`, `./dev/check.sh`

Known limitations:
- v0 packetization only emits motion/dwell packets; non-motion AIL
  instructions (for example assignment) are skipped with warning diagnostics.
- Packet JSON currently supports serialization only (no `fromJson` parser yet).

How to reproduce locally (commands):
- `./build/gcode_parse --mode packet --format json testdata/packets/core_motion.ngc`
- `./build/gcode_parse --mode packet --format debug testdata/packets/assignment_skip.ngc`
- `ctest --test-dir build --output-on-failure -R \"PacketTest|CliFormatTest\"`
- `./dev/check.sh`

## 2026-02-28 (T-029 expression AST for assignments)
- Extended grammar/parser to support assignment statements with expressions,
  including system-variable operands (example: `R1 = $P_ACT_X + 2*R2`).
- Added typed expression AST nodes (literal, variable/system-variable, unary,
  binary) and assignment AST storage in `Line`.
- Added AIL assignment instruction lowering using typed expression AST instead
  of raw expression strings.
- Extended AIL JSON/debug output to serialize assignment expression trees.
- Added parser, AIL, and CLI tests for assignment expression parsing/lowering.

SPEC sections / tests:
- SPEC: Section 3.6 (assignment syntax), Section 5 (syntax diagnostics),
  Section 6 (AIL assignment coverage)
- Tests: `parser_tests`, `ail_tests`, `cli_tests`, `./dev/check.sh`

Known limitations:
- Parenthesized subexpressions are not supported yet in v0.
- Expression support currently targets assignment statements only; runtime
  evaluation/execution is still out of scope.

How to reproduce locally (commands):
- `./build/gcode_parse --mode ail --format json <file.ngc>`
- `./build/gcode_parse --mode ail --format debug <file.ngc>`
- `ctest --test-dir build --output-on-failure -R \"ParserExpressionTest|AilTest|CliFormatTest\"`
- `./dev/check.sh`

## 2026-02-28 (T-026 AIL intermediate representation v0)
- Added AIL IR model (`src/ail.h`, `src/ail.cpp`) with instruction variants:
  linear motion, arc motion, dwell, and placeholders for assignment/sync.
- Added AIL JSON export (`src/ail_json.h`, `src/ail_json.cpp`) with stable
  top-level schema (`schema_version`, `instructions`, `diagnostics`,
  `rejected_lines`).
- Extended CLI with `--mode ail --format json|debug` stage visibility.
- Added AIL unit tests and CLI tests for AIL mode.
- Updated specs/docs to describe parse -> AIL -> lower visibility.

SPEC sections / tests:
- SPEC: Section 2.2 (CLI modes), Section 6 (pipeline/lowering notes)
- Tests: `test/ail_tests.cpp`, `test/cli_tests.cpp`, `./dev/check.sh`

Known limitations:
- AIL v0 currently covers the existing motion subset derived from message
  lowering and includes assignment/sync as placeholder instruction types only.

How to reproduce locally (commands):
- `./build/gcode_parse --mode ail --format json testdata/messages/g4_dwell.ngc`
- `./build/gcode_parse --mode ail --format debug testdata/messages/g4_dwell.ngc`
- `ctest --test-dir build --output-on-failure -R \"AilTest|CliFormatTest\"`
- `./dev/check.sh`

## 2026-02-28 (pipeline roadmap backlog: AIL -> packets)
- Updated `BACKLOG.md` after `T-025` merge (`PR #36`).
- Added new queued tasks `T-026` to `T-030` to evolve architecture toward:
  `Source G-code -> parse -> AIL -> MotionPacket`.
- Marked `T-026` as current in-progress planning/implementation slice.

SPEC sections / tests:
- SPEC: planning targets for Section 2.2, Section 3, Section 5, Section 6, Section 7
- Tests: no code-path change (planning/backlog update only)

Known limitations:
- This update defines the work plan only; AIL/packet implementation is not part
  of this change.

How to reproduce locally (commands):
- `sed -n '1,320p' BACKLOG.md`

## 2026-02-28 (T-025 CLI lower mode)
- Extended `gcode_parse` with `--mode parse|lower` (default `parse`).
- Added lower mode outputs:
  - `--mode lower --format json` emits lowered `MessageResult` JSON.
  - `--mode lower --format debug` emits stable human-readable summary lines.
- Added CLI tests for lower mode JSON/debug outputs and unsupported-mode error.
- Updated `SPEC.md`, README, and development docs for the new CLI behavior.

SPEC sections / tests:
- SPEC: Section 2.2 (CLI modes/formats), Section 7 (CLI test expectation)
- Tests: `test/cli_tests.cpp`, `./dev/check.sh`

Known limitations:
- Lower debug output is line-oriented summary text, not a formal schema.
- No streaming CLI mode yet.

How to reproduce locally (commands):
- `./build/gcode_parse --mode lower --format json testdata/messages/g4_dwell.ngc`
- `./build/gcode_parse --mode lower --format debug testdata/messages/g4_dwell.ngc`
- `ctest --test-dir build --output-on-failure -R CliFormatTest`
- `./dev/check.sh`

## 2026-02-28 (backlog: add T-025 CLI lower mode task)
- Added backlog task `T-025` to implement CLI lower mode
  (`--mode lower --format json|debug`) for file-to-message output.
- Synced backlog state to mark `T-024` done (`PR #35`).

SPEC sections / tests:
- SPEC: planning target references Section 2.2, Section 6, and Section 7
- Tests: no code-path change (backlog planning/sync only)

Known limitations:
- This is planning only; CLI lower mode is not implemented in this change.

How to reproduce locally (commands):
- `sed -n '1,280p' BACKLOG.md`

## 2026-02-28 (T-024 modal-group metadata on messages)
- Added modal metadata to all lowered message types (`G1/G2/G3/G4`):
  `modal.group`, `modal.code`, and `modal.updates_state`.
- Implemented Siemens-preferred baseline mapping for supported functions:
  `G1/G2/G3 -> GGroup1 (updates_state=true)`, `G4 -> GGroup2 (updates_state=false)`.
- Extended JSON serialization/deserialization and message JSON goldens to
  include modal metadata.
- Added/updated unit tests for lowering families, message extraction,
  streaming callbacks, and JSON modal fields.

SPEC sections / tests:
- SPEC: Section 6 (Message Lowering modal metadata + JSON schema notes),
  Section 9 (reference docs alignment)
- Tests: `messages_tests`, `streaming_tests`, `messages_json_tests`,
  `lowering_family_g{1,2,3,4}_tests`, message JSON goldens, `./dev/check.sh`

Known limitations:
- v0.1 provides modal metadata for supported functions only; it does not
  implement full multi-group modal conflict validation or machine-specific
  modal behavior beyond this baseline mapping.

How to reproduce locally (commands):
- `./dev/check.sh`
- `ctest --test-dir build --output-on-failure -R \"MessagesJsonTest|MessagesTest|StreamingTest|G[1-4]LowererTest\"`

## 2026-02-27 (backlog sync after T-023 merge)
- Updated `BACKLOG.md` to mark `T-023` done (`PR #33`) and clear stale Ready
  Queue / In Progress entries.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: not applicable (backlog state sync only)

Known limitations:
- Remaining backlog work is currently in Icebox only.

How to reproduce locally (commands):
- `sed -n '1,260p' BACKLOG.md`

## 2026-02-27 (T-023 README rewrite)
- Rewrote `README.md` into a comprehensive, implementation-aligned guide:
  feature status, prerequisites, build/test/benchmark/docs workflow, and API
  entry points.
- Added concrete usage snippets for batch parse/lower, streaming callbacks, and
  JSON conversion helpers.
- Added contribution/documentation alignment notes linking `SPEC.md`, `PRD.md`,
  and mdBook policy.

SPEC sections / tests:
- SPEC: Section 6 (Message Lowering summary references), Section 9 (Documentation Policy)
- Tests: `./dev/check.sh`

Known limitations:
- README examples are intentionally minimal and are not compiled as doctests.

How to reproduce locally (commands):
- `./dev/check.sh`
- `sed -n '1,260p' README.md`

## 2026-02-27 (backlog: add README rewrite task T-023)
- Added backlog task `T-023` in Ready Queue to rewrite `README.md` as a
  comprehensive, implementation-aligned guide.

SPEC sections / tests:
- SPEC: planning target references Section 9 and Section 6
- Tests: no code-path change (backlog planning update only)

Known limitations:
- This adds planning/task definition only; README rewrite implementation is not
  included in this change.

How to reproduce locally (commands):
- `sed -n '1,260p' BACKLOG.md`

## 2026-02-27 (backlog sync after T-022 merge)
- Updated `BACKLOG.md` to move `T-022` out of Ready/In Progress and into Done
  with `PR #31`.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: not applicable (backlog state sync only)

Known limitations:
- Remaining backlog items (`coverage threshold policy`, `multi-file include`)
  are still pending future scoped PRs.

How to reproduce locally (commands):
- `sed -n '1,260p' BACKLOG.md`

## 2026-02-27 (T-022 mdBook docs + Pages pipeline)
- Added mdBook documentation source tree under `docs/` with:
  - `docs/src/development_reference.md`
  - `docs/src/program_reference.md`
- Added CI docs build job and GitHub Pages deploy job from `main` in
  `.github/workflows/ci.yml`.
- Updated `SPEC.md` documentation policy to require mdBook updates on code/API
  behavior changes.
- Added generated docs output ignore rule (`docs/book`) in `.gitignore`.

SPEC sections / tests:
- SPEC: Section 9 (Documentation Policy), Section 7 (CI/testing expectations reference)
- Tests: CI docs build step (`mdbook build docs`)

Known limitations:
- This change establishes mdBook structure and publishing pipeline; it does not
  fully migrate all legacy markdown docs into mdBook in one pass.

How to reproduce locally (commands):
- `mdbook build docs`
- `mdbook serve docs --open`
- `./dev/check.sh`

## 2026-02-27 (CI dependency: gcovr)
- Added `gcovr` to CI apt dependency installation for both `build-test` and
  `benchmark-smoke` jobs in `.github/workflows/ci.yml`.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: not applicable (CI environment dependency update only)

Known limitations:
- Coverage thresholds/report publishing are not enabled yet; this change only
  prepares CI tooling availability.

How to reproduce locally (commands):
- `sudo apt-get update && sudo apt-get install -y gcovr`
- `gcovr --version`

## 2026-02-27 (backlog status sync after merged T-021)
- Updated `BACKLOG.md` to mark `T-021` as done (`PR #28`) and clear stale
  `Ready Queue` / `In Progress` references.
- Removed the stale "Performance benchmarking harness" item from Icebox because
  it was completed in `T-020`.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: not applicable (documentation/backlog status update only)

Known limitations:
- Icebox still contains larger roadmap items (`coverage policy`, `multi-file include`)
  that need separate scoped tasks before implementation.

How to reproduce locally (commands):
- `sed -n '1,260p' BACKLOG.md`
- `sed -n '1,120p' CHANGELOG_AGENT.md`

## 2026-02-22
- Added `./dev/check.sh` to run format checks, build, tests, and sanitizer tests.
- Added CI workflow to run the same checks.
- Updated contributor rules to require `./dev/check.sh` evidence and test/spec coverage.

SPEC sections / tests:
- SPEC: N/A (process change only)
- Tests: `ctest --test-dir build --output-on-failure` via `./dev/check.sh`

Known limitations:
- Requires `clang-format`, `clang-tidy`, `antlr4`, and ANTLR4 C++ runtime installed.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-02-22 (follow-up)
- Ran `./dev/check.sh`, fixed `clang-format` violations in parser/test sources.
- Updated sanitizer test invocation to set `ASAN_OPTIONS=detect_leaks=0` for environments where LeakSanitizer is blocked by ptrace.
- Updated `CMakeLists.txt` to read `ANTLR4_RUNTIME_INCLUDE_DIR` and `ANTLR4_RUNTIME_LIB` from environment variables, matching CI configuration.
- Reordered `./dev/check.sh` so `cmake --build` runs before `clang-tidy`, ensuring generated ANTLR headers exist for tidy analysis.
- Added `dev/antlr4.sh` and CI ANTLR 4.10.1 tool pinning, and updated `CMakeLists.txt` to honor `ANTLR4_EXECUTABLE` from environment for consistent tool/runtime compatibility.

SPEC sections / tests:
- SPEC: Section 6 (Testing Expectations)
- Tests: `./dev/check.sh` including normal and sanitizer `ctest`

Known limitations:
- Leak detection is disabled during sanitizer `ctest` in `./dev/check.sh`; dedicated leak checks should run in a compatible environment.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-02-22 (ooda governance docs)
- Added `ROADMAP.md` with milestone-based direction and exit criteria.
- Added `BACKLOG.md` with prioritized tasks, acceptance criteria, and task template.
- Added `OODA.md` with loop rules, merge policy, and definition of done.
- Updated `AGENTS.md` to require agent use of `BACKLOG.md`, `ROADMAP.md`, and `OODA.md` before implementation starts.

SPEC sections / tests:
- SPEC: Section 6 (Testing Expectations) alignment and process enforcement
- Tests: no code-path change; governance/documentation-only update

Known limitations:
- Backlog items are initial and will need regular reprioritization as CI/runtime data evolves.

How to reproduce locally (commands):
- `git checkout feature/ooda-repo-loop`
- `sed -n '1,220p' ROADMAP.md`
- `sed -n '1,260p' BACKLOG.md`
- `sed -n '1,260p' OODA.md`

## 2026-02-23 (message lowering v0 slice)
- Added standalone message-lowering module that converts parsed AST into
  polymorphic queue messages, starting with `G1Message`.
- Added `G1Message` extraction fields: optional filename/line source,
  optional `N` line number, target pose `xyzabc`, and feed `F`.
- Added tests validating valid-line extraction and skip-error-line behavior
  (`G1 X10`, `G1 G2 X10`, `G1 X20`).

SPEC sections / tests:
- SPEC: Section 6 (Message Lowering), Section 7 (Testing Expectations)
- Tests: `messages_tests`, `parser_tests`, plus `./dev/check.sh`

Known limitations:
- Lowering currently emits only `G1Message`; `G2/G3` message types are backlog items.
- Resume-from-line is behaviorally supported by full reparse + skip-error policy; no dedicated incremental session API yet.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure`
- `./dev/check.sh`

## 2026-02-23 (message lowering explicit rejection policy)
- Added explicit `rejected_lines` reporting in message lowering results so
  library users can detect invalid non-emitted lines and reasons directly.
- Enforced fail-fast lowering behavior: stop at first error line.
- Added/updated tests to assert explicit rejection reporting and fail-fast behavior.
- Added lowercase-equivalence coverage (`x`/`X`, `g1`/`G1`) in message-lowering tests.
- Updated SPEC testing requirement to standardize on GoogleTest for new unit tests.

SPEC sections / tests:
- SPEC: Section 6 (Message Lowering), Section 7 (Testing Expectations)
- Tests: `messages_tests`, `./dev/check.sh`

Known limitations:
- Existing tests currently include a custom harness; migration to GoogleTest is pending.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-02-23 (agent policy and PR template)
- Updated `AGENTS.md` with explicit single-command quality gate (`./dev/check.sh`)
  and tooling config pointers.
- Added `.github/pull_request_template.md` with OODA sections:
  Observe/Orient/Decide/Act, evidence, and follow-ups.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: process/docs-only change; validated with `./dev/check.sh`

Known limitations:
- PR template quality depends on maintainers filling each section with concrete evidence.

How to reproduce locally (commands):
- `sed -n '1,260p' AGENTS.md`
- `sed -n '1,260p' .github/pull_request_template.md`

## 2026-02-23 (migrate tests to GoogleTest)
- Refactored `test/parser_tests.cpp` from custom harness to GoogleTest cases.
- Refactored `test/messages_tests.cpp` from custom harness to GoogleTest cases.
- Updated CMake test registration to `gtest_discover_tests` and CI dependencies
  to include `libgtest-dev`.

SPEC sections / tests:
- SPEC: Section 7 (Testing Expectations, GoogleTest requirement)
- Tests: parser and message test suites now run through GoogleTest via `ctest`

Known limitations:
- Existing golden snapshots remain plain text; only harness changed.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-02-23 (message JSON serialization with nlohmann/json)
- Added `src/messages_json.h` + `src/messages_json.cpp` with message result
  JSON serialization/deserialization APIs.
- Added `nlohmann/json` dependency integration in CMake and CI.
- Added JSON tests: round-trip, invalid-JSON diagnostic, and asset-based
  golden message output validation under `testdata/messages/`.

SPEC sections / tests:
- SPEC: Section 2.2, Section 6 (JSON schema notes), Section 7 (message JSON goldens)
- Tests: `messages_json_tests`, `messages_tests`, `parser_tests`, and `./dev/check.sh`

Known limitations:
- `fromJson` currently reconstructs supported message types (`G1`) only.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-02-23 (backlog task for G2/G3 lowering)
- Added backlog task `T-012` for implementing typed `G2`/`G3` message lowering.
- Updated backlog tracking to mark `T-011` done and clear in-progress slot.

SPEC sections / tests:
- SPEC: planned coverage target in Section 6 (message lowering) and Section 7 (tests)
- Tests: no code-path change in this update

Known limitations:
- This is planning/governance only; no `G2/G3` lowering implementation included yet.

How to reproduce locally (commands):
- `sed -n '1,260p' BACKLOG.md`
- `sed -n '1,320p' CHANGELOG_AGENT.md`

## 2026-02-23 (backlog task for message golden matrix)
- Added backlog task `T-013` to expand `testdata/messages` beyond a single fail-fast fixture.
- Defined acceptance criteria for a table-driven golden suite covering valid, mixed, and case-equivalence scenarios.

SPEC sections / tests:
- SPEC: planned coverage target in Section 7 (testing)
- Tests: no code-path change in this update

Known limitations:
- Planning update only; fixture expansion is not implemented in this change.

How to reproduce locally (commands):
- `sed -n '1,260p' BACKLOG.md`
- `sed -n '1,360p' CHANGELOG_AGENT.md`

## 2026-02-23 (T-001 parser fuzz smoke gate)
- Added `fuzz_smoke_tests` GoogleTest target that runs parser/lowering smoke
  checks over deterministic corpus files and fixed-seed generated inputs.
- Added deterministic corpus assets under `testdata/fuzz/corpus/`.
- Integrated fuzz smoke execution into `./dev/check.sh` (normal + sanitizer runs).

SPEC sections / tests:
- SPEC: Section 7 (property/fuzz testing + runtime budget)
- Tests: `FuzzSmokeTest.CorpusAndGeneratedInputsDoNotCrashOrHang`,
  `./dev/check.sh`, CI `build-test`

Known limitations:
- This is a smoke gate, not coverage-guided fuzzing; it improves crash/hang
  detection but does not prove complete memory-boundedness.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R FuzzSmokeTest`
- `./dev/check.sh`

## 2026-02-23 (T-002 regression-test protocol)
- Added `test/regression_tests.cpp` with a concrete regression case:
  `Regression_MixedCartesianAndPolar_G1ReportsError`.
- Added regression test target wiring in CMake (`regression_tests`).
- Documented regression naming convention in `README.md`.
- Updated `OODA.md` Definition of Done to require bug/issue -> regression test linkage.

SPEC sections / tests:
- SPEC: Section 7 (regression test expectation)
- Tests: `RegressionTest.Regression_MixedCartesianAndPolar_G1ReportsError`,
  `./dev/check.sh`

Known limitations:
- Regression suite currently has one seed case; additional fixed bugs should
  extend this suite over time.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R RegressionTest`
- `./dev/check.sh`

## 2026-02-23 (T-003 actionable diagnostics)
- Improved parser diagnostics text to be actionable:
  - syntax diagnostics now use `syntax error:` prefix plus context hints
  - semantic diagnostics include correction guidance for motion conflicts and
    G1 cartesian/polar mixing
- Added parser unit coverage for actionable syntax + semantic diagnostics.
- Refreshed affected golden outputs and message JSON goldens for updated text.

SPEC sections / tests:
- SPEC: Section 5 (Diagnostics), Section 7 (testing)
- Tests: `ParserDiagnosticsTest.ActionableSyntaxAndSemanticMessages`,
  parser golden tests, message JSON golden test, `./dev/check.sh`

Known limitations:
- Syntax hinting currently pattern-matches common ANTLR/lexer messages and may
  remain generic for uncommon parser errors.

How to reproduce locally (commands):
- `./dev/check.sh`

## 2026-02-23 (T-004 CLI JSON/debug output mode)
- Added CLI option `--format json|debug` to `gcode_parse` (default: `debug`).
- Added parse-result JSON formatter with stable top-level keys:
  `schema_version`, `program`, and `diagnostics`.
- Added CLI tests validating debug-mode golden compatibility and JSON output schema.
- Updated README examples and SPEC CLI output notes.

SPEC sections / tests:
- SPEC: Section 2.2 (CLI output modes), Section 7 (testing)
- Tests: `CliFormatTest.DebugFormatMatchesExistingGoldenOutput`,
  `CliFormatTest.JsonFormatOutputsStableSchema`, `./dev/check.sh`

Known limitations:
- JSON mode currently targets parse/diagnostic output only; message-lowering
  JSON remains in separate message serialization APIs.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R CliFormatTest`
- `./build/gcode_parse --format json testdata/g2g3_samples.ngc`
- `./dev/check.sh`

## 2026-02-23 (T-005 grammar nullable-tail warning cleanup)
- Updated `line_no_eol` grammar alternatives to disallow an empty match.
- Removed ANTLR warning about optional block matching empty string in `program`.
- Kept parser behavior stable for existing fixtures and tests.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: parser goldens, message tests, CLI tests, and `./dev/check.sh`

Known limitations:
- Grammar still accepts broad word/value forms by design in v0; this change
  only addresses nullable-tail warning cleanup.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `./dev/check.sh`

## 2026-02-23 (T-012 G2/G3 message lowering)
- Added typed `G2Message` and `G3Message` queue payloads with source info,
  target pose, arc params (`I/J/K/R`), and optional feed.
- Extended AST-to-message lowering to emit `G2`/`G3` messages and map `CR` to
  arc radius field `r`.
- Extended message JSON serialization/deserialization for `G2`/`G3` and `arc`.
- Added tests for G2/G3 extraction and G2/G3 message JSON round-trip/golden output.

SPEC sections / tests:
- SPEC: Section 2.2 (typed message support), Section 6 (message lowering schema),
  Section 7 (testing)
- Tests: `MessagesTest.G2G3Extraction`,
  `MessagesJsonTest.RoundTripWithG2G3PreservesResult`,
  `MessagesJsonTest.GoldenMessageOutput`, `./dev/check.sh`

Known limitations:
- Arc lowering currently captures `I/J/K/R` only; other arc forms (for example
  `AR`, `CIP`, `CT`) are parsed in AST but not lowered into dedicated fields.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R \"MessagesTest|MessagesJsonTest\"`
- `./dev/check.sh`

## 2026-02-23 (T-013 message golden matrix expansion)
- Expanded `testdata/messages` with additional representative fixtures:
  valid multi-line G1, lowercase equivalence, no-filename source case, and
  comments/blank-lines handling.
- Refactored message JSON golden test to table-driven fixture discovery over
  `testdata/messages/*.ngc` with `<name>.golden.json` pairing.
- Documented message fixture naming convention in SPEC Section 7.

SPEC sections / tests:
- SPEC: Section 7 (message fixture naming convention)
- Tests: `MessagesJsonTest.GoldenMessageOutput`, `./dev/check.sh`

Known limitations:
- Table-driven golden runner currently supports one special naming override:
  `nofilename_` fixtures run with `LowerOptions.filename` unset.

How to reproduce locally (commands):
- `ctest --test-dir build --output-on-failure -R MessagesJsonTest.GoldenMessageOutput`
- `./dev/check.sh`

## 2026-02-23 (T-008 resume-from-line session API)
- Added `ParseSession` API (`src/session.h/.cpp`) to support line edits and
  resume-style reparsing from a requested line index.
- Added `SessionTest` coverage for edit-and-resume flow after fixing an error
  line and deterministic message ordering after edits.
- Wired new `session_tests` target into CMake and test discovery.

SPEC sections / tests:
- SPEC: Section 6 (resume session API notes), Section 7 (testing)
- Tests: `SessionTest.EditAndResumeFromErrorLine`,
  `SessionTest.MessageOrderingIsDeterministicAfterEdit`, `./dev/check.sh`

Known limitations:
- Current implementation reparses full text internally while exposing
  resume-from-line API semantics; targeted incremental diff/apply remains a
  separate backlog item.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R SessionTest`
- `./dev/check.sh`

## 2026-02-23 (T-009 queue diff/apply API)
- Added line-keyed queue diff API in `src/message_diff.h/.cpp`:
  `diffMessagesByLine(before, after)` with `added`, `updated`, and `removed_lines`.
- Added queue apply helper `applyMessageDiff(current, diff)` that reconstructs
  deterministic line-ordered message list.
- Added `message_diff_tests` coverage for update/insert/delete edit scenarios.

SPEC sections / tests:
- SPEC: Section 6 (queue diff/apply API notes), Section 7 (testing)
- Tests: `MessageDiffTest.UpdateLineProducesUpdatedEntry`,
  `MessageDiffTest.InsertLineProducesAddedEntry`,
  `MessageDiffTest.DeleteLineProducesRemovedEntry`, `./dev/check.sh`

Known limitations:
- Diff/apply keys messages by source line only; future modal/state-aware
  semantics may require richer identity keys.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R MessageDiffTest`
- `./dev/check.sh`

## 2026-02-23 (T-010 diagnostics policy + severity mapping)
- Defined and implemented severity mapping at lowering stage:
  - `error` for fail-fast rejecting diagnostics
  - `warning` for non-fatal unsupported arc-word lowering limits
- Added arc-lowering warning diagnostics for unsupported words
  (`AR`, `AP`, `RP`, `CIP`, `CT`, `I1`, `J1`, `K1`) while still emitting
  supported `G2/G3` message fields.
- Added test coverage validating warning severity and partial emission behavior.

SPEC sections / tests:
- SPEC: Section 6 (severity mapping policy), Section 7 (testing)
- Tests: `MessagesTest.ArcUnsupportedWordsEmitWarningsButKeepMessage`,
  `./dev/check.sh`

Known limitations:
- Warning list is currently rule-based and explicit; future parser/lowering
  extensions may adjust which words are considered supported.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R ArcUnsupportedWordsEmitWarningsButKeepMessage`
- `./dev/check.sh`

## 2026-02-23 (backlog status finalize)
- Updated `BACKLOG.md` status only: moved `T-010` to Done and cleared In Progress.

SPEC sections / tests:
- SPEC: no behavior change
- Tests: not applicable (docs-only)

Known limitations:
- None (status bookkeeping update only).

How to reproduce locally (commands):
- `sed -n '1,220p' BACKLOG.md`

## 2026-02-24 (SPEC architecture rule for OO per-family modules)
- Added SPEC architecture requirement to avoid monolithic parser/lowering files.
- Added explicit requirement for per-family module/class structure (`G1`,
  `G2/G3`, and future families) in SPEC Section 8.
- Added backlog task `T-014` to track refactor work toward this architecture.

SPEC sections / tests:
- SPEC: Section 8 (Code Architecture Requirements)
- Tests: no behavior change (docs/backlog update only)

Known limitations:
- This change defines architecture policy only; full source refactor is tracked
  separately as `T-014`.

How to reproduce locally (commands):
- `sed -n '1,320p' SPEC.md`
- `sed -n '1,220p' BACKLOG.md`

## 2026-02-24 (T-014 OO per-family module refactor)
- Refactored parser semantic validation into rule classes in
  `src/semantic_rules.cpp` (motion exclusivity + G1 coordinate mode rules).
- Refactored message lowering into per-family lowerer classes/files:
  `G1Lowerer`, `G2Lowerer`, `G3Lowerer`, with factory wiring and common helpers.
- Kept public parse/lowering APIs stable while removing monolithic family logic
  from `src/gcode_parser.cpp` and `src/messages.cpp`.

SPEC sections / tests:
- SPEC: Section 8 (Code Architecture Requirements)
- Tests: full suite via `./dev/check.sh`

Known limitations:
- Refactor focuses on parser semantic rules + message lowering family modules;
  full AST builder extraction from `src/gcode_parser.cpp` can be a future step.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `./dev/check.sh`

## 2026-02-24 (T-015 per-class unit tests for OO family modules)
- Added dedicated GoogleTest targets for OO module files: `semantic_rules`,
  `lowering_family_factory`, `lowering_family_g1`, `lowering_family_g2`, and
  `lowering_family_g3`.
- Added focused unit tests per class/module file under `test/` validating
  motion code mapping, field lowering, semantic diagnostics, and warning
  behavior.
- Updated `BACKLOG.md` status bookkeeping: `T-014` moved to Done and `T-015`
  tracked as In Progress on this branch.

SPEC sections / tests:
- SPEC: Section 7 (Testing Strategy), Section 8 (Code Architecture Requirements)
- Tests: `SemanticRulesTest.*`, `MotionFamilyFactoryTest.*`, `G1LowererTest.*`,
  `G2LowererTest.*`, `G3LowererTest.*`, plus `./dev/check.sh`

Known limitations:
- Per-class coverage is focused on family lowerers and semantic rules introduced
  by the OO split; AST/parser internals remain covered primarily by existing
  parser/message/golden tests.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R "(SemanticRulesTest|MotionFamilyFactoryTest|G1LowererTest|G2LowererTest|G3LowererTest)"`
- `./dev/check.sh`

## 2026-02-24 (T-016 targeted regression edge-case tests)
- Added targeted regression coverage for duplicate same-motion words in one line
  (`G1 G1 X...`) to ensure no false exclusivity error and successful lowering.
- Added regression coverage for syntax-error location reporting on unsupported
  characters (line/column assertion).
- Added regression coverage confirming non-motion G-codes (e.g., `G4`) do not
  emit motion queue messages in v0.

SPEC sections / tests:
- SPEC: Section 7 (Testing Strategy)
- Tests: `RegressionTest.Regression_DuplicateSameMotionWord_NoExclusiveError`,
  `RegressionTest.Regression_UnsupportedChars_HasAccurateLocation`,
  `RegressionTest.Regression_NonMotionGCode_DoesNotEmitMessage`,
  `./dev/check.sh`

Known limitations:
- These tests lock current v0 behavior; future motion-family expansion may
  intentionally change non-motion lowering expectations.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R RegressionTest`
- `./dev/check.sh`

## 2026-02-24 (T-017 long-input stress smoke test)
- Added deterministic long-input stress smoke test in
  `test/fuzz_smoke_tests.cpp` that parses and lowers a large valid program.
- Added assertions for no diagnostics/rejections, expected message count, and
  execution-time envelope to catch crash/hang regressions.
- Updated backlog status: marked `T-016` done and tracked `T-017` in progress.

SPEC sections / tests:
- SPEC: Section 7 (Testing Strategy)
- Tests: `FuzzSmokeTest.LongProgramInputDoesNotCrashOrHang`,
  `./dev/check.sh`

Known limitations:
- Stress threshold is a smoke-level guard, not a formal benchmark budget;
  absolute timing may vary by machine.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R FuzzSmokeTest`
- `./dev/check.sh`

## 2026-02-24 (backlog task added: T-018 G4 dwell support)
- Added new backlog task `T-018` to implement `G4` dwell support with
  `G4 F...` (seconds) and `G4 S...` (spindle revolutions) syntax.
- Defined acceptance criteria for parser/lowering behavior, diagnostics,
  message JSON support, and test coverage expectations.
- Updated stale backlog status for `T-017` to `PR #23`.

SPEC sections / tests:
- SPEC impact planned by task: Sections 3, 5, 6, 7, 8
- Tests planned by task: parser/messages/messages_json/regression + message
  goldens for G4

Known limitations:
- This change is planning-only; no parser or lowering behavior changed yet.

How to reproduce locally (commands):
- `sed -n '1,240p' BACKLOG.md`
- `sed -n '1,520p' CHANGELOG_AGENT.md`

## 2026-02-24 (SPEC update for planned G4 dwell support)
- Updated `SPEC.md` to include `G4` dwell syntax with `G4 F...` (seconds) and
  `G4 S...` (master-spindle revolutions).
- Added diagnostics requirement that `G4` be programmed in a separate NC block
  with explicit correction-hint messaging.
- Extended message-lowering and JSON schema sections to define `G4Message`
  fields (`dwell_mode`, `dwell_value`, source metadata).

SPEC sections / tests:
- SPEC: Sections 1, 2, 3.5, 5, and 6
- Tests: planned under task `T-018` (no code/test behavior change in this docs-only commit)

Known limitations:
- This is a spec-definition update only; parser/lowering implementation for
  `G4Message` remains pending in `T-018`.

How to reproduce locally (commands):
- `sed -n '1,320p' SPEC.md`
- `sed -n '480,620p' CHANGELOG_AGENT.md`

## 2026-02-24 (program reference manual policy + initial manual)
- Added `SPEC.md` Section 9 defining a program-reference maintenance policy,
  including status labels (`Implemented`/`Partial`/`Planned`) and PR update
  requirements.
- Added `PROGRAM_REFERENCE.md` as the implementation snapshot manual for command
  syntax, lowering output, diagnostics, examples, and test references.
- Documented `G4` explicitly as `Planned` in the reference manual so product
  docs track current code behavior vs planned SPEC behavior.

SPEC sections / tests:
- SPEC: Section 9 (Program Reference Manual Policy)
- Tests: not applicable (docs-only change)

Known limitations:
- Manual is intentionally hand-maintained; no automatic doc generation from
  parser grammar/tests yet.

How to reproduce locally (commands):
- `sed -n '1,380p' SPEC.md`
- `sed -n '1,320p' PROGRAM_REFERENCE.md`
- `sed -n '520,700p' CHANGELOG_AGENT.md`

## 2026-02-24 (T-018 G4 dwell support)
- Implemented `G4` dwell support end-to-end with new `G4Message` type, OO
  lowerer module (`src/lowering_family_g4.h/.cpp`), and motion-family factory
  registration.
- Added semantic diagnostics for G4 block rules: separate-block enforcement,
  exactly-one dwell mode (`F` xor `S`), and numeric dwell value validation.
- Extended message JSON and diff/apply paths for `G4Message`, and added G4
  tests + golden fixtures (`testdata/messages/g4_*.ngc/.golden.json`).

SPEC sections / tests:
- SPEC: Section 3.5 (G4 syntax), Section 5 (diagnostics), Section 6 (message
  lowering/JSON), Section 7 (testing)
- Tests: `MessagesTest.G4ExtractionSecondsAndRevolutions`,
  `MessagesTest.G4MustBeSeparateBlockAndFailFast`,
  `MessagesJsonTest.RoundTripWithG4PreservesResult`,
  `SemanticRulesTest.ReportsG4NotSeparateBlock`,
  `SemanticRulesTest.ReportsG4RequiresExactlyOneModeWord`,
  `G4LowererTest.*`, `MessageDiffTest.G4LineUpdateIsTrackedAsUpdatedEntry`,
  `RegressionTest.Regression_G4RequiresSeparateBlock`,
  `MessagesJsonTest.GoldenMessageOutput` (new `g4_*` fixtures),
  `./dev/check.sh`

Known limitations:
- Dwell runtime conversion (e.g. spindle-revolution time conversion using
  spindle RPM/override state) is intentionally out of scope for parser/lowering
  v0 and not computed here.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R "(G4LowererTest|MessagesTest.G4|MessagesJsonTest.RoundTripWithG4|SemanticRulesTest.ReportsG4|RegressionTest.Regression_G4|MessageDiffTest.G4|MessagesJsonTest.GoldenMessageOutput)"`
- `./dev/check.sh`

## 2026-02-24 (add PRD for product/API requirements)
- Added `PRD.md` to define product goals, non-goals, command-family policy,
  quality gates, and release acceptance criteria for the parser library.
- Added explicit product API requirements, including recommended class facade,
  file-based parse/lower APIs, and output contracts (`ParseResult`/
  `MessageResult`).
- Linked SPEC documentation policy to `PRD.md` as the product-level source,
  while keeping syntax/behavior details in `SPEC.md`.

SPEC sections / tests:
- SPEC: Section 9 documentation policy note
- Tests: not applicable (docs-only)

Known limitations:
- PRD defines target API direction; class-facade/file-API implementation still
  requires a dedicated backlog task and code changes.

How to reproduce locally (commands):
- `sed -n '1,260p' PRD.md`
- `sed -n '200,360p' SPEC.md`
- `sed -n '1,260p' src/gcode_parser.h`
- `sed -n '1,260p' src/messages.h`

## 2026-02-24 (plan streaming API + benchmark baseline requirements)
- Added backlog tasks `T-019` (streaming parse/lower API) and `T-020`
  (benchmark harness + 10k-line baseline) with acceptance criteria.
- Updated `PRD.md` API/quality requirements to include streaming output support,
  cancellation/limits, and benchmark throughput visibility.
- Updated `SPEC.md` to document planned streaming output mode and benchmark
  expectations in testing requirements.

SPEC sections / tests:
- SPEC: Section 2.2 (planned streaming output mode), Section 7 (benchmark expectations)
- Tests: planning-only (implementation tracked by T-019/T-020)

Known limitations:
- No streaming runtime API or benchmark executable is implemented in this
  change; this is planning/spec alignment only.

How to reproduce locally (commands):
- `sed -n '1,260p' BACKLOG.md`
- `sed -n '1,260p' PRD.md`
- `sed -n '1,360p' SPEC.md`

## 2026-02-24 (T-019 streaming parse/lower API)
- Added callback-based streaming APIs: `lowerToMessagesStream`,
  `parseAndLowerStream`, and `parseAndLowerFileStream` in `src/messages.h/.cpp`.
- Added stream controls (`StreamOptions`) for early stop via max lines/messages/
  diagnostics and cancel hook.
- Added streaming tests (`test/streaming_tests.cpp`) covering 10k-line streaming,
  cancel behavior, rejected-line signaling, and file-based streaming input.

SPEC sections / tests:
- SPEC: Section 2.2 (streaming output mode), Section 6 (streaming API)
- Tests: `StreamingTest.*`, `./dev/check.sh`

Known limitations:
- Current streaming API still parses full input into AST before lowering stream;
  true incremental parsing is not part of this slice.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R StreamingTest`
- `./dev/check.sh`

## 2026-02-24 (T-020 benchmark harness + 10k-line baseline)
- Added benchmark executable `gcode_bench` (`bench/gcode_bench.cpp`) to measure
  parse-only and parse+lower performance on deterministic input.
- Added `dev/bench.sh` and benchmark output artifact path
  `output/bench/latest.json` with machine-readable metrics (time, lines/sec,
  bytes/sec).
- Added CI benchmark smoke job (`benchmark-smoke`) as soft-gating
  (`continue-on-error: true`) and integrated local benchmark smoke via ctest
  (`BenchmarkSmoke`).

SPEC sections / tests:
- SPEC: Section 7 (Performance benchmarking expectations)
- Tests: `BenchmarkSmoke`, `./dev/check.sh`, `./dev/bench.sh`

Known limitations:
- Benchmark currently uses a deterministic synthetic G1 corpus; broader mixed
  real-world corpora can be added in follow-up work.
- Metrics are wall-clock throughput; hardware-specific CPU cycles are not used
  as CI gating criteria.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R BenchmarkSmoke`
- `./dev/bench.sh`

## 2026-02-27 (T-021 detailed streaming callback message assertions)
- Added streaming unit test that captures callback-emitted messages into a
  vector and validates detailed field content for `G4` and `G1` payloads.
- Updated SPEC testing expectations to require field-level validation for
  streaming callback tests (not just count-based assertions).
- Updated backlog bookkeeping: `T-020` marked done and `T-021` tracked in progress.

SPEC sections / tests:
- SPEC: Section 7 (Testing Expectations)
- Tests: `StreamingTest.CallbackCanCaptureAndValidateDetailedMessages`,
  `./dev/check.sh`

Known limitations:
- This change strengthens tests only; no streaming API behavior changes.

How to reproduce locally (commands):
- `cmake -S . -B build`
- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure -R StreamingTest`
- `./dev/check.sh`
