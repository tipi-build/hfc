set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
cmake_minimum_required(VERSION 3.27.6)
project(
  HfcVersionSteppingTest
  VERSION 1.0
  LANGUAGES CXX)


set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  ${CMAKE_MODULE_PATH}
)
include(HermeticFetchContent)

# ─────────────────────────────────────────────────────────────────────────────
# Dependency 1: autotools library with export declaration & toolchain extension
# ─────────────────────────────────────────────────────────────────────────────
FetchContent_Declare(
  iconv_lib
  URL https://github.com/tipi-build/unittest-autotools-sample/archive/refs/heads/master.zip
  URL_HASH SHA1=fc009356c4657d2aea3f609e0cbd7ac46dc9c1d1
)

FetchContent_MakeHermetic(
  iconv_lib
  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION
     [=[
       add_library(Iconv::Iconv STATIC IMPORTED)
       set_property(TARGET Iconv::Iconv PROPERTY IMPORTED_LOCATION "@HFC_PREFIX_PLACEHOLDER@/lib/libiconv.a")
       set_property(TARGET Iconv::Iconv PROPERTY INTERFACE_INCLUDE_DIRECTORIES @HFC_PREFIX_PLACEHOLDER@/include)
     ]=]
  HERMETIC_TOOLCHAIN_EXTENSION
    [=[
      set(CMAKE_POSITION_INDEPENDENT_CODE ON)
      add_compile_definitions(ICONV_STEPPING_TEST=1)
    ]=]
  HERMETIC_BUILD_SYSTEM autotools
)

HermeticFetchContent_MakeAvailableAtConfigureTime(iconv_lib)


# ─────────────────────────────────────────────────────────────────────────────
# Dependency 2: cmake library with toolchain extension + target aliases
#               This dependency can find_package(iconv_lib) to exercise
#               inter-dependency via HERMETIC_FIND_PACKAGES
# ─────────────────────────────────────────────────────────────────────────────
FetchContent_Declare(
  "mathlib"
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git"
  GIT_TAG "0d338c5f21d1700be6ba5ad736c6f2b7bbb1d522"
)

FetchContent_MakeHermetic(
  "mathlib"
  HERMETIC_BUILD_SYSTEM cmake
  HERMETIC_TOOLCHAIN_EXTENSION [=[
    set(SOME_BUILD_OPTION ON                 CACHE BOOL "")
    set(SOME_OTHER_BUILD_OPTION OFF          CACHE BOOL "")
    add_compile_definitions(
        TIPI_TEAM=1
        TIPI_TEAM_ZURICH=0
        MATHLIB_STEPPING_TEST=1
    )
  ]=]

  HERMETIC_FIND_PACKAGES "iconv_lib"

  HERMETIC_CREATE_TARGET_ALIASES [=[
    # Keep original names AND add an alias
    if("${TARGET_NAME}" STREQUAL "MathFunctions::MathFunctions")
      list(APPEND TARGET_ALIASES "AltMath::MathFunctions")
    endif()
  ]=]
)

HermeticFetchContent_MakeAvailableAtConfigureTime("mathlib")


# ─────────────────────────────────────────────────────────────────────────────
# Final executable linking all targets (original + aliased)
# ─────────────────────────────────────────────────────────────────────────────
add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE
  Iconv::Iconv
  AltMath::MathFunctions
  MathFunctionscbrt::MathFunctionscbrt
)
