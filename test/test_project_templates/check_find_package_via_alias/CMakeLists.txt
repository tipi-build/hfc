set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
cmake_minimum_required(VERSION 3.27.6)
project(
  FindPackageViaAliasTest
  VERSION 1.0
  LANGUAGES CXX)


set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  ${CMAKE_MODULE_PATH}
)

include(HermeticFetchContent)

# Declare the autotools iconv library under a DIFFERENT canonical name
# so that we can test alias resolution via find_package() in sub-builds.
FetchContent_Declare(
  iconv_lib
  GIT_REPOSITORY https://github.com/tipi-build/unittest-autotools-sample.git
  GIT_TAG ad80b024eeda8f4c0a96eedf669dc453ed33a094
)

FetchContent_MakeHermetic(
  iconv_lib
  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION
     [=[
       add_library(HfcDependencyProvidedLib::HfcDependencyProvidedLib STATIC IMPORTED)
       set_property(TARGET HfcDependencyProvidedLib::HfcDependencyProvidedLib PROPERTY IMPORTED_LOCATION "@HFC_PREFIX_PLACEHOLDER@/lib/libiconv.a")
       set_property(TARGET HfcDependencyProvidedLib::HfcDependencyProvidedLib PROPERTY INTERFACE_INCLUDE_DIRECTORIES @HFC_PREFIX_PLACEHOLDER@/include)
     ]=]

  HERMETIC_BUILD_SYSTEM autotools
)

HermeticFetchContent_MakeAvailableAtConfigureTime(iconv_lib)

# Register "HfcDependencyProvidedLib" as an ALIAS for "iconv_lib".
# This is the pattern used in the Arrow example where jemallocAlt -> jemalloc.
HermeticFetchContent_AddContentAliases(iconv_lib "HfcDependencyProvidedLib")

# mathlib's CMakeLists.txt does find_package(HfcDependencyProvidedLib REQUIRED)
# HFC's provider must resolve "HfcDependencyProvidedLib" via alias to "iconv_lib"
# and serve iconv_lib's target cache.
FetchContent_Declare(
  "mathlib"
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git"
  GIT_TAG "00209d38cc22eaaa62e7ba6d96feae80f9d3a250"
)

# When TEST_DATA_DEFER_VIA_ALIAS=ON, request native-rooted find_package() via the
# ALIAS name only (HfcDependencyProvidedLib), while HERMETIC_FIND_PACKAGES lists the
# canonical name FIRST. This exercises the interaction between alias resolution,
# deduplication and HERMETIC_DEFER_NATIVE_ROOTED_FIND_PACKAGE_FOR: the defer flag must
# survive even though the canonical name (which carries no flag) is registered first.
set(mathlib_defer_packages "")
if(TEST_DATA_DEFER_VIA_ALIAS STREQUAL "ON")
  set(mathlib_defer_packages "HfcDependencyProvidedLib")
  # native find_package(iconv_lib) needs a Find module rooted at iconv_lib_ROOT
  list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/custom_find_modules")
endif()

FetchContent_MakeHermetic(
  "mathlib"
  HERMETIC_BUILD_SYSTEM cmake
  # List both canonical AND alias name (like Arrow's "jemalloc;jemallocAlt")
  # to exercise deduplication — both resolve to iconv_lib, only one registration expected.
  HERMETIC_FIND_PACKAGES "iconv_lib;HfcDependencyProvidedLib"
  HERMETIC_DEFER_NATIVE_ROOTED_FIND_PACKAGE_FOR "${mathlib_defer_packages}"
  HERMETIC_TOOLCHAIN_EXTENSION "list(PREPEND CMAKE_MODULE_PATH \"${CMAKE_CURRENT_SOURCE_DIR}/custom_find_modules\")\n"
  HERMETIC_HfcDependencyProvidedLib_FIND_PACKAGE_EXTRA_CODE [=[
    set(HfcDependencyProvidedLib_VERSION "1.2.3.alias_test")
    # The variable is keyed on the content name passed to find_package()
    # (HfcDependencyProvidedLib), even though it resolves to the canonical
    # name iconv_lib. The value must be iconv_lib's HFC install prefix.
    message(STATUS "[hfc_test] HfcDependencyProvidedLib_HERMETIC_INSTALL_PREFIX=${HfcDependencyProvidedLib_HERMETIC_INSTALL_PREFIX}")
  ]=]
)

HermeticFetchContent_MakeAvailableAtBuildTime("mathlib")


add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctions::MathFunctions MathFunctionscbrt::MathFunctionscbrt HfcDependencyProvidedLib::HfcDependencyProvidedLib)
