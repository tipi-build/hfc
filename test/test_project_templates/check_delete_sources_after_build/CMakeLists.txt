
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
  mathlib
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git"
  GIT_TAG "ecc756a4c3f1811cdfd637bd6d8f4e3feb6aff92"
)


string(CONFIGURE
  [=[
  set(test_data_injected "@TEST_DATA_INJECTED@")
  if(test_data_injected STREQUAL "")
    message(FATAL_ERROR "No value set for test_data_injected / TEST_DATA_INJECTED")
  endif()
  ]=]
  toolchain_ext
  @ONLY
)


FetchContent_MakeHermetic(
  mathlib
  HERMETIC_TOOLCHAIN_EXTENSION "${toolchain_ext}"
)

if(TEST_DATA_MAKE_AVAILABLE_AT STREQUAL "configuretime")
  HermeticFetchContent_MakeAvailableAtConfigureTime(mathlib)
elseif(TEST_DATA_MAKE_AVAILABLE_AT STREQUAL "buildtime")
  HermeticFetchContent_MakeAvailableAtBuildTime(mathlib)
else()
  message(FATAL_ERROR "Wrong value of TEST_DATA_MAKE_AVAILABLE_AT")
endif()

FetchContent_GetProperties(mathlib
  POPULATED result_mathlib_POPULATED
  SOURCE_DIR result_mathlib_SOURCE_DIR
  BINARY_DIR result_mathlib_BINARY_DIR
)

message(STATUS "=======================")
message(STATUS "mathlib properties:")
message(STATUS "POPULATED=${result_mathlib_POPULATED}")
message(STATUS "SOURCE_DIR=${result_mathlib_SOURCE_DIR}")
message(STATUS "BINARY_DIR=${result_mathlib_BINARY_DIR}")
message(STATUS "=======================")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/mathlib_sourcedir.txt" "${result_dependency_SOURCE_DIR}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/mathlib_binarydir.txt" "${result_mathlib_BINARY_DIR}")

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctions::MathFunctions MathFunctionscbrt::MathFunctionscbrt)
