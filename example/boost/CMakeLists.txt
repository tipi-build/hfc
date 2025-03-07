cmake_minimum_required(VERSION 3.27.6)
project(boost-example VERSION "0.0.1")

set(FETCHCONTENT_QUIET OFF)
include(FetchContent)


set(hfc_REPOSITORY https://github.com/tipi-build/hfc)
set(hfc_REVISION main)

include(HermeticFetchContent OPTIONAL RESULT_VARIABLE hfc_included) 
if(NOT hfc_included)
  FetchContent_Populate(
    hfc
    GIT_REPOSITORY https://github.com/tipi-build/hfc.git
    GIT_TAG ${hfc_REVISION}
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/src"
    SUBBUILD_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/subbuild"
    BINARY_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/bin"
  )
  FetchContent_GetProperties(hfc)
  message(STATUS "Hermetic FetchContent ${hfc_REVISION} available at '${hfc_SOURCE_DIR}'")
  list(APPEND CMAKE_MODULE_PATH "${hfc_SOURCE_DIR}/cmake")
  include(HermeticFetchContent) 
endif()

FetchContent_Declare(
  Boost
  GIT_REPOSITORY https://github.com/boostorg/boost
  GIT_TAG        "ab7968a0bbcf574a7859240d1d8443f58ed6f6cf" # 1.85
)

FetchContent_MakeHermetic(
  Boost
  HERMETIC_TOOLCHAIN_EXTENSION
  [=[
    set(BOOST_BUILD_TEST OFF CACHE BOOL "" FORCE)
    set(BOOST_ENABLE_PYTHON OFF CACHE BOOL "" FORCE)
  ]=]
)

HermeticFetchContent_MakeAvailableAtBuildTime(Boost)

add_executable(boost_example "boost_example.cpp")
target_link_libraries(boost_example Boost::system)