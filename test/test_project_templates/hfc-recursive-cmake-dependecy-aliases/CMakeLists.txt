set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
cmake_minimum_required(VERSION 3.27.6)
project(
  ModernCMakeExample
  VERSION 1.0
  LANGUAGES CXX)

if(NOT DEFINED TEST_PARAMETER_DEFINE_ALIAS)
  message(FATAL_ERROR "This test needs a value for TEST_PARAMETER_DEFINE_ALIAS to be defined (ON/OFF)")
endif()

# Notes: this test is designed to validate that HFC behaves like FetchContent
# when "same name" contents are defined in multiple places.
#
# In that case we expect that the "first record wins"
#
# In this case we are providing a "mathlib" from the top level project
# if -DTEST_PARAMETER_DEFINE_ALIAS=ON
# -> in this case we do check that only different-mathlib gets built and used
#

set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  ${CMAKE_MODULE_PATH}
)
include(HermeticFetchContent)

FetchContent_Declare(
  different-mathlib
  GIT_REPOSITORY https://github.com/tipi-build/unit-test-cmake-template-2libs.git
  GIT_TAG 20f983e688fd77961445608d44bc90df15f38e76
)

# In the test case in which we are defining the alias we make the
# targets defined in "different-mathlib" apear as both the original mathlib target names
# as well as their alternative name
#
# In the else case we straight up rename them so that they are not findable
if(TEST_PARAMETER_DEFINE_ALIAS)
  set(different_mathlib_target_aliasing_fn [=[

    # MathFunctionscbrt::MathFunctionscbrt
    if("${TARGET_NAME}" STREQUAL "MathFunctionscbrt::MathFunctionscbrt")
      list(APPEND TARGET_ALIASES "MathFunctionscbrt::MathFunctionscbrt" "AltMath::AltMathCbrt")
    endif()

    # MathFunctions::MathFunctions
    if("${TARGET_NAME}" STREQUAL "MathFunctions::MathFunctions")
      list(APPEND TARGET_ALIASES "MathFunctions::MathFunctions" "AltMath::MathFunctions")
    endif()
  ]=])
else()
  set(different_mathlib_target_aliasing_fn [=[
    # MathFunctionscbrt::MathFunctionscbrt -> AltMath::AltMathCbrt
    if("${TARGET_NAME}" STREQUAL "MathFunctionscbrt::MathFunctionscbrt")
      set(TARGET_ALIASES "AltMath::AltMathCbrt")
    endif()

    # MathFunctions::MathFunctions -> AltMath::MathFunctions
    if("${TARGET_NAME}" STREQUAL "MathFunctions::MathFunctions")
      set(TARGET_ALIASES "AltMath::MathFunctions")
    endif()
  ]=])

endif()

FetchContent_MakeHermetic(
  different-mathlib
  HERMETIC_BUILD_SYSTEM cmake

  # rename the math lib here
  HERMETIC_CREATE_TARGET_ALIASES "${different_mathlib_target_aliasing_fn}"
)

HermeticFetchContent_MakeAvailableAtConfigureTime(different-mathlib)

# for the positive test we're aliasing the different-mathlib as "mathlib" so
# we can verify only one is used!
if(TEST_PARAMETER_DEFINE_ALIAS)
  HermeticFetchContent_AddContentAliases(different-mathlib "mathlib")
endif()

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

get_hermetic_target_cache_file_path(mathlib mathlib_target_cache_file)

if(TEST_PARAMETER_DEFINE_ALIAS)
  if(EXISTS "${mathlib_target_cache_file}")
    message(FATAL_ERROR "Unexpect target cache file for mathlib found")
  endif()
else()
  if(NOT EXISTS "${mathlib_target_cache_file}")
    message(FATAL_ERROR "No target cache file for mathlib")
  endif()
endif()

# testing that cmake can actually do something with those targets
add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctionscbrt::MathFunctionscbrt More_MathFunctions::More_MathFunctions)


