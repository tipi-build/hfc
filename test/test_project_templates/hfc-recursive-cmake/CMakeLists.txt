set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
cmake_minimum_required(VERSION 3.27.6)
project(
  ModernCMakeExample
  VERSION 1.0
  LANGUAGES CXX)

function(ensure_targets_EXIST target_name)
  foreach(targetname IN LISTS ARGN)
    if(NOT TARGET ${targetname})
      message(FATAL_ERROR "Target ${targetname} does not exist")
    endif()
  endforeach()
endfunction()

function(ensure_targets_UNDEFINED target_name)
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
  other_math
  GIT_REPOSITORY https://github.com/tipi-build/unit-test-cmake-template-use-hfc-cmake
  GIT_TAG 11da31f30d81ce6eaa64dc32e1918ac091201283
)

FetchContent_MakeHermetic(
  other_math
  HERMETIC_BUILD_SYSTEM cmake
)

# preconditions:


# other_math:
# - defines More_MathFunctions::More_MathFunctions
# - has a HFC dependency "mathlib" (which defines MathFunctionscbrt::MathFunctionscbrt)
ensure_targets_UNDEFINED(More_MathFunctions::More_MathFunctions MathFunctionscbrt::MathFunctionscbrt)
HermeticFetchContent_MakeAvailableAtConfigureTime(other_math)
ensure_targets_EXIST(More_MathFunctions::More_MathFunctions MathFunctionscbrt::MathFunctionscbrt)

# testing that cmake can actually do something with those targets
add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctionscbrt::MathFunctionscbrt More_MathFunctions::More_MathFunctions)


