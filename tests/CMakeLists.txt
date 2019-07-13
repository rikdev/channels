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
  executors_test.cpp
  send_once_limiter_test.cpp
  sync_tracker_test.cpp
  sync_connection_manager_test.cpp
  transponder_test.cpp
  # function main
  main.cpp
)

add_library(unit_test_tools STATIC
  tools/callbacks.cpp
  tools/callbacks.h
  tools/exception_helpers.cpp
  tools/exception_helpers.h
  tools/executor.cpp
  tools/executor.h
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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
