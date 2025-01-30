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

set(HERMETIC_FETCHCONTENT_REMOVE_BUILD_DIR_AFTER_INSTALL OFF CACHE INTERNAL "Avoid deletion of the binary dirs to enable detection of cache entries")
set(HERMETIC_FETCHCONTENT_REMOVE_SOURCE_DIR_AFTER_INSTALL OFF CACHE INTERNAL "Avoid deletion of the source dirs after install")
include(HermeticFetchContent)

FetchContent_Declare(
  "mathlib"
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git"
  GIT_TAG "c35bc46a3bc0f6ed7b14e11d34602079a3d8ab83"
)

FetchContent_MakeHermetic(
  "mathlib"
  HERMETIC_BUILD_SYSTEM cmake
  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION [=[
    add_library(MathFunctions::headers INTERFACE IMPORTED)
    set_property(TARGET MathFunctions::headers PROPERTY INTERFACE_INCLUDE_DIRECTORIES @HFC_SOURCE_DIR_PLACEHOLDER@)
    set_property(TARGET MathFunctions::headers APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES @HFC_BINARY_DIR_PLACEHOLDER@)
]=]
)

HermeticFetchContent_MakeAvailableAtBuildTime("mathlib")

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctions::headers)

