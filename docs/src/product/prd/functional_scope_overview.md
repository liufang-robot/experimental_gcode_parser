# PRD: Functional Scope Overview

## 5. Functional Scope

### 5.1 Current Scope

- `G1`, `G2`, `G3`, `G4` syntax, diagnostics, and typed lowering
- fail-fast lowering at the first error line
- JSON round-trip support for message results

### 5.2 Next Scope

- prioritized extension of additional G and M families
- richer syntax coverage and diagnostics
- API facade hardening and file-based API convenience

### 5.2.1 Executable-Action Modeling Contract

Architecture rule:

- every machine-carried action must be represented as an explicit executable
  instruction in IR
- packetization is optional per instruction family
- parse-only metadata may remain non-executable

Each new machine-action feature must define:

- instruction kind and payload
- execution path:
  - packet
  - runtime/control path
- deterministic diagnostic and policy behavior
