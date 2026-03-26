# Experimental G-code Parser Docs

This mdBook is the canonical documentation entry point for this repository.

Use these sections in this reading order:

- [Requirements](requirements/index.md): the review-first source of truth for
  what the project should support.
- [Product Reference](product/index.md): product goals, public behavior
  contract, and implemented program reference.
- [Execution Workflow](execution_workflow.md): the supported public execution
  model built around `ExecutionSession`.
- [Execution Contract Review](execution_contract_review.md): the reviewed
  public fixture model and generated review site.
- [Development Reference](development/index.md): workflow, OODA, testing,
  architecture, and design notes for maintainers.
- [Project Planning](project/index.md): roadmap, backlog, and work-unit
  selection.

Relationship between the sections:

- `Requirements` says what the project should support.
- `Product Reference` says what the public surface means today.
- `Execution Workflow` explains how to drive the public execution API.
- `Execution Contract Review` shows reviewed trace-based public behavior.
- `Development Reference` is for maintainers, not users of the library.
- `Project Planning` tracks prioritization and unfinished work.

Direct generated review entry:
[Open the generated execution contract review site](generated/execution-contract-review/index.html)

Policy:

- Canonical project docs live in `docs/src/`.
- Root `README.md` is the repository entry page.
- Root `AGENTS.md` is the agent startup map, not the canonical docs body.
- CI builds this book and publishes it from `main`.
