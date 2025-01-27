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

set(HERMETIC_FETCHCONTENT_REMOVE_BUILD_DIR_AFTER_INSTALL OFF CACHE INTERNAL "Avoid deletion of the binary dirs to enable detection of cache entries")
set(HERMETIC_FETCHCONTENT_REMOVE_SOURCE_DIR_AFTER_INSTALL OFF CACHE INTERNAL "Avoid deletion of the source dirs after install")
include(HermeticFetchContent)

FetchContent_Declare(
  other_math_print_tipi
  GIT_REPOSITORY https://github.com/tipi-build/unit-test-cmake-template-use-hfc-cmake-and-autotools.git
  GIT_TAG fd1064f5232bff1b26f2f06cf3e0f8b446c61a6f 
)

FetchContent_MakeHermetic(
  other_math_print_tipi
  HERMETIC_BUILD_SYSTEM cmake
)

# preconditions:

# other_math_print_tipi:
# - defines Other_MathFunctions::Other_MathFunctions
# - has a HFC dependency/content "mathlib" (defines MathFunctions::MathFunctions and MathFunctionscbrt::MathFunctionscbrt)
# - has a HFC dependency/content "Iconv" (defines Iconv::Iconv)
ensure_targets_UNDEFINED(More_MathFunctions::More_MathFunctions MathFunctionscbrt::MathFunctionscbrt Iconv::Iconv)
HermeticFetchContent_MakeAvailableAtConfigureTime(other_math_print_tipi)
ensure_targets_EXIST(More_MathFunctions::More_MathFunctions MathFunctionscbrt::MathFunctionscbrt Iconv::Iconv)

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE Other_MathFunctions::Other_MathFunctions)


