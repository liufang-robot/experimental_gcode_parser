#include "ail_json.h"

#include <type_traits>

#include <nlohmann/json.hpp>

namespace gcode {
namespace {

std::string severityToString(Diagnostic::Severity severity) {
  return severity == Diagnostic::Severity::Error ? "error" : "warning";
}

nlohmann::json locationToJson(const Location &location) {
  nlohmann::json j;
  j["line"] = location.line;
  j["column"] = location.column;
  return j;
}

nlohmann::json diagnosticToJson(const Diagnostic &diag) {
  nlohmann::json j;
  j["severity"] = severityToString(diag.severity);
  j["message"] = diag.message;
  j["location"] = locationToJson(diag.location);
  return j;
}

nlohmann::json exprToJson(const std::shared_ptr<ExprNode> &expr) {
  if (!expr) {
    return nullptr;
  }
  return std::visit(
      [](const auto &node) -> nlohmann::json {
        using T = std::decay_t<decltype(node)>;
        nlohmann::json j;
        if constexpr (std::is_same_v<T, ExprLiteral>) {
          j["kind"] = "literal";
          j["value"] = node.value;
          j["location"] = locationToJson(node.location);
        } else if constexpr (std::is_same_v<T, ExprVariable>) {
          j["kind"] = node.is_system ? "system_variable" : "variable";
          j["name"] = node.name;
          j["location"] = locationToJson(node.location);
        } else if constexpr (std::is_same_v<T, ExprUnary>) {
          j["kind"] = "unary";
          j["op"] = node.op;
          j["operand"] = exprToJson(node.operand);
          j["location"] = locationToJson(node.location);
        } else {
          j["kind"] = "binary";
          j["op"] = node.op;
          j["lhs"] = exprToJson(node.lhs);
          j["rhs"] = exprToJson(node.rhs);
          j["location"] = locationToJson(node.location);
        }
        return j;
      },
      expr->node);
}

nlohmann::json optionalDoubleToJson(const std::optional<double> &value) {
  return value.has_value() ? nlohmann::json(*value) : nlohmann::json(nullptr);
}

nlohmann::json sourceToJson(const SourceInfo &source) {
  nlohmann::json j;
  j["filename"] = source.filename.has_value() ? nlohmann::json(*source.filename)
                                              : nlohmann::json(nullptr);
  j["line"] = source.line;
  j["line_number"] = source.line_number.has_value()
                         ? nlohmann::json(*source.line_number)
                         : nlohmann::json(nullptr);
  return j;
}

std::string modalGroupToString(ModalGroupId group) {
  return group == ModalGroupId::Motion ? "GGroup1" : "GGroup2";
}

nlohmann::json modalToJson(const ModalState &modal) {
  nlohmann::json j;
  j["group"] = modalGroupToString(modal.group);
  j["code"] = modal.code;
  j["updates_state"] = modal.updates_state;
  return j;
}

nlohmann::json poseToJson(const Pose6 &pose) {
  nlohmann::json j;
  j["x"] = optionalDoubleToJson(pose.x);
  j["y"] = optionalDoubleToJson(pose.y);
  j["z"] = optionalDoubleToJson(pose.z);
  j["a"] = optionalDoubleToJson(pose.a);
  j["b"] = optionalDoubleToJson(pose.b);
  j["c"] = optionalDoubleToJson(pose.c);
  return j;
}

nlohmann::json arcToJson(const ArcParams &arc) {
  nlohmann::json j;
  j["i"] = optionalDoubleToJson(arc.i);
  j["j"] = optionalDoubleToJson(arc.j);
  j["k"] = optionalDoubleToJson(arc.k);
  j["r"] = optionalDoubleToJson(arc.r);
  return j;
}

std::string dwellModeToString(DwellMode mode) {
  return mode == DwellMode::Revolutions ? "revolutions" : "seconds";
}

nlohmann::json rejectedLineToJson(const MessageResult::RejectedLine &line) {
  nlohmann::json j;
  j["source"] = sourceToJson(line.source);
  j["reasons"] = nlohmann::json::array();
  for (const auto &reason : line.reasons) {
    j["reasons"].push_back(diagnosticToJson(reason));
  }
  return j;
}

nlohmann::json instructionToJson(const AilInstruction &instruction) {
  return std::visit(
      [](const auto &inst) -> nlohmann::json {
        using T = std::decay_t<decltype(inst)>;
        nlohmann::json j;
        if constexpr (std::is_same_v<T, AilLinearMoveInstruction>) {
          j["kind"] = "motion_linear";
          j["opcode"] = "G1";
          j["source"] = sourceToJson(inst.source);
          j["modal"] = modalToJson(inst.modal);
          j["target_pose"] = poseToJson(inst.target_pose);
          j["feed"] = optionalDoubleToJson(inst.feed);
        } else if constexpr (std::is_same_v<T, AilArcMoveInstruction>) {
          j["kind"] = "motion_arc";
          j["opcode"] = inst.clockwise ? "G2" : "G3";
          j["clockwise"] = inst.clockwise;
          j["source"] = sourceToJson(inst.source);
          j["modal"] = modalToJson(inst.modal);
          j["target_pose"] = poseToJson(inst.target_pose);
          j["arc"] = arcToJson(inst.arc);
          j["feed"] = optionalDoubleToJson(inst.feed);
        } else if constexpr (std::is_same_v<T, AilDwellInstruction>) {
          j["kind"] = "dwell";
          j["opcode"] = "G4";
          j["source"] = sourceToJson(inst.source);
          j["modal"] = modalToJson(inst.modal);
          j["dwell_mode"] = dwellModeToString(inst.dwell_mode);
          j["dwell_value"] = inst.dwell_value;
        } else if constexpr (std::is_same_v<T, AilAssignInstruction>) {
          j["kind"] = "assign";
          j["source"] = sourceToJson(inst.source);
          j["lhs"] = inst.lhs;
          j["rhs"] = exprToJson(inst.rhs);
        } else if constexpr (std::is_same_v<T, AilLabelInstruction>) {
          j["kind"] = "label";
          j["source"] = sourceToJson(inst.source);
          j["name"] = inst.name;
        } else if constexpr (std::is_same_v<T, AilGotoInstruction>) {
          j["kind"] = "goto";
          j["source"] = sourceToJson(inst.source);
          j["opcode"] = inst.opcode;
          j["target"] = inst.target;
          j["target_kind"] = inst.target_kind;
        } else if constexpr (std::is_same_v<T, AilBranchIfInstruction>) {
          j["kind"] = "branch_if";
          j["source"] = sourceToJson(inst.source);
          j["condition"] = nlohmann::json::object();
          j["condition"]["location"] = locationToJson(inst.condition.location);
          j["condition"]["lhs"] = exprToJson(inst.condition.lhs);
          j["condition"]["op"] = inst.condition.op;
          j["condition"]["rhs"] = exprToJson(inst.condition.rhs);
          j["then"] = nlohmann::json::object();
          j["then"]["opcode"] = inst.then_branch.opcode;
          j["then"]["target"] = inst.then_branch.target;
          j["then"]["target_kind"] = inst.then_branch.target_kind;
          if (inst.else_branch.has_value()) {
            j["else"] = nlohmann::json::object();
            j["else"]["opcode"] = inst.else_branch->opcode;
            j["else"]["target"] = inst.else_branch->target;
            j["else"]["target_kind"] = inst.else_branch->target_kind;
          } else {
            j["else"] = nullptr;
          }
        } else {
          j["kind"] = "sync";
          j["source"] = sourceToJson(inst.source);
          j["sync_tag"] = inst.sync_tag;
        }
        return j;
      },
      instruction);
}

nlohmann::json toJson(const AilResult &result) {
  nlohmann::json j;
  j["schema_version"] = 1;
  j["instructions"] = nlohmann::json::array();
  for (const auto &instruction : result.instructions) {
    j["instructions"].push_back(instructionToJson(instruction));
  }
  j["diagnostics"] = nlohmann::json::array();
  for (const auto &diag : result.diagnostics) {
    j["diagnostics"].push_back(diagnosticToJson(diag));
  }
  j["rejected_lines"] = nlohmann::json::array();
  for (const auto &line : result.rejected_lines) {
    j["rejected_lines"].push_back(rejectedLineToJson(line));
  }
  return j;
}

} // namespace

std::string ailToJsonString(const AilResult &result, bool pretty) {
  const auto json = toJson(result);
  return pretty ? json.dump(2) : json.dump();
}

} // namespace gcode
