#
# Base hfc targets cache functionality test 
# ensures that packages and properties can be read from package config exports even if the
# file name does not comply with the expected naming by using HERMETIC_DISCOVER_TARGETS_FILE_PATTERN
#

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
  mathslib
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git"
  GIT_TAG "0148c22210327e5253c486fe066a608f9d6a101c"
)

if(HFCTEST_NEGATIVE_CASE)
  FetchContent_MakeHermetic(mathslib) # validate failure if HERMETIC_DISCOVER_TARGETS_FILE_PATTERN is not provided
else()
  FetchContent_MakeHermetic(mathslib
    HERMETIC_DISCOVER_TARGETS_FILE_PATTERN "mathlib_.*_unexpected\\.cmake$"
  )
endif()

if(HFCTEST_CONFIGURETIME_DEPENDENCY AND HFCTEST_BUILDTIME_DEPENDENCY)
  set(param_error ON)
else()
  if(HFCTEST_CONFIGURETIME_DEPENDENCY) 
    HermeticFetchContent_MakeAvailableAtConfigureTime(mathslib)
  elseif(HFCTEST_BUILDTIME_DEPENDENCY)
    HermeticFetchContent_MakeAvailableAtBuildTime(mathslib)
  else()
    set(param_error ON)
  endif()
endif()

if(param_error)
  message(FATAL_ERROR "This project needs either HFCTEST_CONFIGURETIME_DEPENDENCY or HFCTEST_BUILDTIME_DEPENDENCY defined")
endif()

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctions::MathFunctions MathFunctionscbrt::MathFunctionscbrt)


