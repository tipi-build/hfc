set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
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
  HfcDependencyProvidedLib
  GIT_REPOSITORY https://github.com/tipi-build/unittest-autotools-sample.git
  GIT_TAG ad80b024eeda8f4c0a96eedf669dc453ed33a094 
)

FetchContent_MakeHermetic(
  HfcDependencyProvidedLib
  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION 
     [=[
       add_library(HfcDependencyProvidedLib::HfcDependencyProvidedLib STATIC IMPORTED)
       set_property(TARGET HfcDependencyProvidedLib::HfcDependencyProvidedLib PROPERTY IMPORTED_LOCATION "@HFC_PREFIX_PLACEHOLDER@/lib/libiconv.a")    
       set_property(TARGET HfcDependencyProvidedLib::HfcDependencyProvidedLib PROPERTY INTERFACE_INCLUDE_DIRECTORIES @HFC_PREFIX_PLACEHOLDER@/include)
     ]=]

  HERMETIC_BUILD_SYSTEM autotools
)

HermeticFetchContent_MakeAvailableAtConfigureTime(HfcDependencyProvidedLib)

FetchContent_Declare(
  "mathlib"
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git"
  GIT_TAG "0ad97390f914cbb2a7bbdd035e2bcbd3ec1ac213"
)

set(mathlib_findable_packages "HfcDependencyProvidedLib")

if(TEST_DATA_DISABLE_FIND_HfcDependencyProvidedLib STREQUAL "ON")
  set(mathlib_findable_packages "")
endif()

FetchContent_MakeHermetic(
  "mathlib"
  HERMETIC_BUILD_SYSTEM cmake
  HERMETIC_FIND_PACKAGES "${mathlib_findable_packages}"
)

HermeticFetchContent_MakeAvailableAtBuildTime("mathlib")


add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctions::MathFunctions MathFunctionscbrt::MathFunctionscbrt HfcDependencyProvidedLib::HfcDependencyProvidedLib)

