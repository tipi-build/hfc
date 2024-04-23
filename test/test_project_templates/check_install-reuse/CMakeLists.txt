
cmake_minimum_required(VERSION 3.27.6)
project(
  ModernCMakeExample
  VERSION 1.0
  LANGUAGES CXX)


set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  ${CMAKE_MODULE_PATH}
)

include(FetchContent)
include(HermeticFetchContent)

FetchContent_Declare(
  project-cmake-simple
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git"
  GIT_TAG "ecc756a4c3f1811cdfd637bd6d8f4e3feb6aff92"
)

FetchContent_MakeHermetic(project-cmake-simple)

HermeticFetchContent_MakeAvailableAtBuildTime(project-cmake-simple)

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctions::MathFunctions MathFunctionscbrt::MathFunctionscbrt)
add_executable(MySimpleMain simple_main.cpp)


