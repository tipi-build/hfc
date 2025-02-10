#
# Using dependency built executables in the project build
#

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

# negative test: should not be found
find_program(tool dep-as-tool)

if(tool_FOUND)
  message(FATAL_ERROR "For some reason dep-as-tool is already on the PATH but it shouldn't!")
endif()


include(FetchContent)
include(HermeticFetchContent)

FetchContent_Declare(
  unit-test-dep-as-tool
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-dep-as-tool"
  GIT_TAG "e1d8537a3b972a3660f331061660b5ae9e2c2ee5"
)

# the MAKE_EXECUTABLES_FINDABLE parameter is not required to be present
# test if it defaults to the correct result (i.e. not findable)
if(DEFINED TEST_INPUT_MAKE_EXECUTABLE_FINDABLE)

  set(expecting_to_find_tool "${TEST_INPUT_MAKE_EXECUTABLE_FINDABLE}")

  FetchContent_MakeHermetic(unit-test-dep-as-tool
    MAKE_EXECUTABLES_FINDABLE "${TEST_INPUT_MAKE_EXECUTABLE_FINDABLE}"
  )

else()

  set(expecting_to_find_tool FALSE)
  FetchContent_MakeHermetic(unit-test-dep-as-tool) # see, no MAKE_EXECUTABLES_FINDABLE arg

endif()

HermeticFetchContent_MakeAvailableAtConfigureTime(unit-test-dep-as-tool)


if(expecting_to_find_tool)

  if(NOT CMAKE_PROGRAM_PATH)
    message(FATAL_ERROR "Expected CMAKE_PROGRAM_PATH to not be empty!")
  endif()

  find_program(tool dep-as-tool REQUIRED)

else()

  # negative test: still not be able to find it hopefully
  find_program(tool dep-as-tool)

  if(tool_FOUND)
    message(FATAL_ERROR "For some reason dep-as-tool is already on the PATH but it shouldn't!")
  endif()

endif()

