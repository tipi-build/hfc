#
# Base hfc targets cache functionality test 
# ensures we can use projects that do not install using add_subdirectory
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
  GIT_TAG "d4593cd90243a7bdd3507a918d735353f51621b1"
)

FetchContent_MakeHermetic(mathslib)

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


