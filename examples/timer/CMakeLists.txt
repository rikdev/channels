cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

add_executable(timer
  cancellation_token.h
  inefficient_timer.cpp
  inefficient_timer.h
  one_thread_executor.cpp
  one_thread_executor.h
  main.cpp
)

find_package(Channels REQUIRED)
target_link_libraries(timer PRIVATE Channels::Channels)
