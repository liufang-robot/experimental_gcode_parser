# Architecture and Design

This section mirrors repository design sources and keeps developer-facing
architecture decisions available in published docs.

Source-of-truth files in repo root:
- `ARCHITECTURE.md`
- `IMPLEMENTATION_PLAN.md`

Pages in this section are split by topic for easier review:
- pipeline/modules
- modal/context strategy
- exact-stop/continuous-path modes (Groups 10/11/12)
- Siemens feedrate model (Modal Group 15)
- Siemens modal Group 7 tool-radius compensation
- Siemens working-plane modal semantics (Group 6)
- Siemens M-code model and execution boundaries
- system-variable selector and runtime evaluation model
- rapid traverse model (`G0`, `RTLION`, `RTLIOF`)
- line-by-line streaming execution with blocking and cancellation
- incremental parse session API
- work-offset model (Group 8 + suppression commands)
- dimensions/units model (Groups 13/14 + `AC/IC` + `DIAM*`)
- tool-change semantics (direct `T` vs deferred `T`+`M6`)
- implementation phases and task ordering
