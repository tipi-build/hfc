#
# Check if arguments injected through HFC's FIND_PACKAGE_ARGS make it
# to the underlying find_package() call 
#
# WORD OF CAUTION:
# ================
# we're overriding the find_package() function in the toolchain extension to this end. 
# The whole execution is fundamentally broken for any other purpose than checking the 
# arguments being forwarded!
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


# this is set by the test program, just making sure we have a value
# when set to ON this should ensure we get to use the FindIconv.cmake module found under ./projectCmakeModules/
if(NOT DEFINED FORCE_SYSTEM_Iconv)
  message(FATAL_ERROR "FORCE_SYSTEM_Iconv not defined")
endif()


if(NOT DEFINED TEST_INJECT_FIND_PACKAGE_ARGS)
  message(FATAL_ERROR "TEST_INJECT_FIND_PACKAGE_ARGS not defined")
endif()

if(TEST_INJECT_FIND_PACKAGE_ARGS)
  set(find_package_args_value "REQUIRED HFC_TEST_FLAG")
else()
  set(find_package_args_value "REQUIRED")
endif()


FetchContent_Declare(
  Iconv
  GIT_REPOSITORY "https://github.com/tipi-build/unittest-autotools-sample.git"
  GIT_TAG "ad80b024eeda8f4c0a96eedf669dc453ed33a094"
)

FetchContent_MakeHermetic(
  Iconv
  FIND_PACKAGE_ARGS "${find_package_args_value}"
  HERMETIC_TOOLCHAIN_EXTENSION 
  [=[

  # override find_package with one that can check if HFC_TEST_FLAG was set
  function(find_package)
    # check if HFC_TEST_FLAG was set
    if(NOT "HFC_TEST_FLAG" IN_LIST ARGN)
      message(FATAL_ERROR "Flag HFC_TEST_FLAG was not passed...")
    endif()

    include(FindIconv) # yeah...

    #_find_package(${ARGN})
  endfunction()
      
  ]=]
  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION 
  [=[
    message(FATAL_ERROR "This should never be run in this test")
  ]=]
  HERMETIC_BUILD_SYSTEM autotools
)

HermeticFetchContent_MakeAvailableAtConfigureTime("Iconv")


if(NOT TARGET Iconv::Iconv)
  message(FATAL_ERROR "Could not find target Iconv::Iconv")
endif()

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE Iconv::Iconv)