# Experimental G-code Parser Docs

This mdBook is the product documentation entrypoint for this repository.

Use these sections:

- Development Reference: architecture, build/test workflow, contribution gates.
- Program Reference: implemented G-code function syntax and output behavior.

Policy:

- Code changes that modify parser/lowering behavior must update this book in the
  same PR.
- CI builds this book and publishes it to GitHub Pages from `main`.
