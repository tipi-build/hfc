
cmake_minimum_required(VERSION 3.27.6)
project(
  SBOMBootstrapTest
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  ${CMAKE_MODULE_PATH}
)

option(HFC_ENABLE_CMAKE_SBOM "Enable to Hermetic FetchContent CMake SBOM integration" ON)

set(HERMETIC_FETCHCONTENT_FORWARDED_CMAKE_VARIABLES "CMAKE_BUILD_TYPE;CMAKE_CXX_STANDARD;CMAKE_CXX_STANDARD_REQUIRED")
set(HERMETIC_FETCHCONTENT_BYPASS_PROVIDER_FOR_PACKAGES "Threads;dl;m")

include(HermeticFetchContent)

if(HFC_ENABLE_CMAKE_SBOM)
  include(hfc_sbom)

  hfc_generate_sbom(
    OUTPUT "test-sbom.spdx"
    LICENSE "MIT"
    SUPPLIER "test"
    SUPPLIER_URL "https://test.example.com"
    NO_VERIFY
  )
endif()

if(NOT DEFINED HFC_TEST_DEP_GIT_REPOSITORY)
  set(HFC_TEST_DEP_GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git")
endif()

if(NOT DEFINED HFC_TEST_DEP_GIT_TAG)
  set(HFC_TEST_DEP_GIT_TAG "d4593cd90243a7bdd3507a918d735353f51621b1")
endif()

FetchContent_Declare(
  "project-cmake-simple"
  GIT_REPOSITORY "${HFC_TEST_DEP_GIT_REPOSITORY}"
  GIT_TAG "${HFC_TEST_DEP_GIT_TAG}"
)

FetchContent_MakeHermetic(
  "project-cmake-simple"
  HERMETIC_BUILD_SYSTEM cmake
)

HermeticFetchContent_MakeAvailableAtBuildTime("project-cmake-simple")

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctions::MathFunctions MathFunctionscbrt::MathFunctionscbrt)
