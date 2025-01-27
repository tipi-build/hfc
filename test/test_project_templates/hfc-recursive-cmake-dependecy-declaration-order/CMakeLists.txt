set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
cmake_minimum_required(VERSION 3.27.6)
project(
  ModernCMakeExample
  VERSION 1.0
  LANGUAGES CXX)

if(NOT DEFINED TEST_PARAMETER_RUN_NEGATIVE_CASE) 
  message(FATAL_ERROR "This test needs a value for TEST_PARAMETER_RUN_NEGATIVE_CASE to be defined (ON/OFF)")
endif()

# Notes: this test is designed to validate that HFC behaves like FetchContent
# when "same name" contents are defined in multiple places.
#
# In that case we expect that the "first record wins"
#
# In this case we are providing a "mathlib" within a nested HFC project 
# and ensure that an (erroneous) definition from the project level
# does not get used.
#

set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  ${CMAKE_MODULE_PATH}
)
include(HermeticFetchContent)

# in the negative test we expect the build to fail
if(NOT TEST_PARAMETER_RUN_NEGATIVE_CASE)

  FetchContent_Declare(
    other_math
    GIT_REPOSITORY https://github.com/tipi-build/unit-test-cmake-template-use-hfc-cmake
    GIT_TAG 11da31f30d81ce6eaa64dc32e1918ac091201283 
  )

  FetchContent_MakeHermetic(
    other_math
    HERMETIC_BUILD_SYSTEM cmake
  )

  HermeticFetchContent_MakeAvailableAtConfigureTime(other_math)

endif()

# other_math should already provide a mathlib
# so this declaration should not be run
FetchContent_Declare(
  mathlib
  GIT_REPOSITORY "<BROKEN-clone-will-fail-if-this-runs>"
  GIT_TAG "DATA_ERROR"
)

FetchContent_MakeHermetic(
  mathlib
  HERMETIC_BUILD_SYSTEM cmake
  HERMETIC_TOOLCHAIN_EXTENSION 
  [=[
    message(FATAL_ERROR "Duh!")
  ]=]
)

HermeticFetchContent_MakeAvailableAtConfigureTime(mathlib)

# testing that cmake can actually do something with those targets
add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctionscbrt::MathFunctionscbrt)


