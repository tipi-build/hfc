
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

HermeticFetchContent_MakeAvailableAtBuildTime(Iconv)

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE Iconv::Iconv)
