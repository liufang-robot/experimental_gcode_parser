#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "gcode_parser.h"
#include "messages.h"

namespace {

std::string readFile(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::in);
  std::stringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

std::vector<std::filesystem::path>
collectCorpus(const std::filesystem::path &corpus_dir) {
  std::vector<std::filesystem::path> files;
  for (const auto &entry : std::filesystem::directory_iterator(corpus_dir)) {
    if (entry.is_regular_file()) {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());
  return files;
}

std::string generateInput(std::mt19937 &rng) {
  static constexpr char kChars[] =
      " \t\r\nABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
      "0123456789;()=+-./";
  std::uniform_int_distribution<int> len_dist(0, 256);
  std::uniform_int_distribution<int> char_dist(0, sizeof(kChars) - 2);
  const int len = len_dist(rng);

  std::string text;
  text.reserve(static_cast<size_t>(len));
  for (int i = 0; i < len; ++i) {
    text.push_back(kChars[char_dist(rng)]);
  }
  return text;
}

void parseSmoke(const std::string &input) {
  const auto parsed = gcode::parse(input);
  const auto lowered = gcode::parseAndLower(input);

  // Sanity guard: diagnostics should not explode relative to input size.
  const auto bound = input.size() * 4 + 1024;
  EXPECT_LE(parsed.diagnostics.size(), bound);
  EXPECT_LE(lowered.diagnostics.size(), bound);
}

TEST(FuzzSmokeTest, CorpusAndGeneratedInputsDoNotCrashOrHang) {
  const std::filesystem::path source_dir(GCODE_SOURCE_DIR);
  const auto corpus_dir = source_dir / "testdata" / "fuzz" / "corpus";

  const auto start = std::chrono::steady_clock::now();

  const auto corpus = collectCorpus(corpus_dir);
  ASSERT_FALSE(corpus.empty());
  for (const auto &file : corpus) {
    parseSmoke(readFile(file));
  }

  std::mt19937 rng(0xC0FFEEu);
  for (int i = 0; i < 3000; ++i) {
    parseSmoke(generateInput(rng));
  }

  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - start)
                              .count();
  EXPECT_LT(elapsed_ms, 5000);
}

} // namespace
