#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "gcode/execution_session.h"
#include "streaming_event_log.h"

namespace {

void printUsage() {
  std::cerr << "Usage: gcode_exec_session [--format debug|json] "
               "[--replace-editable-suffix <file>] <file>\n";
}

std::optional<std::string> readTextFile(const std::string &path) {
  std::ifstream input_file(path, std::ios::in);
  if (!input_file) {
    return std::nullopt;
  }
  std::stringstream buffer;
  buffer << input_file.rdbuf();
  return buffer.str();
}

} // namespace

int main(int argc, const char **argv) {
  std::string format = "debug";
  std::string replacement_path;
  std::string file_path;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--format") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --format (expected debug|json)\n";
        return 2;
      }
      format = argv[++i];
      if (format != "debug" && format != "json") {
        std::cerr << "Unsupported format: " << format
                  << " (expected debug|json)\n";
        return 2;
      }
      continue;
    }
    if (arg == "--replace-editable-suffix") {
      if (i + 1 >= argc) {
        std::cerr << "Missing file for --replace-editable-suffix\n";
        return 2;
      }
      replacement_path = argv[++i];
      continue;
    }
    if (!file_path.empty()) {
      printUsage();
      return 2;
    }
    file_path = arg;
  }

  if (file_path.empty()) {
    printUsage();
    return 2;
  }

  const auto input_text = readTextFile(file_path);
  if (!input_text.has_value()) {
    std::cerr << "Failed to open file: " << file_path << "\n";
    return 2;
  }

  std::optional<std::string> replacement_text;
  if (!replacement_path.empty()) {
    replacement_text = readTextFile(replacement_path);
    if (!replacement_text.has_value()) {
      std::cerr << "Failed to open replacement file: " << replacement_path
                << "\n";
      return 2;
    }
  }

  gcode::EventLogRecorder recorder;
  gcode::RecordingExecutionSink sink(recorder);
  gcode::ReadyRuntimeRecorder runtime(recorder);
  gcode::NeverCancelled cancellation;
  gcode::LowerOptions options;
  options.filename = file_path;

  gcode::ExecutionSession session(sink, runtime, cancellation, options);
  if (!session.pushChunk(*input_text)) {
    std::cerr << "Failed to enqueue input for execution session\n";
    return 1;
  }

  bool input_finished = false;
  bool replacement_applied = false;

  while (true) {
    const auto step = input_finished ? session.pump() : session.finish();
    input_finished = true;

    if (step.status == gcode::StepStatus::Progress) {
      if (session.state() == gcode::EngineState::Completed) {
        recorder.add({{"event", "session.completed"}});
        break;
      }
      continue;
    }

    if (step.status == gcode::StepStatus::Completed) {
      recorder.add({{"event", "session.completed"}});
      break;
    }

    if (step.status == gcode::StepStatus::Rejected &&
        step.rejected.has_value()) {
      recorder.add(
          {{"event", "session.rejected"},
           {"line", step.rejected->source.line},
           {"params", {{"reason_count", step.rejected->reasons.size()}}}});
      if (replacement_text.has_value() && !replacement_applied &&
          session.replaceEditableSuffix(*replacement_text)) {
        replacement_applied = true;
        recorder.add({{"event", "session.editable_suffix_replaced"},
                      {"line", session.rejectedLine().value_or(0)}});
        continue;
      }
      break;
    }

    if (step.status == gcode::StepStatus::Blocked && step.blocked.has_value()) {
      recorder.add({{"event", "session.blocked"},
                    {"line", step.blocked->line},
                    {"params",
                     {{"kind", step.blocked->token.kind},
                      {"id", step.blocked->token.id},
                      {"reason", step.blocked->reason}}}});
      break;
    }

    if (step.status == gcode::StepStatus::Cancelled) {
      recorder.add({{"event", "session.cancelled"}});
      break;
    }

    recorder.add({{"event", "session.faulted"}});
    break;
  }

  if (format == "json") {
    std::cout << recorder.toJsonLines();
  } else {
    std::cout << recorder.toDebugText();
  }

  return (session.state() == gcode::EngineState::Faulted ||
          session.state() == gcode::EngineState::Rejected)
             ? 1
             : 0;
}
