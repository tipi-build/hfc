# This test cases tries to ensure that recursive usage of HFC is working as
# expected even is HFC is *also* loaded in a project that is being add_subdirectory'ed
#

set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
cmake_minimum_required(VERSION 3.27.6)
project(
  ModernCMakeExample
  VERSION 1.0
  LANGUAGES CXX)

function(ensure_targets_EXIST)
  foreach(targetname IN LISTS ARGN)
    if(NOT TARGET ${targetname})
      message(FATAL_ERROR "Target ${targetname} does not exist")
    endif()
  endforeach()
endfunction()

function(ensure_targets_UNDEFINED)
  foreach(targetname IN LISTS ARGN)
    if(TARGET ${targetname})
      message(FATAL_ERROR "Target ${targetname} is defined unexpectedly")
    endif()
  endforeach()
endfunction()


set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  ${CMAKE_MODULE_PATH}
)
include(HermeticFetchContent)

FetchContent_Declare(
  toplevel-mathlib
  GIT_REPOSITORY https://github.com/tipi-build/unit-test-cmake-template-2libs.git
  GIT_TAG 20f983e688fd77961445608d44bc90df15f38e76
)

FetchContent_MakeHermetic(
  toplevel-mathlib
  HERMETIC_BUILD_SYSTEM cmake
)

HermeticFetchContent_MakeAvailableAtConfigureTime(toplevel-mathlib)

ensure_targets_EXIST(MathFunctionscbrt::MathFunctionscbrt MathFunctions::MathFunctions)
ensure_targets_UNDEFINED(More_MathFunctions::More_MathFunctions)

FetchContent_Declare(
  more_math_hfc
  GIT_REPOSITORY https://github.com/tipi-build/unit-test-cmake-template-use-hfc-cmake
  GIT_TAG 466e9dc85ab908d9e42b76ccf3c723ecf00b241e
)

FetchContent_Populate(more_math_hfc)

# just make sure that the more_math stuff hasn't been loaded yet
ensure_targets_UNDEFINED(More_MathFunctions)


# other_math:
# - defines More_MathFunctions::More_MathFunctions
# - has a HFC dependency "mathlib" (which defines MathFunctionscbrt::MathFunctionscbrt)
add_subdirectory("${more_math_hfc_SOURCE_DIR}" "${more_math_hfc_BINARY_DIR}")

ensure_targets_EXIST(More_MathFunctions MathFunctionscbrt::MathFunctionscbrt MathFunctions::MathFunctions)

# testing that cmake can actually do something with those targets
add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctionscbrt::MathFunctionscbrt More_MathFunctions)


include(hfc_sbom)

hfc_generate_sbom(
  OUTPUT "hfc-sbom.spdx"
  LICENSE "Proprietary"
  SUPPLIER "tipi technologies AG"
  SUPPLIER_URL "https://tipi.build"
  NO_VERIFY
)

