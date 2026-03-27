# G-code Syntax Requirements

This subtree collects the review-first parser-input requirements for G-code
syntax. It describes what input forms should be accepted or rejected, not only
what is implemented today.

For each syntax family, review should answer:

- what input forms must be accepted
- what input forms must be rejected
- what diagnostics are required
- what parse structure should be preserved
- what tests and fixtures must exist

## Review Status

Status values used in this subtree:

- `Wanted`
- `Reviewed`
- `Deferred`
- `Rejected`

Current state:

- `Wanted`: initial consolidated checklist extracted from the product spec
  (`../../product/spec/index.md`)
- not yet fully reviewed item-by-item

## Syntax Requirement Areas

- [Line and Program Structure](line_and_program_structure.md)
- [Comments and Whitespace](comments_and_whitespace.md)
- [Motion, Modal, Dwell, and M Functions](motion_modal_dwell_and_m_functions.md)
- [Variables, Control Flow, Subprograms, and Tools](variables_control_flow_subprograms_and_tools.md)

## Requirement Inventory Output

When review is complete, each syntax family should eventually carry:

- reviewed status
- canonical examples
- invalid examples
- owning tests and fixtures
- linked work unit IDs
