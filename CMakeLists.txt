cmake_minimum_required(VERSION 3.27.6)
set(FETCHCONTENT_QUIET OFF)

project(hermetic-fetchcontent VERSION "0.0.1")

set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/cmake"
  "${CMAKE_CURRENT_LIST_DIR}/cmake/modules"
  ${CMAKE_MODULE_PATH}
)

option(HFC_ENABLE_CMAKE_SBOM "Enable to Hermetic FetchContent CMake SBOM integration" ON)


set(HERMETIC_FETCHCONTENT_FORWARDED_CMAKE_VARIABLES "CMAKE_BUILD_TYPE;CMAKE_CXX_STANDARD;CMAKE_CXX_STANDARD_REQUIRED")
set(HERMETIC_FETCHCONTENT_BYPASS_PROVIDER_FOR_PACKAGES "Threads;dl;m")

include(HermeticFetchContent)

if(HFC_ENABLE_CMAKE_SBOM)
  include(hfc_sbom)

  hfc_generate_sbom(
    OUTPUT "hfc-sbom.spdx"
    LICENSE "Proprietary"
    SUPPLIER "tipi technologies AG"
    SUPPLIER_URL "https://tipi.build"
    NO_VERIFY
  )
endif()

enable_testing()
add_subdirectory(test)
