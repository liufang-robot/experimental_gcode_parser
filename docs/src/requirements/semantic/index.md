# G-code Semantic Validation Requirements

This subtree collects non-syntax validation rules. These rules apply after text
has been parsed into a structured form, but before execution.

Use this subtree for questions like:

- the text parses, but is it allowed?
- is a numeric value in range?
- are these words legal together?
- should this produce a warning or an error?

## Review Status

Status values used in this subtree:

- `Wanted`
- `Reviewed`
- `Deferred`
- `Rejected`

Current state:

- `Wanted`: initial checklist extracted from current parser and semantic behavior
+- not yet fully reviewed item-by-item
+
+## Semantic Validation Areas
+
+- [Motion, Dwell, and Modal Validation](motion_dwell_and_modal_validation.md)
+- [Variables, Control Flow, and Subprogram Validation](variables_control_flow_and_subprogram_validation.md)
+- [Diagnostics Policy and Classification](diagnostics_policy_and_classification.md)
+
+## Scope Reminder
+
+- syntax requirements answer whether text can be parsed into structured form
+- semantic requirements answer whether a parsed command, value, or combination
+  is allowed
+- execution requirements answer what should happen for valid input at runtime
+EOF
cat > docs/src/requirements/semantic/motion_dwell_and_modal_validation.md <<'EOF'
# Semantic: Motion, Dwell, and Modal Validation

## Motion Command Validation

Status: `Reviewed` (2026-03-10 review pass)

Rules to review:

- `G1` must not mix Cartesian and polar endpoint modes in one block
- `G0` must not mix Cartesian and polar endpoint modes in one block, if that
  rule is desired consistently
- one conflicting motion-family command per block should produce a diagnostic
- arc-center words must match the effective working plane

Examples:

- valid: `G1 X10 Y20 F100`
- invalid: `G1 X10 AP=90 RP=10`
- invalid under `G17`: `G2 X1 Y2 K3`

Review note:

- family status is reviewed; detailed per-command invalid-combination rules
  still need to be expanded explicitly

## Dwell Validation

Status: `Reviewed` (2026-03-10 review pass)

Rules to review:

- `G4` must be in a separate block
- `G4` requires exactly one dwell mode word
- `G4 F...` dwell value must be positive
- `G4 S...` dwell value must be positive
- non-numeric dwell values are invalid

Examples:

- valid: `G4 F3`
- valid: `G4 S30`
- invalid: `G4 F-3`
- invalid: `G4 F3 S30`
- invalid: `G4`
- invalid: `G4 X10`

Review note:

- family status is reviewed; exact diagnostic wording and severity still belong
  in the diagnostics-policy follow-up

## Tool and Modal Validation

Status: `Reviewed` (2026-03-10 review pass)

Rules to review:

- invalid tool selector form by tool-management mode
- `M6` with no pending tool selection policy classification
- invalid modal family conflicts inside one block
- duplicate modal codes in one block: warning, error, or allowed

Review note:

- family status is reviewed; duplicate-modal severity and tool-policy
  classification still need narrower rule text
