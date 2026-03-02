#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

#include "ail.h"
#include "ail_json.h"
#include "ast_printer.h"
#include "gcode_parser.h"
#include "messages.h"
#include "messages_json.h"
#include "packet.h"
#include "packet_json.h"

namespace {

std::string boolText(bool value) { return value ? "true" : "false"; }

std::string groupText(gcode::ModalGroupId group) {
  return group == gcode::ModalGroupId::Motion ? "GGroup1" : "GGroup2";
}

bool hasErrors(const std::vector<gcode::Diagnostic> &diagnostics) {
  for (const auto &diag : diagnostics) {
    if (diag.severity == gcode::Diagnostic::Severity::Error) {
      return true;
    }
  }
  return false;
}

std::string diagnosticSeverityText(gcode::Diagnostic::Severity severity) {
  return severity == gcode::Diagnostic::Severity::Error ? "error" : "warning";
}

void appendSource(std::ostringstream &out, const gcode::SourceInfo &source) {
  out << " line=" << source.line;
  if (source.line_number.has_value()) {
    out << " n=" << *source.line_number;
  }
}

void appendOptionalDouble(std::ostringstream &out, const char *key,
                          const std::optional<double> &value) {
  if (!value.has_value()) {
    return;
  }
  out << " " << key << "=" << std::setprecision(12) << *value;
}

void appendModal(std::ostringstream &out, const gcode::ModalState &modal) {
  out << " group=" << groupText(modal.group) << " code=" << modal.code
      << " updates=" << boolText(modal.updates_state);
}

std::string exprToDebugText(const std::shared_ptr<gcode::ExprNode> &expr) {
  if (!expr) {
    return "<null>";
  }
  return std::visit(
      [](const auto &node) -> std::string {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, gcode::ExprLiteral>) {
          std::ostringstream out;
          out << std::setprecision(12) << node.value;
          return out.str();
        } else if constexpr (std::is_same_v<T, gcode::ExprVariable>) {
          return node.name;
        } else if constexpr (std::is_same_v<T, gcode::ExprUnary>) {
          return "(" + node.op + exprToDebugText(node.operand) + ")";
        } else {
          return "(" + exprToDebugText(node.lhs) + " " + node.op + " " +
                 exprToDebugText(node.rhs) + ")";
        }
      },
      expr->node);
}

std::string formatLowerDebug(const gcode::MessageResult &result) {
  std::ostringstream out;

  for (const auto &message : result.messages) {
    std::visit(
        [&](const auto &msg) {
          out << "MSG";
          appendSource(out, msg.source);
          appendModal(out, msg.modal);
          if constexpr (std::is_same_v<std::decay_t<decltype(msg)>,
                                       gcode::G1Message>) {
            out << " type=G1";
            appendOptionalDouble(out, "x", msg.target_pose.x);
            appendOptionalDouble(out, "y", msg.target_pose.y);
            appendOptionalDouble(out, "z", msg.target_pose.z);
            appendOptionalDouble(out, "a", msg.target_pose.a);
            appendOptionalDouble(out, "b", msg.target_pose.b);
            appendOptionalDouble(out, "c", msg.target_pose.c);
            appendOptionalDouble(out, "feed", msg.feed);
          } else if constexpr (std::is_same_v<std::decay_t<decltype(msg)>,
                                              gcode::G2Message>) {
            out << " type=G2";
            appendOptionalDouble(out, "x", msg.target_pose.x);
            appendOptionalDouble(out, "y", msg.target_pose.y);
            appendOptionalDouble(out, "z", msg.target_pose.z);
            appendOptionalDouble(out, "a", msg.target_pose.a);
            appendOptionalDouble(out, "b", msg.target_pose.b);
            appendOptionalDouble(out, "c", msg.target_pose.c);
            appendOptionalDouble(out, "i", msg.arc.i);
            appendOptionalDouble(out, "j", msg.arc.j);
            appendOptionalDouble(out, "k", msg.arc.k);
            appendOptionalDouble(out, "r", msg.arc.r);
            appendOptionalDouble(out, "feed", msg.feed);
          } else if constexpr (std::is_same_v<std::decay_t<decltype(msg)>,
                                              gcode::G3Message>) {
            out << " type=G3";
            appendOptionalDouble(out, "x", msg.target_pose.x);
            appendOptionalDouble(out, "y", msg.target_pose.y);
            appendOptionalDouble(out, "z", msg.target_pose.z);
            appendOptionalDouble(out, "a", msg.target_pose.a);
            appendOptionalDouble(out, "b", msg.target_pose.b);
            appendOptionalDouble(out, "c", msg.target_pose.c);
            appendOptionalDouble(out, "i", msg.arc.i);
            appendOptionalDouble(out, "j", msg.arc.j);
            appendOptionalDouble(out, "k", msg.arc.k);
            appendOptionalDouble(out, "r", msg.arc.r);
            appendOptionalDouble(out, "feed", msg.feed);
          } else if constexpr (std::is_same_v<std::decay_t<decltype(msg)>,
                                              gcode::G4Message>) {
            out << " type=G4";
            out << " dwell_mode="
                << (msg.dwell_mode == gcode::DwellMode::Revolutions
                        ? "revolutions"
                        : "seconds");
            out << " dwell=" << std::setprecision(12) << msg.dwell_value;
          }
          out << "\n";
        },
        message);
  }

  for (const auto &rejected : result.rejected_lines) {
    out << "REJECT";
    appendSource(out, rejected.source);
    out << " errors=" << rejected.reasons.size();
    if (!rejected.reasons.empty()) {
      out << " first=\"" << rejected.reasons.front().message << "\"";
    }
    out << "\n";
  }

  for (const auto &diag : result.diagnostics) {
    out << "DIAG line=" << diag.location.line << " col=" << diag.location.column
        << " sev=" << diagnosticSeverityText(diag.severity) << " msg=\""
        << diag.message << "\"\n";
  }

  out << "SUMMARY messages=" << result.messages.size()
      << " rejected=" << result.rejected_lines.size()
      << " diagnostics=" << result.diagnostics.size() << "\n";
  return out.str();
}

std::string formatAilDebug(const gcode::AilResult &result) {
  std::ostringstream out;
  for (const auto &inst : result.instructions) {
    std::visit(
        [&](const auto &i) {
          out << "AIL";
          appendSource(out, i.source);
          if constexpr (std::is_same_v<std::decay_t<decltype(i)>,
                                       gcode::AilLinearMoveInstruction>) {
            out << " kind=motion_linear opcode=G1";
            appendModal(out, i.modal);
            appendOptionalDouble(out, "x", i.target_pose.x);
            appendOptionalDouble(out, "y", i.target_pose.y);
            appendOptionalDouble(out, "z", i.target_pose.z);
            appendOptionalDouble(out, "a", i.target_pose.a);
            appendOptionalDouble(out, "b", i.target_pose.b);
            appendOptionalDouble(out, "c", i.target_pose.c);
            appendOptionalDouble(out, "feed", i.feed);
          } else if constexpr (std::is_same_v<std::decay_t<decltype(i)>,
                                              gcode::AilArcMoveInstruction>) {
            out << " kind=motion_arc opcode=" << (i.clockwise ? "G2" : "G3");
            appendModal(out, i.modal);
            appendOptionalDouble(out, "x", i.target_pose.x);
            appendOptionalDouble(out, "y", i.target_pose.y);
            appendOptionalDouble(out, "z", i.target_pose.z);
            appendOptionalDouble(out, "a", i.target_pose.a);
            appendOptionalDouble(out, "b", i.target_pose.b);
            appendOptionalDouble(out, "c", i.target_pose.c);
            appendOptionalDouble(out, "i", i.arc.i);
            appendOptionalDouble(out, "j", i.arc.j);
            appendOptionalDouble(out, "k", i.arc.k);
            appendOptionalDouble(out, "r", i.arc.r);
            appendOptionalDouble(out, "feed", i.feed);
          } else if constexpr (std::is_same_v<std::decay_t<decltype(i)>,
                                              gcode::AilDwellInstruction>) {
            out << " kind=dwell opcode=G4";
            appendModal(out, i.modal);
            out << " dwell_mode="
                << (i.dwell_mode == gcode::DwellMode::Revolutions
                        ? "revolutions"
                        : "seconds");
            out << " dwell=" << std::setprecision(12) << i.dwell_value;
          } else if constexpr (std::is_same_v<std::decay_t<decltype(i)>,
                                              gcode::AilAssignInstruction>) {
            out << " kind=assign lhs=" << i.lhs << " rhs=\""
                << exprToDebugText(i.rhs) << "\"";
          } else if constexpr (std::is_same_v<std::decay_t<decltype(i)>,
                                              gcode::AilLabelInstruction>) {
            out << " kind=label name=" << i.name;
          } else if constexpr (std::is_same_v<std::decay_t<decltype(i)>,
                                              gcode::AilGotoInstruction>) {
            out << " kind=goto opcode=" << i.opcode << " target=" << i.target
                << " target_kind=" << i.target_kind;
          } else if constexpr (std::is_same_v<std::decay_t<decltype(i)>,
                                              gcode::AilBranchIfInstruction>) {
            out << " kind=branch_if then_opcode=" << i.then_branch.opcode
                << " then_target=" << i.then_branch.target;
            if (i.else_branch.has_value()) {
              out << " else_opcode=" << i.else_branch->opcode
                  << " else_target=" << i.else_branch->target;
            }
          } else {
            out << " kind=sync tag=" << i.sync_tag;
          }
          out << "\n";
        },
        inst);
  }

  for (const auto &rejected : result.rejected_lines) {
    out << "REJECT";
    appendSource(out, rejected.source);
    out << " errors=" << rejected.reasons.size();
    if (!rejected.reasons.empty()) {
      out << " first=\"" << rejected.reasons.front().message << "\"";
    }
    out << "\n";
  }

  for (const auto &diag : result.diagnostics) {
    out << "DIAG line=" << diag.location.line << " col=" << diag.location.column
        << " sev=" << diagnosticSeverityText(diag.severity) << " msg=\""
        << diag.message << "\"\n";
  }

  out << "SUMMARY instructions=" << result.instructions.size()
      << " rejected=" << result.rejected_lines.size()
      << " diagnostics=" << result.diagnostics.size() << "\n";
  return out.str();
}

std::string packetTypeText(gcode::PacketType type) {
  switch (type) {
  case gcode::PacketType::LinearMove:
    return "linear_move";
  case gcode::PacketType::ArcMove:
    return "arc_move";
  case gcode::PacketType::Dwell:
    return "dwell";
  }
  return "linear_move";
}

std::string formatPacketDebug(const gcode::PacketResult &result) {
  std::ostringstream out;
  for (const auto &packet : result.packets) {
    out << "PACKET id=" << packet.packet_id;
    appendSource(out, packet.source);
    appendModal(out, packet.modal);
    out << " type=" << packetTypeText(packet.type);

    std::visit(
        [&](const auto &payload) {
          using T = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<T, gcode::MotionLinearPayload>) {
            appendOptionalDouble(out, "x", payload.target_pose.x);
            appendOptionalDouble(out, "y", payload.target_pose.y);
            appendOptionalDouble(out, "z", payload.target_pose.z);
            appendOptionalDouble(out, "a", payload.target_pose.a);
            appendOptionalDouble(out, "b", payload.target_pose.b);
            appendOptionalDouble(out, "c", payload.target_pose.c);
            appendOptionalDouble(out, "feed", payload.feed);
          } else if constexpr (std::is_same_v<T, gcode::MotionArcPayload>) {
            out << " clockwise=" << boolText(payload.clockwise);
            appendOptionalDouble(out, "x", payload.target_pose.x);
            appendOptionalDouble(out, "y", payload.target_pose.y);
            appendOptionalDouble(out, "z", payload.target_pose.z);
            appendOptionalDouble(out, "a", payload.target_pose.a);
            appendOptionalDouble(out, "b", payload.target_pose.b);
            appendOptionalDouble(out, "c", payload.target_pose.c);
            appendOptionalDouble(out, "i", payload.arc.i);
            appendOptionalDouble(out, "j", payload.arc.j);
            appendOptionalDouble(out, "k", payload.arc.k);
            appendOptionalDouble(out, "r", payload.arc.r);
            appendOptionalDouble(out, "feed", payload.feed);
          } else {
            out << " dwell_mode="
                << (payload.dwell_mode == gcode::DwellMode::Revolutions
                        ? "revolutions"
                        : "seconds");
            out << " dwell=" << std::setprecision(12) << payload.dwell_value;
          }
        },
        packet.payload);
    out << "\n";
  }

  for (const auto &rejected : result.rejected_lines) {
    out << "REJECT";
    appendSource(out, rejected.source);
    out << " errors=" << rejected.reasons.size();
    if (!rejected.reasons.empty()) {
      out << " first=\"" << rejected.reasons.front().message << "\"";
    }
    out << "\n";
  }

  for (const auto &diag : result.diagnostics) {
    out << "DIAG line=" << diag.location.line << " col=" << diag.location.column
        << " sev=" << diagnosticSeverityText(diag.severity) << " msg=\""
        << diag.message << "\"\n";
  }

  out << "SUMMARY packets=" << result.packets.size()
      << " rejected=" << result.rejected_lines.size()
      << " diagnostics=" << result.diagnostics.size() << "\n";
  return out.str();
}

} // namespace

int main(int argc, const char **argv) {
  std::string mode = "parse";
  std::string format = "debug";
  std::string file_path;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--mode") {
      if (i + 1 >= argc) {
        std::cerr
            << "Missing value for --mode (expected parse|ail|packet|lower)\n";
        return 2;
      }
      mode = argv[++i];
      if (mode != "parse" && mode != "ail" && mode != "packet" &&
          mode != "lower") {
        std::cerr << "Unsupported mode: " << mode
                  << " (expected parse|ail|packet|lower)\n";
        return 2;
      }
      continue;
    }

    if (arg == "--format") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --format (expected debug|json)\n";
        return 2;
      }
      format = argv[++i];
      if (format != "json" && format != "debug") {
        std::cerr << "Unsupported format: " << format
                  << " (expected debug|json)\n";
        return 2;
      }
      continue;
    }

    if (!file_path.empty()) {
      std::cerr << "Unexpected extra positional argument. Usage: gcode_parse "
                   "[--mode parse|ail|packet|lower] [--format debug|json] "
                   "<file>\n";
      return 2;
    }
    file_path = arg;
  }

  if (file_path.empty()) {
    std::cerr << "Usage: gcode_parse [--mode parse|ail|packet|lower] "
                 "[--format debug|json] "
                 "<file>\n";
    return 2;
  }

  std::ifstream input_file(file_path, std::ios::in);
  if (!input_file) {
    std::cerr << "Failed to open file: " << file_path << "\n";
    return 2;
  }

  std::stringstream buffer;
  buffer << input_file.rdbuf();

  if (mode == "parse") {
    const auto result = gcode::parse(buffer.str());
    if (format == "json") {
      std::cout << gcode::formatJson(result);
    } else {
      std::cout << gcode::format(result);
    }
    return hasErrors(result.diagnostics) ? 1 : 0;
  }

  if (mode == "ail") {
    gcode::LowerOptions options;
    options.filename = file_path;
    const auto result = gcode::parseAndLowerAil(buffer.str(), options);
    if (format == "json") {
      std::cout << gcode::ailToJsonString(result);
    } else {
      std::cout << formatAilDebug(result);
    }
    return hasErrors(result.diagnostics) ? 1 : 0;
  }

  if (mode == "packet") {
    gcode::LowerOptions options;
    options.filename = file_path;
    const auto result = gcode::parseLowerAndPacketize(buffer.str(), options);
    if (format == "json") {
      std::cout << gcode::packetToJsonString(result);
    } else {
      std::cout << formatPacketDebug(result);
    }
    return hasErrors(result.diagnostics) ? 1 : 0;
  }

  gcode::LowerOptions options;
  options.filename = file_path;
  const auto result = gcode::parseAndLower(buffer.str(), options);
  if (format == "json") {
    std::cout << gcode::toJsonString(result);
  } else {
    std::cout << formatLowerDebug(result);
  }
  return hasErrors(result.diagnostics) ? 1 : 0;
}
