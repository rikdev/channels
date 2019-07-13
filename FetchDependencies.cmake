function(fetch_dependencies)
  include(FetchContent)
  
  FetchContent_Declare(
    COW
    GIT_REPOSITORY https://github.com/rikdev/cow.git
  )
  
  set(BUILD_TESTING OFF)
  set(ENABLE_TOOLING OFF)
  set(BUILD_EXAMPLES OFF)
  FetchContent_MakeAvailable(COW)
endfunction()
fetch_dependencies()
