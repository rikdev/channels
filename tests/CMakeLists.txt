option(FETCH_DEPENDENCIES "Download and install dependencies" OFF)
if(FETCH_DEPENDENCIES)
  include(FetchDependencies.cmake)
else()
  find_package(Catch2 REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(unit_tests
  # public api tests
  aggregating_channel_test.cpp
  buffered_channel_test.cpp
  channel_test.cpp
  connection_manager_test.cpp
  executors_test.cpp
  new_only_limiter_test.cpp
  send_once_limiter_test.cpp
  sync_tracker_test.cpp
  sync_connection_manager_test.cpp
  transponder_test.cpp
  tuple_elvis_test.cpp
  # function main
  main.cpp
)
target_compile_options(unit_tests
  PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:--coverage>
)
target_link_options(unit_tests
  PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:--coverage>
)

add_library(unit_test_tools STATIC
  tools/callbacks.cpp
  tools/callbacks.h
  tools/exception_helpers.cpp
  tools/exception_helpers.h
  tools/executor.cpp
  tools/executor.h
  tools/relation_only.h
  tools/thread_helpers.h
  tools/tracker.cpp
  tools/tracker.h
)
target_link_libraries(unit_test_tools
  PUBLIC
  Threads::Threads
  Catch2::Catch2
)

target_link_libraries(unit_tests
  PRIVATE
  ${PROJECT_NAME}::${LIBRARY_NAME}
  unit_test_tools
  Threads::Threads
  Catch2::Catch2
)

add_test(NAME unit_tests COMMAND unit_tests)

# tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

## coverage
set(CTEST_SOURCE_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CTEST_BINARY_DIRECTORY "${CMAKE_BINARY_DIR}")
set(SOURCES_LIST_FILE "${CMAKE_CURRENT_BINARY_DIR}/CoverageSources.txt")

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CTEST_COVERAGE_COMMAND "llvm-cov")
  set(CTEST_COVERAGE_EXTRA_FLAGS "gcov")
else()
  set(CTEST_COVERAGE_COMMAND "gcov")
  set(CTEST_COVERAGE_EXTRA_FLAGS)
endif()

get_target_property(
  MAIN_TARGET_SOURCES
  ${PROJECT_NAME}::${LIBRARY_NAME}
  SOURCES
)
file(GENERATE
  OUTPUT "${SOURCES_LIST_FILE}"
  CONTENT "$<JOIN:${MAIN_TARGET_SOURCES},\n>"
)
configure_file(cmake/coverage.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/coverage.cmake"
  @ONLY
)
