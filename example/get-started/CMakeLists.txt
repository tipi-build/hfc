set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
cmake_minimum_required(VERSION 3.27.6)
project(
  get-started 
  VERSION 1.0
  LANGUAGES CXX)

# Bootstrap hfc : Copy this after your project() call
include(FetchContent)
FetchContent_Populate(hfc
  GIT_REPOSITORY https://github.com/tipi-build/hfc.git
  GIT_TAG main
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/src"
  SUBBUILD_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/subbuild"
  BINARY_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/bin"
)
FetchContent_GetProperties(hfc)
list(APPEND CMAKE_MODULE_PATH "${hfc_SOURCE_DIR}/cmake")
include(HermeticFetchContent)

# Your dependencies
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