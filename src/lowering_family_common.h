#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>

#include "messages.h"

namespace gcode {

inline bool parseDoubleText(const std::optional<std::string> &text,
                            double *value) {
  if (!text.has_value()) {
    return false;
  }
  try {
    *value = std::stod(*text);
    return true;
  } catch (...) {
    return false;
  }
}

inline bool isWordItem(const LineItem &item) {
  return std::holds_alternative<Word>(item);
}

inline SourceInfo sourceFromLine(const Line &line,
                                 const LowerOptions &options) {
  SourceInfo source;
  source.filename = options.filename;
  source.line = line.line_index;
  if (line.line_number.has_value()) {
    source.line_number = line.line_number->value;
  }
  return source;
}

inline void fillPoseAndFeed(const Line &line, Pose6 *pose,
                            std::optional<double> *feed, ArcParams *arc) {
  for (const auto &item : line.items) {
    if (!isWordItem(item)) {
      continue;
    }
    const auto &word = std::get<Word>(item);
    double parsed = 0.0;
    if (word.head == "X" && parseDoubleText(word.value, &parsed)) {
      pose->x = parsed;
    } else if (word.head == "Y" && parseDoubleText(word.value, &parsed)) {
      pose->y = parsed;
    } else if (word.head == "Z" && parseDoubleText(word.value, &parsed)) {
      pose->z = parsed;
    } else if (word.head == "A" && parseDoubleText(word.value, &parsed)) {
      pose->a = parsed;
    } else if (word.head == "B" && parseDoubleText(word.value, &parsed)) {
      pose->b = parsed;
    } else if (word.head == "C" && parseDoubleText(word.value, &parsed)) {
      pose->c = parsed;
    } else if (word.head == "F" && parseDoubleText(word.value, &parsed)) {
      *feed = parsed;
    } else if (arc && word.head == "I" &&
               parseDoubleText(word.value, &parsed)) {
      arc->i = parsed;
    } else if (arc && word.head == "J" &&
               parseDoubleText(word.value, &parsed)) {
      arc->j = parsed;
    } else if (arc && word.head == "K" &&
               parseDoubleText(word.value, &parsed)) {
      arc->k = parsed;
    } else if (arc && (word.head == "R" || word.head == "CR") &&
               parseDoubleText(word.value, &parsed)) {
      arc->r = parsed;
    }
  }
}

inline void addWarningDiagnostic(std::vector<Diagnostic> *diagnostics,
                                 const Location &location,
                                 const std::string &message) {
  Diagnostic diag;
  diag.severity = Diagnostic::Severity::Warning;
  diag.message = message;
  diag.location = location;
  diagnostics->push_back(std::move(diag));
}

inline void addArcLoweringWarnings(const Line &line,
                                   std::vector<Diagnostic> *diagnostics) {
  static const std::set<std::string> unsupported_heads = {
      "AR", "AP", "RP", "CIP", "CT", "I1", "J1", "K1"};
  for (const auto &item : line.items) {
    if (!isWordItem(item)) {
      continue;
    }
    const auto &word = std::get<Word>(item);
    if (unsupported_heads.find(word.head) != unsupported_heads.end()) {
      addWarningDiagnostic(diagnostics, word.location,
                           "lowering ignored unsupported arc word: " +
                               word.head);
    }
  }
}

} // namespace gcode
