cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

project(Visualization)

set(SOURCE_FILES src/main.cpp )

set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/build/)
set(CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/build/)

include_directories(src)

find_package(VTK REQUIRED)

include_directories(${VTK_INCLUDE_DIRS})

add_executable(Visualization ${SOURCE_FILES})

add_subdirectory("${PROJECT_SOURCE_DIR}/src/bspline")

target_link_libraries(Visualization PUBLIC BSplineEvaluator)

target_link_libraries(Visualization PUBLIC ${VTK_LIBRARIES})

target_include_directories(Visualization PUBLIC 
  "${PROJECT_BINARY_DIR}" 
  "${PROJECT_SOURCE_DIR}/bspline"
  "${PROJECT_SOURCE_DIR}/window"
  ${VTK_INCLUDE_DIRS}
)