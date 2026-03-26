# Development: Testing Reference

Use this page for maintainer-facing test entry points and representative
verification locations in the repository.

## Representative Test Files

- `test/messages_tests.cpp`
- `test/streaming_tests.cpp`
- `test/messages_json_tests.cpp`
- `test/regression_tests.cpp`
- `test/semantic_rules_tests.cpp`
- `test/lowering_family_g4_tests.cpp`
- `test/packet_tests.cpp`
- `test/cli_tests.cpp`

## Representative Test Data

- `testdata/packets/*.ngc`
- `testdata/cli/`
- `testdata/execution_contract/core/`

## Primary Local Gate

Run:

```bash
./dev/check.sh
```

Use [Workflow](workflow.md) for the full local validation flow.
