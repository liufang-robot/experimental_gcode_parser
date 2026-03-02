cmake_minimum_required(VERSION 3.16)
project(gcode_parser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(GTest REQUIRED)
include(GoogleTest)
find_package(nlohmann_json REQUIRED)

if(NOT ANTLR4_EXECUTABLE AND DEFINED ENV{ANTLR4_EXECUTABLE})
  set(ANTLR4_EXECUTABLE "$ENV{ANTLR4_EXECUTABLE}")
endif()

if(NOT ANTLR4_EXECUTABLE)
  find_program(ANTLR4_EXECUTABLE antlr4)
endif()
if(NOT ANTLR4_EXECUTABLE)
  message(FATAL_ERROR "antlr4 not found. Install it or add to PATH.")
endif()

set(GRAMMAR_FILE ${CMAKE_CURRENT_SOURCE_DIR}/grammar/GCode.g4)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(GENERATED_SOURCES ${GENERATED_DIR}/GCodeLexer.cpp
                      ${GENERATED_DIR}/GCodeParser.cpp)

add_custom_command(
  OUTPUT ${GENERATED_SOURCES}
  BYPRODUCTS ${GENERATED_DIR}/GCodeLexer.h
             ${GENERATED_DIR}/GCodeParser.h
             ${GENERATED_DIR}/GCodeListener.h
             ${GENERATED_DIR}/GCodeBaseListener.h
             ${GENERATED_DIR}/GCodeVisitor.h
             ${GENERATED_DIR}/GCodeBaseVisitor.h
             ${GENERATED_DIR}/GCodeLexer.tokens
             ${GENERATED_DIR}/GCodeParser.tokens
  COMMAND ${ANTLR4_EXECUTABLE} -Dlanguage=Cpp -visitor -listener -o
          ${GENERATED_DIR} ${GRAMMAR_FILE}
  DEPENDS ${GRAMMAR_FILE}
  COMMENT "Generating C++ parser from GCode.g4")

add_custom_target(generate_gcode_parser DEPENDS ${GENERATED_SOURCES})

add_library(gcode_antlr STATIC ${GENERATED_SOURCES})
add_dependencies(gcode_antlr generate_gcode_parser)
target_include_directories(gcode_antlr PUBLIC ${GENERATED_DIR})

if(NOT ANTLR4_RUNTIME_ROOT)
  set(ANTLR4_RUNTIME_ROOT /home/liufang/optcnc/install)
endif()

if(NOT ANTLR4_RUNTIME_INCLUDE_DIR AND DEFINED ENV{ANTLR4_RUNTIME_INCLUDE_DIR})
  set(ANTLR4_RUNTIME_INCLUDE_DIR "$ENV{ANTLR4_RUNTIME_INCLUDE_DIR}")
endif()
if(NOT ANTLR4_RUNTIME_LIB AND DEFINED ENV{ANTLR4_RUNTIME_LIB})
  set(ANTLR4_RUNTIME_LIB "$ENV{ANTLR4_RUNTIME_LIB}")
endif()

if(NOT ANTLR4_RUNTIME_INCLUDE_DIR)
  find_path(
    ANTLR4_RUNTIME_INCLUDE_DIR antlr4-runtime.h
    PATHS ${ANTLR4_RUNTIME_ROOT}/include/antlr4-runtime
    NO_DEFAULT_PATH)
endif()
if(NOT ANTLR4_RUNTIME_LIB)
  find_library(
    ANTLR4_RUNTIME_LIB antlr4-runtime
    PATHS ${ANTLR4_RUNTIME_ROOT}/lib
    NO_DEFAULT_PATH)
endif()

if(NOT ANTLR4_RUNTIME_INCLUDE_DIR OR NOT ANTLR4_RUNTIME_LIB)
  message(
    FATAL_ERROR
      "ANTLR4 C++ runtime not found. Set ANTLR4_RUNTIME_INCLUDE_DIR and ANTLR4_RUNTIME_LIB."
  )
endif()

target_include_directories(gcode_antlr PUBLIC ${ANTLR4_RUNTIME_INCLUDE_DIR})
target_link_libraries(gcode_antlr PUBLIC ${ANTLR4_RUNTIME_LIB})

add_library(
  gcode_parser STATIC src/gcode_parser.cpp src/semantic_rules.cpp
                      src/ast_printer.cpp src/messages.cpp
                      src/ail.cpp src/ail_json.cpp
                      src/packet.cpp src/packet_json.cpp
                      src/lowering_family_factory.cpp src/lowering_family_g1.cpp
                      src/lowering_family_g2.cpp src/lowering_family_g3.cpp
                      src/lowering_family_g4.cpp src/messages_json.cpp
                      src/session.cpp src/message_diff.cpp)
target_include_directories(gcode_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
                                            ${GENERATED_DIR})
target_link_libraries(gcode_parser PUBLIC gcode_antlr)
target_link_libraries(gcode_parser PRIVATE nlohmann_json::nlohmann_json)

add_executable(gcode_parse src/main.cpp)
target_link_libraries(gcode_parse PRIVATE gcode_parser)
target_include_directories(gcode_parse PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(gcode_bench bench/gcode_bench.cpp)
target_link_libraries(gcode_bench PRIVATE gcode_parser)
target_link_libraries(gcode_bench PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(gcode_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

enable_testing()

add_executable(parser_tests test/parser_tests.cpp)
target_link_libraries(parser_tests PRIVATE gcode_parser)
target_link_libraries(parser_tests PRIVATE GTest::gtest_main)
target_include_directories(parser_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(parser_tests PRIVATE
                           GCODE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
gtest_discover_tests(parser_tests DISCOVERY_MODE PRE_TEST)

add_executable(messages_tests test/messages_tests.cpp)
target_link_libraries(messages_tests PRIVATE gcode_parser)
target_link_libraries(messages_tests PRIVATE GTest::gtest_main)
target_include_directories(messages_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
gtest_discover_tests(messages_tests DISCOVERY_MODE PRE_TEST)

add_executable(messages_json_tests test/messages_json_tests.cpp)
target_link_libraries(messages_json_tests PRIVATE gcode_parser)
target_link_libraries(messages_json_tests PRIVATE GTest::gtest_main)
target_include_directories(messages_json_tests
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(messages_json_tests PRIVATE
                           GCODE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
gtest_discover_tests(messages_json_tests DISCOVERY_MODE PRE_TEST)

add_executable(ail_tests test/ail_tests.cpp)
target_link_libraries(ail_tests PRIVATE gcode_parser)
target_link_libraries(ail_tests PRIVATE GTest::gtest_main)
target_link_libraries(ail_tests PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(ail_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
gtest_discover_tests(ail_tests DISCOVERY_MODE PRE_TEST)

add_executable(ail_executor_tests test/ail_executor_tests.cpp)
target_link_libraries(ail_executor_tests PRIVATE gcode_parser)
target_link_libraries(ail_executor_tests PRIVATE GTest::gtest_main)
target_include_directories(ail_executor_tests
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
gtest_discover_tests(ail_executor_tests DISCOVERY_MODE PRE_TEST)

add_executable(ail_lowering_tests test/ail_lowering_tests.cpp)
target_link_libraries(ail_lowering_tests PRIVATE gcode_parser)
target_link_libraries(ail_lowering_tests PRIVATE GTest::gtest_main)
target_link_libraries(ail_lowering_tests PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(ail_lowering_tests
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(ail_lowering_tests PRIVATE
                           GCODE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
gtest_discover_tests(ail_lowering_tests DISCOVERY_MODE PRE_TEST)

add_executable(packet_tests test/packet_tests.cpp)
target_link_libraries(packet_tests PRIVATE gcode_parser)
target_link_libraries(packet_tests PRIVATE GTest::gtest_main)
target_link_libraries(packet_tests PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(packet_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(packet_tests PRIVATE
                           GCODE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
gtest_discover_tests(packet_tests DISCOVERY_MODE PRE_TEST)

add_executable(fuzz_smoke_tests test/fuzz_smoke_tests.cpp)
target_link_libraries(fuzz_smoke_tests PRIVATE gcode_parser)
target_link_libraries(fuzz_smoke_tests PRIVATE GTest::gtest_main)
target_include_directories(fuzz_smoke_tests
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(fuzz_smoke_tests PRIVATE
                           GCODE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
gtest_discover_tests(fuzz_smoke_tests DISCOVERY_MODE PRE_TEST)

add_executable(regression_tests test/regression_tests.cpp)
target_link_libraries(regression_tests PRIVATE gcode_parser)
target_link_libraries(regression_tests PRIVATE GTest::gtest_main)
target_include_directories(regression_tests
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
gtest_discover_tests(regression_tests DISCOVERY_MODE PRE_TEST)

add_executable(cli_tests test/cli_tests.cpp)
target_link_libraries(cli_tests PRIVATE GTest::gtest_main)
target_compile_definitions(cli_tests PRIVATE
                           GCODE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
                           GCODE_PARSE_BIN="$<TARGET_FILE:gcode_parse>")
gtest_discover_tests(cli_tests DISCOVERY_MODE PRE_TEST)

add_executable(session_tests test/session_tests.cpp)
target_link_libraries(session_tests PRIVATE gcode_parser)
target_link_libraries(session_tests PRIVATE GTest::gtest_main)
target_include_directories(session_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
gtest_discover_tests(session_tests DISCOVERY_MODE PRE_TEST)

add_executable(message_diff_tests test/message_diff_tests.cpp)
target_link_libraries(message_diff_tests PRIVATE gcode_parser)
target_link_libraries(message_diff_tests PRIVATE GTest::gtest_main)
target_include_directories(message_diff_tests
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
gtest_discover_tests(message_diff_tests DISCOVERY_MODE PRE_TEST)

add_executable(streaming_tests test/streaming_tests.cpp)
target_link_libraries(streaming_tests PRIVATE gcode_parser)
target_link_libraries(streaming_tests PRIVATE GTest::gtest_main)
target_include_directories(streaming_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
gtest_discover_tests(streaming_tests DISCOVERY_MODE PRE_TEST)

add_executable(semantic_rules_tests test/semantic_rules_tests.cpp)
target_link_libraries(semantic_rules_tests PRIVATE gcode_parser)
target_link_libraries(semantic_rules_tests PRIVATE GTest::gtest_main)
target_include_directories(semantic_rules_tests
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
gtest_discover_tests(semantic_rules_tests DISCOVERY_MODE PRE_TEST)

add_executable(lowering_family_factory_tests test/lowering_family_factory_tests.cpp)
target_link_libraries(lowering_family_factory_tests PRIVATE gcode_parser)
target_link_libraries(lowering_family_factory_tests PRIVATE GTest::gtest_main)
target_include_directories(lowering_family_factory_tests
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
gtest_discover_tests(lowering_family_factory_tests DISCOVERY_MODE PRE_TEST)

add_executable(lowering_family_g1_tests test/lowering_family_g1_tests.cpp)
target_link_libraries(lowering_family_g1_tests PRIVATE gcode_parser)
target_link_libraries(lowering_family_g1_tests PRIVATE GTest::gtest_main)
target_include_directories(lowering_family_g1_tests
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
gtest_discover_tests(lowering_family_g1_tests DISCOVERY_MODE PRE_TEST)

add_executable(lowering_family_g2_tests test/lowering_family_g2_tests.cpp)
target_link_libraries(lowering_family_g2_tests PRIVATE gcode_parser)
target_link_libraries(lowering_family_g2_tests PRIVATE GTest::gtest_main)
target_include_directories(lowering_family_g2_tests
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
gtest_discover_tests(lowering_family_g2_tests DISCOVERY_MODE PRE_TEST)

add_executable(lowering_family_g3_tests test/lowering_family_g3_tests.cpp)
target_link_libraries(lowering_family_g3_tests PRIVATE gcode_parser)
target_link_libraries(lowering_family_g3_tests PRIVATE GTest::gtest_main)
target_include_directories(lowering_family_g3_tests
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
gtest_discover_tests(lowering_family_g3_tests DISCOVERY_MODE PRE_TEST)

add_executable(lowering_family_g4_tests test/lowering_family_g4_tests.cpp)
target_link_libraries(lowering_family_g4_tests PRIVATE gcode_parser)
target_link_libraries(lowering_family_g4_tests PRIVATE GTest::gtest_main)
target_include_directories(lowering_family_g4_tests
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
gtest_discover_tests(lowering_family_g4_tests DISCOVERY_MODE PRE_TEST)

add_test(
  NAME BenchmarkSmoke
  COMMAND gcode_bench --iterations 1 --lines 10000 --output
          ${CMAKE_CURRENT_BINARY_DIR}/bench_smoke.json)
