#
# Base hfc targets cache functionality test
# ensures that packages and properties can be read from package config exports
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
  GIT_TAG "ecc756a4c3f1811cdfd637bd6d8f4e3feb6aff92"
)

FetchContent_MakeHermetic(mathslib
  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION
    [=[
      add_library(Another::Mathslib STATIC IMPORTED)
      set_property(TARGET Another::Mathslib PROPERTY IMPORTED_LOCATION "@HFC_PREFIX_PLACEHOLDER@/lib/libMathFunctions.a")
      set_property(TARGET Another::Mathslib PROPERTY INTERFACE_INCLUDE_DIRECTORIES @HFC_PREFIX_PLACEHOLDER@/include)
    ]=]
  HERMETIC_BUILD_SYSTEM cmake
)

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
target_link_libraries(MyExample PRIVATE Another::Mathslib)

if (TARGET MathFunctions::MathFunctions)
  message(FATAL_ERROR "Unexpectedly found target MathFunctions::MathFunctions that is exported by the dependency project")
endif()


if(TARGET MathFunctionscbrt::MathFunctionscbrt)
  message(FATAL_ERROR "Unexpectedly found target MathFunctionscbrt::MathFunctionscbrt that is exported by the dependency project")
endif()


