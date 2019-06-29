cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# Declaring
project(Channels
  VERSION 0.2.0
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
find_package(Threads REQUIRED)
target_link_libraries(${LIBRARY_NAME} PUBLIC Threads::Threads)
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
  include/channels/detail/cow_optional.h
  include/channels/detail/future_shared_state.h
  include/channels/detail/intrusive_list.h
  include/channels/detail/lock_view.h
  include/channels/detail/shared_state.h
  include/channels/detail/shared_state_base.h
  include/channels/detail/type_traits.h
  include/channels/detail/view.h
  include/channels/detail/compatibility/apply.h
  include/channels/detail/compatibility/compile_features.h
  include/channels/detail/compatibility/functional.h
  include/channels/detail/compatibility/shared_mutex.h
  include/channels/detail/compatibility/type_traits.h
  include/channels/detail/compatibility/utility.h
  include/channels/utility/callback.h
  include/channels/utility/send_once_limiter.h
  include/channels/utility/sync_connection_manager.h
  include/channels/utility/sync_tracker.h
  src/connection.cpp
  src/error.cpp
  src/detail/intrusive_list.cpp
  src/detail/shared_state_base.cpp
  src/utility/sync_tracker.cpp
)
target_compile_options(${LIBRARY_NAME}
  PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/analyze>
  $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wdocumentation>
)
target_compile_features(${LIBRARY_NAME} PUBLIC cxx_std_14)

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
add_subdirectory(tools)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Examples
option(CHANNELS_BUILD_EXAMPLES "Build examples" OFF)
if(CHANNELS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()