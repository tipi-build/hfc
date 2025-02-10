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

set(git_shallow_arg_value "")
if(HFCTEST_DATA_GIT_SHALLOW_VALUE)
  set(git_shallow_arg_value "GIT_SHALLOW" "${HFCTEST_DATA_GIT_SHALLOW_VALUE}")
endif()


FetchContent_Declare(
  dependency
  GIT_REPOSITORY https://github.com/tipi-build/unit-test-cmake-template-2libs.git
  GIT_TAG 9652ce2756ae45f7124ba250ce9d98b7192b6102
  SOURCE_SUBDIR build/cmake
  ${git_shallow_arg_value} # may be empty, may be GIT_SHALLOW ON or GIT_SHALLOW OFF depending on $HFCTEST_DATA_GIT_SHALLOW_VALUE
)

FetchContent_MakeHermetic(
  dependency
  HERMETIC_BUILD_SYSTEM cmake
)

HermeticFetchContent_MakeAvailableAtConfigureTime(dependency)


FetchContent_GetProperties(dependency
  POPULATED result_dependency_POPULATED
  SOURCE_DIR result_dependency_SOURCE_DIR
)


message(STATUS "=======================")
message(STATUS "POPULATED=${result_dependency_POPULATED}")
message(STATUS "SOURCE_DIR=${result_dependency_SOURCE_DIR}")
message(STATUS "=======================")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/dependency_sourcedir.txt" "${result_dependency_SOURCE_DIR}")