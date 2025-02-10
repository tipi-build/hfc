set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
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

include(HermeticFetchContent)

FetchContent_Declare(
  "mathlib"
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git"
  GIT_TAG "ecc756a4c3f1811cdfd637bd6d8f4e3feb6aff92"
  PATCH_COMMAND ${CMAKE_COMMAND} -E rename "MathFunctionscbrt.h" "MathFunctionscbrt-MOVED-BY-PATCH_COMMAND.h"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/thirdparty/patch/CMakeLists.txt"  "CMakeLists.txt"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/thirdparty/patch/CMakeLists.txt"  "CMakeLists.txt"
)

FetchContent_MakeHermetic(
  "mathlib"
  HERMETIC_BUILD_SYSTEM cmake
)

HermeticFetchContent_MakeAvailableAtBuildTime("mathlib")


add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctions::MathFunctions MathFunctionscbrt::MathFunctionscbrt)

