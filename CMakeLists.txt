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
# Adding log_setup log regex to boost as required components
find_package(Boost 1.50 REQUIRED COMPONENTS
  system
  log_setup
  log
  regex
  thread
  filesystem
  date_time
)
message(STATUS "Boost version: ${Boost_VERSION}")

include_directories(include)

# Build the config_parser library from Assignment 1
add_library(config_parser
    src/config/config_parser.cc
)

# startup utilities
add_library(startup_utils
    src/server/startup_utils.cc
)

# Add request handlers library
add_library(request_handlers
    src/server/request_handlers/request_handler.cc
    src/server/request_handlers/echo_handler.cc
    src/server/request_handlers/static_file_handler.cc
    src/server/request_handlers/handler_registry.cc
    src/server/request_handlers/not_found_handler.cc
)

# Add logger library
add_library(logger
  src/util/logger.cc
)

# Connected and linked the server components to Boost
include_directories(
    src/server
    src/config
    src/server/tests
    src/server/request_handlers
    src/util
    tests/
)
add_executable(webserver
    src/server_main.cc
)

add_library(server_logic
  src/server/server.cc
  src/server/session.cc
  src/server/tagged_exceptions.cc
)

# Reformating to  fit in screen. Adding
target_link_libraries(webserver
  Boost::system
  # Adding log_setup and log as dependencies
  Boost::log_setup
  Boost::log
  Boost::regex
  Boost::thread
  Boost::filesystem
  Boost::date_time
  server_logic
  config_parser
  request_handlers
  logger
  startup_utils
)
target_link_libraries(startup_utils
  Boost::system
  Boost::log_setup
  Boost::log
  Boost::regex
  Boost::thread
  Boost::filesystem
  Boost::date_time
  server_logic
  config_parser
  request_handlers
  logger
)
target_link_libraries(server_logic
  request_handlers
  logger
  Boost::log_setup
  Boost::log
  Boost::regex
  Boost::thread
  Boost::filesystem
  Boost::date_time
)
# added logging to consig_parser
target_link_libraries(config_parser
  logger
  Boost::log_setup
  Boost::log
)

# Adding Unit test for config_parser
add_executable(config_parser_test
  tests/config/config_parser_test.cc
)
target_link_libraries(config_parser_test
  config_parser
  gtest_main
  logger
)

# Define macro to access file path from code (for SimpleConfig test)
# DO NOT put files among the tests put them in common_files
target_compile_definitions(config_parser_test PRIVATE
  TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/common_files"
)

gtest_discover_tests(config_parser_test)


# only add tests that all share same target_link_libraries
set(SERVER_TEST_SOURCES
  tests/server/server_test.cc
  tests/server/session_test.cc
  tests/server/request_handlers/request_handler_test.cc
  tests/server/request_handlers/echo_handler_test.cc
  tests/server/request_handlers/static_file_handler_test.cc
  tests/server/request_handlers/not_found_handler_test.cc
  tests/server/tagged_exceptions_test.cc
  tests/server/startup_utils_test.cc
  # Add more test libraries here
)

foreach(test_src ${SERVER_TEST_SOURCES})
  # NAME_WE removes LONGEST extension
  # example: asd.qwe.zxc -> asd
  get_filename_component(test_name ${test_src} NAME_WE)
  # these are aggregated to be used at the end in coverage report
  list(APPEND SERVER_TEST_NAMES ${test_name})

  add_executable(${test_name} ${test_src})

  target_link_libraries(${test_name}
    Boost::filesystem
    Boost::system
    gtest_main
    gmock_main
    server_logic
    request_handlers
    startup_utils
  )

  gtest_discover_tests(${test_name})
endforeach()


# Copy the integration test script to the build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_test.py
     DESTINATION ${CMAKE_BINARY_DIR}
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# allows integration test to access our actual config
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/configs/server_config.txt
     DESTINATION ${CMAKE_BINARY_DIR}
     FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)

# allows integration test to access our real static files
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/static
     DESTINATION ${CMAKE_BINARY_DIR}
     FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)

# Manually add the integration test (To run you can use `ctest -R integration_test`)
add_test(
  NAME integration_test
  COMMAND ${CMAKE_BINARY_DIR}/integration_test.py
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Add environment variable for Python integration tests to access common files
set_tests_properties(integration_test PROPERTIES
  ENVIRONMENT "TEST_DATA_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/common_files"
)

# Add code coverage report
include(cmake/CodeCoverageReportConfig.cmake)
generate_coverage_report(
    TARGETS webserver config_parser server_logic request_handlers startup_utils
    TESTS config_parser_test ${SERVER_TEST_NAMES}
)
