#include "lowering_family_g4.h"

#include "lowering_family_common.h"

namespace gcode {

int G4Lowerer::motionCode() const { return 4; }

ParsedMessage
G4Lowerer::lower(const Line &line, const LowerOptions &options,
                 std::vector<Diagnostic> * /*diagnostics*/) const {
  G4Message message;
  message.source = sourceFromLine(line, options);
  message.modal.group = ModalGroupId::NonModal;
  message.modal.code = "G4";
  message.modal.updates_state = false;

  for (const auto &item : line.items) {
    if (!isWordItem(item)) {
      continue;
    }
    const auto &word = std::get<Word>(item);
    double parsed = 0.0;
    if (word.head == "F" && parseDoubleText(word.value, &parsed)) {
      message.dwell_mode = DwellMode::Seconds;
      message.dwell_value = parsed;
      break;
    }
    if (word.head == "S" && parseDoubleText(word.value, &parsed)) {
      message.dwell_mode = DwellMode::Revolutions;
      message.dwell_value = parsed;
      break;
    }
  }

  return message;
}

} // namespace gcode
