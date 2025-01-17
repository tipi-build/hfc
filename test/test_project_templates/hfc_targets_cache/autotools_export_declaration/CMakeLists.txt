#
# Base hfc targets cache functionality test 
# ensures that packages and properties can be read from package config exports 
#

cmake_minimum_required(VERSION 3.30.5)
cmake_policy(SET CMP0168 NEW)

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
  Iconv
  GIT_REPOSITORY https://github.com/tipi-build/unittest-autotools-sample.git
  GIT_TAG ad80b024eeda8f4c0a96eedf669dc453ed33a094 
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

if(HFCTEST_CONFIGURETIME_DEPENDENCY AND HFCTEST_BUILDTIME_DEPENDENCY)
  set(param_error ON)
else()
  if(HFCTEST_CONFIGURETIME_DEPENDENCY) 
    HermeticFetchContent_MakeAvailableAtConfigureTime(Iconv)
  elseif(HFCTEST_BUILDTIME_DEPENDENCY)
    HermeticFetchContent_MakeAvailableAtBuildTime(Iconv)
  else()
    set(param_error ON)
  endif()
endif()

if(param_error)
  message(FATAL_ERROR "This project needs either HFCTEST_CONFIGURETIME_DEPENDENCY or HFCTEST_BUILDTIME_DEPENDENCY defined")
endif()

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE Iconv::Iconv)

