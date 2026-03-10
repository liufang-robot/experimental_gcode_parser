#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "gcode/streaming_execution_engine.h"
#include "streaming_event_log.h"

namespace {

void printUsage() {
  std::cerr << "Usage: gcode_stream_exec [--format debug|json] <file>\n";
}

} // namespace

int main(int argc, const char **argv) {
  std::string format = "debug";
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

  std::ifstream input_file(file_path, std::ios::in);
  if (!input_file) {
    std::cerr << "Failed to open file: " << file_path << "\n";
    return 2;
  }

  std::stringstream buffer;
  buffer << input_file.rdbuf();

  gcode::EventLogRecorder recorder;
  gcode::RecordingExecutionSink sink(recorder);
  gcode::ReadyRuntimeRecorder runtime(recorder);
  gcode::NeverCancelled cancellation;
  gcode::LowerOptions options;
  options.filename = file_path;

  gcode::StreamingExecutionEngine engine(sink, runtime, cancellation, options);
  engine.pushChunk(buffer.str());

  while (true) {
    const auto step = engine.finish();
    if (step.status == gcode::StepStatus::Progress) {
      if (engine.state() == gcode::EngineState::Completed) {
        break;
      }
      continue;
    }
    if (step.status == gcode::StepStatus::Completed) {
      recorder.add({{"event", "engine.completed"}});
      break;
    }
    if (step.status == gcode::StepStatus::WaitingForInput &&
        step.waiting.has_value()) {
      recorder.add({{"event", "engine.waiting_for_input"},
                    {"line", step.waiting->line},
                    {"params", {{"reason", step.waiting->reason}}}});
      break;
    }
    if (step.status == gcode::StepStatus::Blocked && step.blocked.has_value()) {
      recorder.add({{"event", "engine.blocked"},
                    {"line", step.blocked->line},
                    {"params",
                     {{"kind", step.blocked->token.kind},
                      {"id", step.blocked->token.id},
                      {"reason", step.blocked->reason}}}});
      break;
    }
    if (step.status == gcode::StepStatus::Cancelled) {
      recorder.add({{"event", "engine.cancelled"}});
      break;
    }
    recorder.add({{"event", "engine.faulted"}});
    break;
  }

  if (format == "json") {
    std::cout << recorder.toJsonLines();
  } else {
    std::cout << recorder.toDebugText();
  }
  return engine.state() == gcode::EngineState::Faulted ? 1 : 0;
}
