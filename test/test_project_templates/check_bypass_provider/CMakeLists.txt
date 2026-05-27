set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
cmake_minimum_required(VERSION 3.27.6)
project(
  BypassProviderTest
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  ${CMAKE_MODULE_PATH}
)

# This is the key setting under test: Threads must be bypassed to the native find_package
set(HERMETIC_FETCHCONTENT_BYPASS_PROVIDER_FOR_PACKAGES "Threads")

include(HermeticFetchContent)

# ─────────────────────────────────────────────────────────────────────────────
# Dependency 1: autotools library whose export declaration references Threads::Threads
#               in INTERFACE_LINK_LIBRARIES. This mimics real-world libraries (like liblzma)
#               that link pthread publicly.
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
       set_property(TARGET Iconv::Iconv PROPERTY INTERFACE_LINK_LIBRARIES "Threads::Threads")
     ]=]
  HERMETIC_BUILD_SYSTEM autotools
)

HermeticFetchContent_MakeAvailableAtConfigureTime(iconv_lib)


# ─────────────────────────────────────────────────────────────────────────────
# Dependency 2: cmake library that depends on iconv_lib via HERMETIC_FIND_PACKAGES.
#               This exercises the summary file propagation path: when this dep's
#               proxy toolchain registers iconv_lib, and this dep's build consumes
#               iconv_lib's target cache, the INTERFACE_LINK_LIBRARIES "Threads::Threads"
#               triggers find_package(Threads) through the provider again.
# ─────────────────────────────────────────────────────────────────────────────
set(PATCH_INJECT_FIND_PACKAGE_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/patch_inject_find_package.cmake")
file(WRITE "${PATCH_INJECT_FIND_PACKAGE_SCRIPT}" "file(APPEND \"\${SOURCE_DIR}/CMakeLists.txt\" \"\\nfind_package(iconv_lib QUIET)\\n\")\n")

FetchContent_Declare(
  "mathlib"
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git"
  GIT_TAG "0d338c5f21d1700be6ba5ad736c6f2b7bbb1d522"
  PATCH_COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=<SOURCE_DIR> -P "${PATCH_INJECT_FIND_PACKAGE_SCRIPT}"
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
    )
  ]=]
  HERMETIC_FIND_PACKAGES "iconv_lib"
)

HermeticFetchContent_MakeAvailableAtConfigureTime("mathlib")


# ─────────────────────────────────────────────────────────────────────────────
# Executable linking both
# ─────────────────────────────────────────────────────────────────────────────
add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE
  Iconv::Iconv
  MathFunctions::MathFunctions
  MathFunctionscbrt::MathFunctionscbrt
)
