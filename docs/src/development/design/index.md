# Architecture and Design

This section contains canonical developer-facing architecture decisions and
implementation-planning references for the parser project.

Top-level pages in this section:

- [Architecture](architecture.md)
- [Repository Implementation Plan](implementation_plan_root.md)
- [Implementation Plan From Requirements](implementation_plan_from_requirements.md)

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
- requirements-driven current-vs-target execution plan
