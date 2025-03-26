cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

project(Visualization)

set(SOURCE_FILES src/main.cpp )

set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/build/)
set(CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/build/)

include_directories(src)

add_executable(Visualization ${SOURCE_FILES})

target_include_directories(Visualization PUBLIC 
  "${PROJECT_BINARY_DIR}" 
  "${PROJECT_SOURCE_DIR}/ui"
  "${PROJECT_SOURCE_DIR}/window")
