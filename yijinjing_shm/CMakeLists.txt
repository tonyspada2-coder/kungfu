cmake_minimum_required(VERSION 3.15)
project(yijinjing_shm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(nng REQUIRED)
find_package(rxcpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(tabulate REQUIRED)
find_package(Catch2 REQUIRED)
find_package(SQLite3 REQUIRED)

include_directories(include)

set(YIJINJING_SOURCES
    src/io/io.cpp
    src/io/locator.cpp
    src/io/sqlite.cpp
    src/io/trace.cpp
    src/journal/assemble.cpp
    src/journal/journal.cpp
    src/journal/page.cpp
    src/journal/reader.cpp
    src/journal/writer.cpp
    src/util/MurmurHash3.cpp
    src/util/StackWalker.cpp
    src/util/hash.cpp
    src/util/log.cpp
    src/util/mmap.cpp
    src/util/nanomsg.cpp
    src/util/signal.cpp
    src/util/stacktrace.cpp
    src/util/terminal.cpp
    src/util/time.cpp
)

add_library(yijinjing_shm SHARED ${YIJINJING_SOURCES})

target_link_libraries(yijinjing_shm PRIVATE fmt::fmt-header-only nlohmann_json::nlohmann_json nng::nng rxcpp::rxcpp spdlog::spdlog_header_only tabulate::tabulate SQLite::SQLite3)

if(WIN32)
  set_target_properties(yijinjing_shm PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(BUILD_TESTING)
  enable_testing()
  add_executable(yijinjing_tests tests/test_journal.cpp)
  target_link_libraries(yijinjing_tests PRIVATE yijinjing_shm Catch2::Catch2 fmt::fmt-header-only nlohmann_json::nlohmann_json nng::nng rxcpp::rxcpp spdlog::spdlog_header_only tabulate::tabulate)
  add_test(NAME yijinjing_tests COMMAND yijinjing_tests)
endif()
