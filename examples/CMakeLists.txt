file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake "")
set(${PROJECT_NAME}_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(timer)
