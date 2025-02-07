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
  PATCH_COMMAND "${CMAKE_COMMAND} -E environment"
)

FetchContent_MakeHermetic(
  "mathlib"
  HERMETIC_BUILD_SYSTEM cmake
  HERMETIC_PREPATCHED_RESOLVER [=[
  if(${GIT_TAG} STREQUAL "ecc756a4c3f1811cdfd637bd6d8f4e3feb6aff92")
     set(GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs-simulation-prepatch")
     set(GIT_TAG "7b661a3c99164648f2209b811cea4d12b9858e67")
     set(RESOLVED_PATCH TRUE)
  endif()
 ]=]
)

HermeticFetchContent_MakeAvailableAtConfigureTime("mathlib")

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctions::MathFunctions MathFunctionscbrt::MathFunctionscbrt MathFunctionsmultiplybytwo::MathFunctionsmultiplybytwo)

