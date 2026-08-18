set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
cmake_minimum_required(VERSION 3.27.6)
project(
  SourceSubdirPrecedence
  VERSION 1.0
  LANGUAGES CXX)


set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  ${CMAKE_MODULE_PATH}
)

include(HermeticFetchContent)

# This commit of the test repo has BOTH a decoy root CMakeLists.txt (which
# message(FATAL_ERROR ...) if it is ever add_subdirectory()'d) AND the real,
# buildable project under build/cmake. Because SOURCE_SUBDIR is set, HFC must
# only ever look at build/cmake and never touch the root CMakeLists.txt.
FetchContent_Declare(
  project-cmake-simple
  GIT_REPOSITORY https://github.com/tipi-build/unit-test-cmake-template-2libs.git
  GIT_TAG 64d6223c191a40d87a5bb9459ea03094df40a0a1
  SOURCE_SUBDIR build/cmake
  BINARY_DIR ${CMAKE_BINARY_DIR}/project-cmake-simple-build
)

# The interlocked make-available path routes through
# hfc_provide_dependency_FETCHCONTENT(), which populates the source and
# add_subdirectory()s it honoring SOURCE_SUBDIR.
hfc_FetchContent_MakeAvailable_interlocked(project-cmake-simple)


add_executable(MySimpleMain simple_main.cpp)
target_link_libraries(MySimpleMain MathFunctions MathFunctionscbrt)
