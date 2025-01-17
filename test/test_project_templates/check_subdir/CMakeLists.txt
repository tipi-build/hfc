set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
cmake_minimum_required(VERSION 3.30.5)
cmake_policy(SET CMP0168 NEW)

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

FetchContent_Declare(
  project-cmake-simple
  GIT_REPOSITORY https://github.com/tipi-build/unit-test-cmake-template-2libs.git
  GIT_TAG 9652ce2756ae45f7124ba250ce9d98b7192b6102
  SOURCE_SUBDIR build/cmake
)

FetchContent_MakeHermetic(
  project-cmake-simple
  HERMETIC_BUILD_SYSTEM cmake
)

HermeticFetchContent_MakeAvailableAtConfigureTime(project-cmake-simple)


add_executable(MySimpleMain simple_main.cpp)
target_link_libraries(MySimpleMain MathFunctions::MathFunctions MathFunctionscbrt::MathFunctionscbrt)

