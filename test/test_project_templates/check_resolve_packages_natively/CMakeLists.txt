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

# ── Dependency A ─────────────────────────────────────────────────────────────
# HfcDependencyProvidedLib: an autotools library that does NOT ship cmake
# config files.  Made available at configure-time so its install tree is ready
# before mathlib is configured.
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

# ── Dependency B ─────────────────────────────────────────────────────────────
# mathlib: a cmake project whose CMakeLists.txt calls
#   find_package(HfcDependencyProvidedLib REQUIRED)
# Without HERMETIC_DEFER_NATIVE_ROOTED_FIND_PACKAGE_FOR this find_package()
# is intercepted by HFC's target-cache emulation, which may not populate all
# variables the consumer expects (e.g. <Pkg>_VERSION, <Pkg>_DIR, etc.).
#
# With HERMETIC_DEFER_NATIVE_ROOTED_FIND_PACKAGE_FOR HfcDependencyProvidedLib:
#   1. HFC sets HfcDependencyProvidedLib_ROOT to the HFC install prefix.
#   2. find_package(HfcDependencyProvidedLib BYPASS_PROVIDER) is called.
#   3. cmake finds FindHfcDependencyProvidedLib.cmake (injected into
#      CMAKE_MODULE_PATH via the toolchain extension below) and uses _ROOT
#      to locate the library — exactly as a native cmake project would.
FetchContent_Declare(
  "mathlib"
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git"
  GIT_TAG "0ad97390f914cbb2a7bbdd035e2bcbd3ec1ac213"
)

FetchContent_MakeHermetic(
  "mathlib"
  HERMETIC_BUILD_SYSTEM cmake
  HERMETIC_FIND_PACKAGES "HfcDependencyProvidedLib"
  HERMETIC_DEFER_NATIVE_ROOTED_FIND_PACKAGE_FOR "HfcDependencyProvidedLib"
  HERMETIC_TOOLCHAIN_EXTENSION [=[
    # Inject the project's Find module directory so that cmake's native
    # find_package(HfcDependencyProvidedLib) can locate FindHfcDependencyProvidedLib.cmake.
    # HERMETIC_FETCHCONTENT_ROOT_PROJECT_SOURCE_DIR is set in every HFC proxy
    # toolchain and points to the root project's source directory.
    list(PREPEND CMAKE_MODULE_PATH "${HERMETIC_FETCHCONTENT_ROOT_PROJECT_SOURCE_DIR}/find-modules")
  ]=]
)

HermeticFetchContent_MakeAvailableAtBuildTime("mathlib")

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE
  MathFunctions::MathFunctions
  MathFunctionscbrt::MathFunctionscbrt
  HfcDependencyProvidedLib::HfcDependencyProvidedLib
)
