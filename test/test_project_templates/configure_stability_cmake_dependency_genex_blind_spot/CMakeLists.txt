
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

# forward this to the dependencies
set(HERMETIC_FETCHCONTENT_FORWARDED_CMAKE_VARIABLES "HFC_TEST_GENEX_FLAG")

include(HermeticFetchContent)


FetchContent_Declare(
  "mathlib"
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git"
  GIT_TAG "0d338c5f21d1700be6ba5ad736c6f2b7bbb1d522"
)

FetchContent_MakeHermetic(
  "mathlib"
  HERMETIC_BUILD_SYSTEM cmake
  HERMETIC_TOOLCHAIN_EXTENSION [=[
    set(SOME_BUILD_OPTION ON        CACHE BOOL "")
    set(SOME_OTHER_BUILD_OPTION OFF CACHE BOOL "")
    add_compile_definitions(
        TIPI_TEAM=1
        TIPI_TEAM_ZURICH=0
    )
    # Generator expression that reads a non-CMAKE_ cache variable via $CACHE{...}.
    # The GENEX string itself is constant across configure runs: toggling
    # HFC_TEST_GENEX_FLAG between OFF and ON does not change the stored expression.
    # compute_live_toolchain_fingerprint() hashes the literal string, so the
    # fingerprint is identical whether the flag is ON or OFF -- the blind spot.
    add_compile_definitions($<$<BOOL:$CACHE{HFC_TEST_GENEX_FLAG}>:HFC_GENEX_FLAG_ACTIVE>)
  ]=]
)

option(HFC_BUILD_AT_CONFIGURE_TIME "Build mathlib at cmake configure time (vs build time)" OFF)

if(HFC_BUILD_AT_CONFIGURE_TIME)
  HermeticFetchContent_MakeAvailableAtConfigureTime("mathlib")
else()
  HermeticFetchContent_MakeAvailableAtBuildTime("mathlib")
endif()

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctions::MathFunctions MathFunctionscbrt::MathFunctionscbrt)
