
cmake_minimum_required(VERSION 3.27.6)
project(
  ModernCMakeExample
  VERSION 1.0
  LANGUAGES CXX)


set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  "${CMAKE_CURRENT_SOURCE_DIR}/projectCmakeModules" # this!
  ${CMAKE_MODULE_PATH}
)

include(HermeticFetchContent)


# this is set by the test program, just making sure we have a value
# when set to ON this should ensure we get to use the FindIconv.cmake module found under ./projectCmakeModules/
if(NOT DEFINED FORCE_SYSTEM_Iconv)
  message(FATAL_ERROR "FORCE_SYSTEM_Iconv not defined")
endif()

# test if data from this build's CMakeCache get forwarded, there's a message() outputtting TESTDATA_INJECTED in our custom findIconv.cmake
# and the test checks if it can find that info
set(HERMETIC_FETCHCONTENT_FORWARDED_CMAKE_VARIABLES "TESTDATA_INJECTED;CMAKE_BUILD_TYPE;CMAKE_CXX_STANDARD;CMAKE_CXX_STANDARD_REQUIRED")

FetchContent_Declare(
  Iconv
  GIT_REPOSITORY "https://github.com/tipi-build/unittest-autotools-sample.git"
  GIT_TAG "ad80b024eeda8f4c0a96eedf669dc453ed33a094"
)

FetchContent_MakeHermetic(
  Iconv
  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION
  [=[
    add_library(Iconv::Iconv STATIC IMPORTED)
    set_property(TARGET Iconv::Iconv PROPERTY IMPORTED_LOCATION "@HFC_PREFIX_PLACEHOLDER@/lib/libiconv.a")
    set_property(TARGET Iconv::Iconv PROPERTY INTERFACE_INCLUDE_DIRECTORIES @HFC_PREFIX_PLACEHOLDER@/include)
  ]=]
  HERMETIC_BUILD_SYSTEM autotools
)

HermeticFetchContent_MakeAvailableAtConfigureTime("Iconv")


if(NOT TARGET Iconv::Iconv)
  message(FATAL_ERROR "Could not find target Iconv::Iconv")
endif()

# our custom findIconv defines this so we can be sure it worked...
get_target_property(iconv_LABELS Iconv::Iconv LABELS)

if(NOT iconv_LABELS STREQUAL "LABEL_BY_HFC")
  message(FATAL_ERROR "iconv_LABELS not set as expected... it seems I iconv was not provided through our custom findIconv module")
endif()


add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE Iconv::Iconv)
