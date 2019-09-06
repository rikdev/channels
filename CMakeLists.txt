cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# Declaring
project(Channels
  VERSION 0.2.1
  DESCRIPTION "Another C++ signal/slot library"
  HOMEPAGE_URL "https://github.com/rikdev/channels"
  LANGUAGES CXX
)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(
    -Werror
    -pedantic
    -pedantic-errors
    -Wall
    -Wextra
    -Wnull-dereference
    -Wconversion
    -Wsign-conversion
  )
endif()

set(LIBRARY_NAME ${PROJECT_NAME})
add_library(${LIBRARY_NAME})
add_library(${PROJECT_NAME}::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME})

# Building
option(FETCH_DEPENDENCIES "Download and install dependencies" OFF)
if(FETCH_DEPENDENCIES)
  include(FetchDependencies.cmake)
else()
  find_package(COW REQUIRED)
endif()
find_package(Threads REQUIRED)

target_link_libraries(${LIBRARY_NAME} PUBLIC Threads::Threads COW::Optional)
target_include_directories(${LIBRARY_NAME}
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/channels>
)
target_sources(${LIBRARY_NAME}
  PRIVATE
  include/channels/aggregating_channel.h
  include/channels/buffered_channel.h
  include/channels/channel.h
  include/channels/channel_traits.h
  include/channels/connection.h
  include/channels/continuation_status.h
  include/channels/error.h
  include/channels/fwd.h
  include/channels/transmitter.h
  include/channels/detail/cast_view.h
  include/channels/detail/future_shared_state.h
  include/channels/detail/intrusive_list.h
  include/channels/detail/range_view.h
  include/channels/detail/shared_state.h
  include/channels/detail/shared_state_base.h
  include/channels/detail/type_traits.h
  include/channels/detail/compatibility/apply.h
  include/channels/detail/compatibility/compile_features.h
  include/channels/detail/compatibility/functional.h
  include/channels/detail/compatibility/shared_mutex.h
  include/channels/detail/compatibility/type_traits.h
  include/channels/utility/executors.h
  include/channels/utility/send_once_limiter.h
  include/channels/utility/sync_connection_manager.h
  include/channels/utility/sync_tracker.h
  include/channels/utility/transponder.h
  include/channels/utility/tuple_elvis.h
  src/connection.cpp
  src/error.cpp
  src/detail/intrusive_list.cpp
  src/detail/shared_state_base.cpp
  src/utility/sync_connection_manager.cpp
  src/utility/sync_tracker.cpp
)
target_compile_options(${LIBRARY_NAME}
  PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/analyze>
  $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wdocumentation>
)
target_compile_features(${LIBRARY_NAME} PUBLIC cxx_std_14)
set_target_properties(${LIBRARY_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Testing
include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

# Installing
include(GNUInstallDirs)
set(PROJECT_EXPORT_NAME ${PROJECT_NAME}Targets)
install(TARGETS ${LIBRARY_NAME}
  EXPORT ${PROJECT_EXPORT_NAME}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ TYPE INCLUDE)

set(PROJECT_INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
install(EXPORT ${PROJECT_EXPORT_NAME}
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${PROJECT_INSTALL_CONFIGDIR}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${PROJECT_NAME}ConfigVersion.cmake
  COMPATIBILITY SameMinorVersion
)
install(FILES
  ${PROJECT_NAME}Config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${PROJECT_INSTALL_CONFIGDIR}
)

# Packaging
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
include(CPack)

# Tooling
option(ENABLE_TOOLING "Enable tooling support" ON)
if(ENABLE_TOOLING)
  add_subdirectory(tools)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# Examples
option(BUILD_EXAMPLES "Build examples" OFF)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
