
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


FetchContent_Declare(
  "project-cmake-simple"
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git"
  GIT_TAG "d4593cd90243a7bdd3507a918d735353f51621b1"
)

FetchContent_MakeHermetic(
  "project-cmake-simple"
  HERMETIC_BUILD_SYSTEM cmake
  HERMETIC_TOOLCHAIN_EXTENSION [=[
    message(WARNING "This should be visible in the logs!")
  ]=]
)

HermeticFetchContent_MakeAvailableAtBuildTime("project-cmake-simple")

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE MathFunctions::MathFunctions MathFunctionscbrt::MathFunctionscbrt)
add_executable(MySimpleMain simple_main.cpp)

