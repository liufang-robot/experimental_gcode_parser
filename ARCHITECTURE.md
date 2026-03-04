# ARCHITECTURE — Parser and Runtime Design (v1 Draft)

## 1. Purpose
This document defines how the codebase will be organized to satisfy the current
PRD requirements, with a Siemens-840D-compatible syntax model and
configuration-driven runtime semantics.

## 2. Confirmed v1 Decisions
- Siemens syntax compatibility: `Yes`
- Siemens runtime semantics: `Optional via profile/config`
- Baseline controller profile: `840D`
- Unknown M-code policy: `Error`
- Modal conflicts: `Error`
- Machine-related behavior: `Config-driven` (not hardcoded)

## 3. High-Level Pipeline
```mermaid
flowchart LR
  A[NC Text] --> B[Parser<br/>grammar + AST]
  B --> C[Semantic Normalizer<br/>canonical statements]
  C --> D[Modal/Context Engine<br/>group states + validation]
  D --> E[AIL Lowering<br/>deterministic executable IR]
  E --> F[AilExecutor<br/>runtime execution]
  E --> G[Message/Packet Output]
  A --> PS[ParseSession<br/>line edits + resume]
  PS --> B
  PS --> C
  H[MachineProfile + Policies] --> C
  H --> D
  H --> E
  H --> F
```

## 4. Code Organization (Target)
```mermaid
flowchart TB
  subgraph parser["Parser Layer"]
    G4["grammar/GCode.g4"]
    GP["src/gcode_parser.cpp"]
    AST["src/ast.h"]
  end

  subgraph semantic["Semantic Layer (new/expanded)"]
    SN["src/semantic_normalizer.*"]
    SR["src/semantic_rules.cpp"]
  end

  subgraph modal["Modal/Context Layer (new)"]
    ME["src/modal_engine.*"]
    MR["src/modal_registry.*"]
  end

  subgraph ail["AIL Layer"]
    AL["src/ail.cpp"]
    AJ["src/ail_json.cpp"]
  end

  subgraph runtime["Runtime Layer"]
    EX["AilExecutor"]
    PL["Policy Interfaces"]
  end

  G4 --> GP --> AST --> SN --> ME --> AL --> EX
  SR --> SN
  MR --> ME
  AL --> AJ
  PL --> EX
```

## 5. Config and Policy Model
```mermaid
classDiagram
  class MachineProfile {
    +controller: ControllerKind
    +tool_change_mode: ToolChangeMode
    +tool_management: bool
    +unknown_mcode_policy: ErrorPolicy
    +modal_conflict_policy: ErrorPolicy
    +supported_work_offsets: RangeSet
  }

  class ModalRules {
    +group_conflict_matrix
    +group_precedence
    +validateTransition(...)
  }

  class ToolPolicy {
    <<interface>>
    +resolveToolSelection(...)
  }

  class MCodePolicy {
    <<interface>>
    +validateAndMapMCode(...)
  }

  class RuntimePolicy {
    +tool_policy: ToolPolicy
    +mcode_policy: MCodePolicy
  }

  MachineProfile --> ModalRules
  MachineProfile --> RuntimePolicy
```

## 6. Modal State Strategy
- Keep a single modal registry keyed by Siemens group IDs.
- Grouped state includes at minimum:
  - Group 1: motion type family (`G0/G1/G2/G3` as baseline in current scope)
  - Group 6: working-plane selection (`G17/G18/G19`)
  - Group 7: `G40/G41/G42`
  - Group 8: `G500/G54..G57/G505..G599`
  - Group 10: `G60/G64..G645`
  - Group 11: `G9` (block-scope)
  - Group 12: `G601/G602/G603`
  - Group 13: `G70/G71/G700/G710`
  - Group 14: `G90/G91`
  - Group 15: feed type family
- Related non-group/aux state (modeled alongside modal groups):
  - rapid interpolation control (`RTLION` / `RTLIOF`) affecting `G0` behavior
  - block-scope suppress contexts (`G53/G153/SUPA`)

## 6.1 Incremental Parse Session Strategy
- `ParseSession` owns editable source lines and cached lowering boundary metadata.
- Resume contract:
  - preserve immutable prefix before resume line
  - reparse + relower suffix from resume line
  - merge diagnostics/instructions/messages deterministically by 1-based line
- Resume entry points:
  - explicit line (`reparseFromLine(line)`)
  - first error (`reparseFromFirstError()`)
- v1 scope:
  - deterministic prefix/suffix merge semantics and API behavior
  - parser-internal tree-reuse optimization is a later performance enhancement

```mermaid
stateDiagram-v2
  [*] --> BlockStart
  BlockStart --> ApplyModalWords
  ApplyModalWords --> ValidateConflicts
  ValidateConflicts --> EmitError: conflict/invalid
  ValidateConflicts --> ApplyBlockScope
  ApplyBlockScope --> LowerInstruction
  LowerInstruction --> BlockEnd
  BlockEnd --> BlockStart
```

## 7. Parse vs Runtime Responsibility
- Parser layer:
  - recognizes syntax and preserves source form/location
  - does not hardcode machine-specific behaviors
- Semantic/modal layers:
  - resolve meaning under `MachineProfile`
  - enforce modal/conflict policies
- Runtime layer:
  - executes AIL with policy hooks
  - applies machine-specific actions (tool/mcode behavior)

## 8. Current Coverage Snapshot
- Implemented:
  - core `G1/G2/G3/G4` flow
  - control flow core (`goto`, `if_goto`, structured `if/else/endif` lowering)
  - line-number targeting runtime
- Partial:
  - `G0` rapid semantics
  - M-code semantics
  - comment compatibility extensions
- Detailed exact-stop/continuous-path architecture note:
  - [docs/src/design/exactstop_contpath_architecture.md](/home/liufang/optcnc/gcode/docs/src/design/exactstop_contpath_architecture.md)
- Detailed M-code architecture note:
  - [docs/src/design/mcode_architecture.md](/home/liufang/optcnc/gcode/docs/src/design/mcode_architecture.md)
- Detailed rapid-traverse architecture note:
  - [docs/src/design/rapid_traverse_architecture.md](/home/liufang/optcnc/gcode/docs/src/design/rapid_traverse_architecture.md)
- Planned:
  - modal groups 6/7/8/10/11/12/13/14/15 full state model
  - tool management behaviors
  - full feed/dimension/unit/work-offset integration

## 9. Implementation Sequence (High Level)
1. Build modal registry + machine profile skeleton.
2. Move existing modal logic into modal engine.
3. Introduce semantic normalizer output schema.
4. Add one feature family at a time (comments, M, groups, feed, dimensions).
5. Keep each slice test-first with `./dev/check.sh`.
