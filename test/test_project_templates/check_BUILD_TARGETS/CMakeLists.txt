set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
cmake_minimum_required(VERSION 3.27.6)
project(
  check_BUILD_TARGETS
  VERSION 1.0
  LANGUAGES CXX)


set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  ${CMAKE_MODULE_PATH}
)

include(HermeticFetchContent)

FetchContent_Declare(
  components
  GIT_REPOSITORY https://github.com/tipi-build/unit-test-cmake-install-components.git
  GIT_TAG        bbe53f7fcf2824933bd2ef25dfb801b68723cd3a
)

FetchContent_MakeHermetic(
  components

  BUILD_TARGETS "some-component;another-component"

  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION  [=[
    add_executable(some-component IMPORTED )
    set_target_properties(some-component PROPERTIES
      IMPORTED_LOCATION "@HFC_PREFIX_PLACEHOLDER@/bin/some-component")
    add_executable(another-component IMPORTED )
    set_target_properties(another-component PROPERTIES
      IMPORTED_LOCATION "@HFC_PREFIX_PLACEHOLDER@/bin/another-component")
  ]=]

)

HermeticFetchContent_MakeAvailableAtConfigureTime(components)

get_target_property(some_component_LOCATION some-component IMPORTED_LOCATION)
get_target_property(another_component_LOCATION another-component IMPORTED_LOCATION)

cmake_path(GET some_component_LOCATION PARENT_PATH components_install_bin_folder)

if(NOT EXISTS "${some_component_LOCATION}")
  message(FATAL_ERROR "HFC didn't install requested components in BUILD_TARGETS: ${some_component_LOCATION}!")
endif()

if(NOT EXISTS "${another_component_LOCATION}")
  message(FATAL_ERROR "HFC didn't install requested components in BUILD_TARGETS: ${another_component_LOCATION}!")
endif()

if(EXISTS "${components_install_bin_folder}/yet-another-component")
  message(FATAL_ERROR "HFC installed all components but BUILD_TARGETS was provided !")
endif()