
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
  "version_update"
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-version-update.git"
  GIT_TAG "790f82e8a01b062b34133ef71dd94e9468717f37"
)

FetchContent_MakeHermetic(
  "version_update"
  HERMETIC_BUILD_SYSTEM cmake
)

HermeticFetchContent_MakeAvailableAtConfigureTime("version_update")

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE fakecachedep::fakecachedep)
add_executable(MySimpleMain simple_main.cpp)

