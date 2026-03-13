# Experimental G-code Parser Docs

This mdBook is the product documentation entrypoint for this repository.

Use these sections:

- Execution Workflow: the supported public execution model built around
  `StreamingExecutionEngine` and `ExecutionSession`.
- Requirements: target syntax, semantic validation, and execution behavior to
  review before implementation work starts.
- Development Reference: build/test workflow, contribution gates, and
  developer architecture/design notes.
- Program Reference: implemented G-code function syntax and output behavior.

Policy:

- Code changes that modify parser/lowering behavior must update this book in the
  same PR.
- CI builds this book and publishes it to GitHub Pages from `main`.
