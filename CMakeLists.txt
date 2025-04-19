cmake_minimum_required(VERSION 3.10.0)
project(tutututu-maxverstappen)

# Make sure developers do not run cmake in the main project directory, to keep
# build artifacts from becoming clutter
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed.
    Please make a new directory (called a build directory) and run CMake from there.
    You may need to remove CMakeCache.txt." )
endif()

# Turn on debug builds if we are building in a devel shell
if (CMAKE_BUILD_TYPE STREQUAL "" AND "$ENV{DEVEL_SHELL}" STREQUAL "1")
    message(STATUS "Setting debug build type by default in devel shell")
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Output binaries to a sub directory "bin"
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enable cmake testing
include(CTest)
enable_testing()

# Enable GoogleTest
include(GoogleTest)
add_subdirectory(/usr/src/googletest googletest)

# Enable Boost
# Use static libraries so binaries can be deployed without a full boost install
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.50 REQUIRED COMPONENTS system)
message(STATUS "Boost version: ${Boost_VERSION}")

include_directories(include)

# Build the config_parser library from Assignment 1
add_library(config_parser
    src/config/config_parser.cc
)

# Connected and linked the server components to Boost
include_directories(
    src/server
    src/config
    src/server/tests
)
add_executable(webserver 
    src/server_main.cc
    src/server/server.cc
    src/server/session.cc
)
target_link_libraries(webserver Boost::system config_parser)

# Adding Unit test for config_parser
add_executable(config_parser_test
  src/config/tests/config_parser_test.cc
)
target_link_libraries(config_parser_test
  config_parser
  gtest_main
)

# Define macro to access file path from code (for SimpleConfig test)
target_compile_definitions(config_parser_test PRIVATE
  TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/config/tests"
)

gtest_discover_tests(config_parser_test)

# Adding Unit test for session (echo logic)
add_executable(session_test
  src/server/tests/session_test.cc
  src/server/session.cc
)

target_link_libraries(session_test
  Boost::system
  gtest_main
)

gtest_discover_tests(session_test)

# Adding Unit test for server (factory + DI)
add_executable(server_test
  src/server/tests/server_test.cc
  src/server/server.cc
  src/server/session.cc
)

target_link_libraries(server_test
  Boost::system
  gtest_main
)

gtest_discover_tests(server_test)

# Add code coverage report
include(cmake/CodeCoverageReportConfig.cmake)
generate_coverage_report(
    TARGETS webserver config_parser session_test server_test
    TESTS config_parser_test session_test server_test
)