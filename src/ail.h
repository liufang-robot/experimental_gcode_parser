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

#include "machine_profile.h"
#include "messages.h"
#include "subprogram_policy.h"
#include "tool_selection.h"

namespace gcode {
class ToolPolicy;
enum class RapidInterpolationMode { Linear, NonLinear };
enum class ToolRadiusCompMode { Off, Left, Right };
enum class WorkingPlane { XY, ZX, YZ };
enum class ToolActionTiming { Immediate, DeferredUntilM6 };

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
  std::vector<MessageResult::RejectedLine> rejected_lines;
};

AilResult lowerToAil(const Program &program,
                     const std::vector<Diagnostic> &parse_diagnostics,
                     const LowerOptions &options = {});

AilResult parseAndLowerAil(std::string_view input,
                           const LowerOptions &options = {});

enum class ConditionResolutionKind { True, False, Pending, Error };

struct ConditionResolution {
  ConditionResolutionKind kind = ConditionResolutionKind::Error;
  std::optional<std::string> wait_key;
  std::optional<int64_t> retry_at_ms;
  std::optional<std::string> error_message;
};

using ConditionResolver =
    std::function<ConditionResolution(const Condition &, const SourceInfo &)>;

enum class ExecutorStatus { Ready, BlockedOnCondition, Completed, Fault };

struct ExecutorBlockedState {
  size_t instruction_index = 0;
  std::optional<std::string> wait_key;
  std::optional<int64_t> retry_at_ms;
};

struct ExecutorState {
  ExecutorStatus status = ExecutorStatus::Ready;
  size_t pc = 0;
  size_t call_stack_depth = 0;
  std::optional<RapidInterpolationMode> rapid_mode_current;
  std::optional<ToolRadiusCompMode> tool_radius_comp_current;
  std::optional<WorkingPlane> working_plane_current;
  std::optional<ToolSelectionState> active_tool_selection;
  std::optional<ToolSelectionState> pending_tool_selection;
  std::optional<ExecutorBlockedState> blocked;
  std::optional<std::string> fault_message;
};

class AilExecutor {
public:
  explicit AilExecutor(
      std::vector<AilInstruction> instructions,
      ErrorPolicy unknown_mcode_policy = ErrorPolicy::Error,
      ErrorPolicy m6_without_pending_policy = ErrorPolicy::Error,
      std::shared_ptr<const ToolPolicy> tool_policy = nullptr,
      ErrorPolicy unresolved_subprogram_policy = ErrorPolicy::Error,
      SubprogramSearchPolicy subprogram_search_policy =
          SubprogramSearchPolicy::ExactOnly,
      std::shared_ptr<const SubprogramPolicy> subprogram_policy = nullptr);

  const ExecutorState &state() const { return state_; }
  const std::vector<Diagnostic> &diagnostics() const { return diagnostics_; }

  void notifyEvent(const std::string &wait_key);
  bool step(int64_t now_ms, const ConditionResolver &resolver);

private:
  struct SubprogramCallFrame {
    size_t return_pc = 0;
    size_t target_pc = 0;
    int64_t remaining_repeats = 1;
  };

  std::optional<size_t> resolveGotoTarget(size_t current_index,
                                          const AilGotoInstruction &inst);
  bool evaluateBranchAtPc(int64_t now_ms, const ConditionResolver &resolver);
  bool handleMCodeAtPc();
  bool handleToolSelectAtPc();
  bool handleToolChangeAtPc();
  bool advanceOneInstruction(int64_t now_ms, const ConditionResolver &resolver);
  void addFault(const SourceInfo &source, const std::string &message);
  void addWarning(const SourceInfo &source, const std::string &message);

  std::vector<AilInstruction> instructions_;
  std::unordered_map<std::string, std::vector<size_t>> label_positions_;
  std::unordered_map<int, std::vector<size_t>> line_number_positions_;
  std::vector<SubprogramCallFrame> call_stack_frames_;
  std::unordered_set<std::string> pending_events_;
  ErrorPolicy unknown_mcode_policy_ = ErrorPolicy::Error;
  ErrorPolicy m6_without_pending_policy_ = ErrorPolicy::Error;
  std::shared_ptr<const ToolPolicy> tool_policy_;
  std::shared_ptr<const SubprogramPolicy> subprogram_policy_;
  ExecutorState state_;
  std::vector<Diagnostic> diagnostics_;
};

} // namespace gcode
