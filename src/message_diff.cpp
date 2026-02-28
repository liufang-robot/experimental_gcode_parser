#include "message_diff.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace gcode {
namespace {

int lineOf(const ParsedMessage &message) {
  return std::visit([](const auto &msg) { return msg.source.line; }, message);
}

bool closeEnough(double lhs, double rhs) { return std::abs(lhs - rhs) < 1e-9; }

bool optionalDoubleEqual(const std::optional<double> &lhs,
                         const std::optional<double> &rhs) {
  if (lhs.has_value() != rhs.has_value()) {
    return false;
  }
  if (!lhs.has_value()) {
    return true;
  }
  return closeEnough(*lhs, *rhs);
}

bool sourceEqual(const SourceInfo &lhs, const SourceInfo &rhs) {
  return lhs.filename == rhs.filename && lhs.line == rhs.line &&
         lhs.line_number == rhs.line_number;
}

bool poseEqual(const Pose6 &lhs, const Pose6 &rhs) {
  return optionalDoubleEqual(lhs.x, rhs.x) &&
         optionalDoubleEqual(lhs.y, rhs.y) &&
         optionalDoubleEqual(lhs.z, rhs.z) &&
         optionalDoubleEqual(lhs.a, rhs.a) &&
         optionalDoubleEqual(lhs.b, rhs.b) && optionalDoubleEqual(lhs.c, rhs.c);
}

bool arcEqual(const ArcParams &lhs, const ArcParams &rhs) {
  return optionalDoubleEqual(lhs.i, rhs.i) &&
         optionalDoubleEqual(lhs.j, rhs.j) &&
         optionalDoubleEqual(lhs.k, rhs.k) && optionalDoubleEqual(lhs.r, rhs.r);
}

bool modalEqual(const ModalState &lhs, const ModalState &rhs) {
  return lhs.group == rhs.group && lhs.code == rhs.code &&
         lhs.updates_state == rhs.updates_state;
}

bool messageEqual(const ParsedMessage &lhs, const ParsedMessage &rhs) {
  if (lhs.index() != rhs.index()) {
    return false;
  }
  if (std::holds_alternative<G1Message>(lhs)) {
    const auto &a = std::get<G1Message>(lhs);
    const auto &b = std::get<G1Message>(rhs);
    return sourceEqual(a.source, b.source) && modalEqual(a.modal, b.modal) &&
           poseEqual(a.target_pose, b.target_pose) &&
           optionalDoubleEqual(a.feed, b.feed);
  }
  if (std::holds_alternative<G2Message>(lhs)) {
    const auto &a = std::get<G2Message>(lhs);
    const auto &b = std::get<G2Message>(rhs);
    return sourceEqual(a.source, b.source) && modalEqual(a.modal, b.modal) &&
           poseEqual(a.target_pose, b.target_pose) && arcEqual(a.arc, b.arc) &&
           optionalDoubleEqual(a.feed, b.feed);
  }
  if (std::holds_alternative<G3Message>(lhs)) {
    const auto &a = std::get<G3Message>(lhs);
    const auto &b = std::get<G3Message>(rhs);
    return sourceEqual(a.source, b.source) && modalEqual(a.modal, b.modal) &&
           poseEqual(a.target_pose, b.target_pose) && arcEqual(a.arc, b.arc) &&
           optionalDoubleEqual(a.feed, b.feed);
  }
  const auto &a = std::get<G4Message>(lhs);
  const auto &b = std::get<G4Message>(rhs);
  return sourceEqual(a.source, b.source) && modalEqual(a.modal, b.modal) &&
         a.dwell_mode == b.dwell_mode &&
         closeEnough(a.dwell_value, b.dwell_value);
}

std::map<int, ParsedMessage>
indexByLine(const std::vector<ParsedMessage> &messages) {
  std::map<int, ParsedMessage> indexed;
  for (const auto &message : messages) {
    indexed[lineOf(message)] = message;
  }
  return indexed;
}

} // namespace

MessageDiff diffMessagesByLine(const MessageResult &before,
                               const MessageResult &after) {
  MessageDiff diff;
  const auto before_index = indexByLine(before.messages);
  const auto after_index = indexByLine(after.messages);

  for (const auto &[line, message] : before_index) {
    const auto found = after_index.find(line);
    if (found == after_index.end()) {
      diff.removed_lines.push_back(line);
      continue;
    }
    if (!messageEqual(message, found->second)) {
      diff.updated.push_back({line, found->second});
    }
  }

  for (const auto &[line, message] : after_index) {
    if (before_index.find(line) == before_index.end()) {
      diff.added.push_back({line, message});
    }
  }

  return diff;
}

std::vector<ParsedMessage>
applyMessageDiff(const std::vector<ParsedMessage> &current,
                 const MessageDiff &diff) {
  auto indexed = indexByLine(current);
  for (int line : diff.removed_lines) {
    indexed.erase(line);
  }
  for (const auto &entry : diff.updated) {
    indexed[entry.line] = entry.message;
  }
  for (const auto &entry : diff.added) {
    indexed[entry.line] = entry.message;
  }

  std::vector<ParsedMessage> applied;
  applied.reserve(indexed.size());
  for (const auto &[line, message] : indexed) {
    (void)line;
    applied.push_back(message);
  }
  return applied;
}

} // namespace gcode
