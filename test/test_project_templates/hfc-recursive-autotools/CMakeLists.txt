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
  print_tipi
  GIT_REPOSITORY https://github.com/tipi-build/unit-test-cmake-template-use-hfc-autotools.git
  GIT_TAG e181239cde6f0c0767b6cedecb253dc16ca1b900 
)

FetchContent_MakeHermetic(
  print_tipi
  HERMETIC_BUILD_SYSTEM cmake
)

# preconditions:

# print_tipi
# - defines print_hello_tipi::print_hello_tipi
# - has a HFC dependency "Iconvs" (which defines Iconv::Iconv)
ensure_targets_UNDEFINED(print_hello_tipi::print_hello_tipi Iconv::Iconv)
HermeticFetchContent_MakeAvailableAtConfigureTime(print_tipi)
ensure_targets_EXIST(print_hello_tipi::print_hello_tipi Iconv::Iconv)

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE Iconv::Iconv print_hello_tipi::print_hello_tipi)
