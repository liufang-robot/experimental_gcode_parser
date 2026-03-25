# Experimental G-code Parser Docs

This mdBook is the canonical documentation entry point for this repository.

Use these sections:

- [Product Reference](product/index.md): product goals, behavior contract, and
  implemented program reference.
- [Execution Workflow](execution_workflow.md): the supported public execution
  model built around `ExecutionSession`.
- [Execution Contract Review](execution_contract_review.md): the reviewed
  public fixture model and generated review site.
- [Requirements](requirements/index.md): reviewed syntax, semantic, and
  execution requirements.
- [Project Planning](project/index.md): roadmap and backlog.
- [Development Reference](development/index.md): workflow, OODA, architecture,
  and design notes.

Direct generated review entry:
[Open the generated execution contract review site](generated/execution-contract-review/index.html)

Policy:

- Canonical project docs live in `docs/src/`.
- Root `README.md` is the repository entry page.
- Root `AGENTS.md` is the agent startup map, not the canonical docs body.
- CI builds this book and publishes it from `main`.
