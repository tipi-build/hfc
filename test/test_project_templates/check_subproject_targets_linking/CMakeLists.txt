
cmake_minimum_required(VERSION 3.30.5)
cmake_policy(SET CMP0168 NEW)

project(
  ModernCMakeExample
  VERSION 1.0
  LANGUAGES CXX)


set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  ${CMAKE_MODULE_PATH}
)

include(HermeticFetchContent)

FetchContent_Declare(
  project-cmake-nested
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-nested.git"
  GIT_TAG "46264f8ac13af27a3c9db0febbff337925c6bd7f"
)

FetchContent_MakeHermetic(project-cmake-nested)

HermeticFetchContent_MakeAvailableAtBuildTime(project-cmake-nested)


add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctions::MathFunctions MathFunctionscbrt::MathFunctionscbrt)

