include(CMakeFindDependencyMacro)
find_dependency(COW)
find_dependency(Threads)

include("${CMAKE_CURRENT_LIST_DIR}/ChannelsTargets.cmake")
