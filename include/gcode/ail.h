#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "gcode/condition_runtime.h"
#include "gcode/lowering_types.h"
#include "gcode/policy_types.h"
#include "gcode/runtime_status.h"

namespace gcode {
class IExecutionRuntime;
class IExecutionSink;
class IRuntime;
enum class RapidInterpolationMode { Linear, NonLinear };
enum class ToolRadiusCompMode { Off, Left, Right };
enum class WorkingPlane { XY, ZX, YZ };
enum class ToolActionTiming { Immediate, DeferredUntilM6 };
enum class SubprogramSearchPolicy { ExactOnly, ExactThenBareName };

struct ToolSelectionState {
  std::optional<int64_t> selector_index;
  std::string selector_value;
};

enum class ToolSelectionResolutionKind { Resolved, Unresolved, Ambiguous };

struct ToolSelectionResolution {
  ToolSelectionResolutionKind kind = ToolSelectionResolutionKind::Resolved;
  ToolSelectionState selection;
  bool substituted = false;
  std::string message;
};

struct SubprogramResolution {
  bool resolved = false;
  std::string resolved_target;
  bool fallback_used = false;
  std::string message;
};

using LabelPositionMap = std::unordered_map<std::string, std::vector<size_t>>;
using ToolSelectionResolver =
    std::function<ToolSelectionResolution(const ToolSelectionState &)>;
using SubprogramTargetResolver = std::function<SubprogramResolution(
    const std::string &, const LabelPositionMap &)>;

struct AilLinearMoveInstruction {
  SourceInfo source;
  ModalState modal;
  std::string opcode = "G1";
  Pose6 target_pose;
  std::optional<double> feed;
  std::optional<RapidInterpolationMode> rapid_mode_effective;
};

struct AilArcMoveInstruction {
  SourceInfo source;
  ModalState modal;
  bool clockwise = true;
  WorkingPlane plane_effective = WorkingPlane::XY;
  Pose6 target_pose;
  ArcParams arc;
  std::optional<double> feed;
};

struct AilDwellInstruction {
  SourceInfo source;
  ModalState modal;
  DwellMode dwell_mode = DwellMode::Seconds;
  double dwell_value = 0.0;
};

struct AilMCodeInstruction {
  SourceInfo source;
  std::optional<int64_t> address_extension;
  int64_t value = 0;
};

struct AilRapidTraverseModeInstruction {
  SourceInfo source;
  RapidInterpolationMode mode = RapidInterpolationMode::Linear;
};

struct AilToolRadiusCompInstruction {
  SourceInfo source;
  ToolRadiusCompMode mode = ToolRadiusCompMode::Off;
};

struct AilWorkingPlaneInstruction {
  SourceInfo source;
  WorkingPlane plane = WorkingPlane::XY;
};

struct AilToolSelectInstruction {
  SourceInfo source;
  std::optional<int64_t> selector_index;
  std::string selector_value;
  ToolActionTiming timing = ToolActionTiming::DeferredUntilM6;
};

struct AilToolChangeInstruction {
  SourceInfo source;
  ToolActionTiming timing = ToolActionTiming::DeferredUntilM6;
};

struct AilReturnBoundaryInstruction {
  SourceInfo source;
  std::string opcode = "RET";
};

struct AilSubprogramCallInstruction {
  SourceInfo source;
  std::string target;
  std::optional<int64_t> repeat_count;
};

// Placeholder variants for upcoming non-motion semantics.
struct AilAssignInstruction {
  SourceInfo source;
  std::string lhs;
  std::shared_ptr<ExprNode> rhs;
};

struct AilLabelInstruction {
  SourceInfo source;
  std::string name;
};

struct AilGotoInstruction {
  SourceInfo source;
  std::string opcode;
  std::string target;
  std::string target_kind;
};

struct AilBranchIfInstruction {
  SourceInfo source;
  Condition condition;
  AilGotoInstruction then_branch;
  std::optional<AilGotoInstruction> else_branch;
};

struct AilSyncInstruction {
  SourceInfo source;
  std::string sync_tag;
};

using AilInstruction =
    std::variant<AilLinearMoveInstruction, AilArcMoveInstruction,
                 AilDwellInstruction, AilMCodeInstruction,
                 AilRapidTraverseModeInstruction, AilToolRadiusCompInstruction,
                 AilWorkingPlaneInstruction, AilToolSelectInstruction,
                 AilToolChangeInstruction, AilReturnBoundaryInstruction,
                 AilSubprogramCallInstruction, AilAssignInstruction,
                 AilLabelInstruction, AilGotoInstruction,
                 AilBranchIfInstruction, AilSyncInstruction>;

struct AilResult {
  std::vector<AilInstruction> instructions;
  std::vector<Diagnostic> diagnostics;
  std::vector<RejectedLine> rejected_lines;
};

AilResult lowerToAil(const Program &program,
                     const std::vector<Diagnostic> &parse_diagnostics,
                     const LowerOptions &options = {});

AilResult parseAndLowerAil(std::string_view input,
                           const LowerOptions &options = {});

enum class ExecutorStatus { Ready, Blocked, Completed, Fault };

struct ExecutorBlockedState {
  size_t instruction_index = 0;
  std::optional<WaitToken> wait_token;
  std::optional<int64_t> retry_at_ms;
  std::optional<ToolSelectionState> tool_change_target_on_resume;
};

struct ExecutorState {
  ExecutorStatus status = ExecutorStatus::Ready;
  size_t pc = 0;
  size_t call_stack_depth = 0;
  std::string motion_code_current = "G1";
  RapidInterpolationMode rapid_mode_current = RapidInterpolationMode::Linear;
  ToolRadiusCompMode tool_radius_comp_current = ToolRadiusCompMode::Off;
  WorkingPlane working_plane_current = WorkingPlane::XY;
  std::optional<ToolSelectionState> active_tool_selection;
  std::optional<ToolSelectionState> pending_tool_selection;
  std::optional<ToolSelectionState> selected_tool_selection;
  std::optional<ExecutorBlockedState> blocked;
  std::optional<std::string> fault_message;
};

struct AilExecutorInitialState {
  std::string motion_code_current = "G1";
  RapidInterpolationMode rapid_mode_current = RapidInterpolationMode::Linear;
  ToolRadiusCompMode tool_radius_comp_current = ToolRadiusCompMode::Off;
  WorkingPlane working_plane_current = WorkingPlane::XY;
  std::optional<ToolSelectionState> active_tool_selection;
  std::optional<ToolSelectionState> pending_tool_selection;
  std::optional<ToolSelectionState> selected_tool_selection;
};

struct AilExecutorOptions {
  ErrorPolicy unknown_mcode_policy = ErrorPolicy::Error;
  ErrorPolicy m6_without_pending_policy = ErrorPolicy::Error;
  ErrorPolicy unresolved_tool_policy = ErrorPolicy::Error;
  ErrorPolicy ambiguous_tool_policy = ErrorPolicy::Error;
  bool allow_tool_substitution = false;
  std::optional<ToolSelectionState> fallback_tool_selection;
  std::unordered_map<std::string, std::string> tool_substitution_map;
  ErrorPolicy unresolved_subprogram_policy = ErrorPolicy::Error;
  SubprogramSearchPolicy subprogram_search_policy =
      SubprogramSearchPolicy::ExactOnly;
  std::unordered_map<std::string, std::string> subprogram_alias_map;
  ToolSelectionResolver tool_selection_resolver;
  SubprogramTargetResolver subprogram_target_resolver;
  std::optional<AilExecutorInitialState> initial_state;
};

class AilExecutor {
public:
  explicit AilExecutor(std::vector<AilInstruction> instructions,
                       AilExecutorOptions options = {});

  const ExecutorState &state() const { return state_; }
  const std::vector<Diagnostic> &diagnostics() const { return diagnostics_; }

  void notifyEvent(const WaitToken &wait_token);
  bool step(int64_t now_ms, IExecutionSink &sink, IExecutionRuntime &runtime);
  bool step(int64_t now_ms, const IExecutionRuntime &runtime);
  bool step(int64_t now_ms, const IConditionResolver &resolver);
  bool step(int64_t now_ms, const ConditionResolver &resolver);

private:
  struct SubprogramCallFrame {
    size_t return_pc = 0;
    size_t target_pc = 0;
    int64_t remaining_repeats = 1;
  };

  std::optional<size_t> resolveGotoTarget(size_t current_index,
                                          const AilGotoInstruction &inst);
  bool evaluateBranchAtPc(int64_t now_ms, const IConditionResolver &resolver);
  bool handleMCodeAtPc();
  bool handleToolSelectAtPc(IExecutionSink *sink = nullptr,
                            IRuntime *runtime = nullptr);
  bool handleToolChangeAtPc(IExecutionSink *sink = nullptr,
                            IRuntime *runtime = nullptr);
  void applyResolvedToolSelection(const ToolSelectionState &selection);
  bool dispatchToolChangeAtPc(const SourceInfo &source,
                              const ToolSelectionState &target_selection,
                              IExecutionSink *sink, IRuntime *runtime);
  bool advanceOneInstruction(int64_t now_ms,
                             const IConditionResolver &resolver);
  bool advanceOneInstruction(int64_t now_ms, const IConditionResolver &resolver,
                             IExecutionSink *sink, IRuntime *runtime);
  void addFault(const SourceInfo &source, const std::string &message);
  void addWarning(const SourceInfo &source, const std::string &message);

  std::vector<AilInstruction> instructions_;
  std::unordered_map<std::string, std::vector<size_t>> label_positions_;
  std::unordered_map<int, std::vector<size_t>> line_number_positions_;
  std::vector<SubprogramCallFrame> call_stack_frames_;
  std::unordered_set<WaitToken, WaitTokenHash> pending_events_;
  AilExecutorOptions options_;
  ExecutorState state_;
  std::vector<Diagnostic> diagnostics_;
};

} // namespace gcode
