set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
cmake_minimum_required(VERSION 3.30.5)
cmake_policy(SET CMP0168 NEW)

project(
  get-started 
  VERSION 1.0
  LANGUAGES CXX)

# boostrap hfc
include(FetchContent)
set(hfc_REPOSITORY https://github.com/tipi-build/hfc.git)
set(hfc_REVISION main)
FetchContent_Populate(
  hermetic_fetchcontent
  GIT_REPOSITORY ${hfc_REPOSITORY}
  GIT_TAG ${hfc_REVISION}
  SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/thirdparty/cache/hfc-${hfc_REVISION}/src"
  SUBBUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/thirdparty/cache/hfc-${hfc_REVISION}/subbuild"
  BINARY_DIR "${CMAKE_CURRENT_LIST_DIR}/thirdparty/cache/hfc-${hfc_REVISION}/bin"
)
FetchContent_GetProperties(hermetic_fetchcontent)
message(STATUS "Hermetic FetchContent ${hfc_REVISION} available at '${hermetic_fetchcontent_SOURCE_DIR}'")
list(APPEND CMAKE_MODULE_PATH "${hermetic_fetchcontent_SOURCE_DIR}/cmake")
include(HermeticFetchContent)

FetchContent_Declare(
  "mathlib"
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git"
  GIT_TAG "ecc756a4c3f1811cdfd637bd6d8f4e3feb6aff92"

  SBOM_LICENSE "Apache License 2.0"
  SBOM_SUPPLIER "The tipi authors"
)

FetchContent_MakeHermetic(
  "mathlib"
  HERMETIC_BUILD_SYSTEM cmake
)

HermeticFetchContent_MakeAvailableAtBuildTime("mathlib")


add_executable(get-started get-started.cpp)
target_link_libraries(get-started PRIVATE MathFunctions::MathFunctions MathFunctionscbrt::MathFunctionscbrt)

#
# SBOM generation
#
include(hfc_sbom)
hfc_generate_sbom(
  OUTPUT "sbomfile.spdx"
  LICENSE "Apache License 2.0"
  SUPPLIER "tipi.build"
  SUPPLIER_URL "https://tipi.build"
  NO_VERIFY
)