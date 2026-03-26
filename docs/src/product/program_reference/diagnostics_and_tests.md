# Program Reference: Diagnostics and Test References

## Parser Diagnostics Highlights

Implemented checks include:

- block length limit: 512 chars including LF
- invalid skip-level value (`/0.. /9` enforced)
- invalid or misplaced `N` address words
- duplicate `N` warning when line-number jumps are present
- `G4` block-shape checks
- assignment-shape baseline (`AP90` rejected; `AP=90`, `X1=10` accepted)

## Test References

Representative test locations:

- `test/messages_tests.cpp`
- `test/streaming_tests.cpp`
- `test/messages_json_tests.cpp`
- `test/regression_tests.cpp`
- `test/semantic_rules_tests.cpp`
- `test/lowering_family_g4_tests.cpp`
- `test/packet_tests.cpp`
- `test/cli_tests.cpp`

Representative golden/assets locations:

- `testdata/packets/*.ngc`
- `testdata/cli/`
- `testdata/execution_contract/core/`
