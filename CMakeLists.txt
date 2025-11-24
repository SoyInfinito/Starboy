cmake_minimum_required(VERSION 3.16)
project(starboy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

add_executable(starboy src/main.cpp)

if(TARGET SDL2::SDL2)
  # Link both SDL2 and SDL2main on Windows so the CRT entry point is satisfied
  target_link_libraries(starboy PRIVATE SDL2::SDL2 SDL2::SDL2main)
else()
  target_include_directories(starboy PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(starboy PRIVATE ${SDL2_LIBRARIES})
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
