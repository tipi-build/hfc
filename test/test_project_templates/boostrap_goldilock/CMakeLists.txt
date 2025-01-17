
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

set(HERMETIC_FETCHCONTENT_LOG_DEBUG ON)

include(HermeticFetchContent)

# check if goldilock is to be found somewhere ;)
if(NOT EXISTS "${HERMETIC_FETCHCONTENT_goldilock_BIN}")
  message(FATAL_ERROR "Could not find the goldilock binary at the expected location (${HERMETIC_FETCHCONTENT_goldilock_BIN})")
endif()

message(STATUS "Goldilock found at ${HERMETIC_FETCHCONTENT_goldilock_BIN}")

# test that it works  
__get_env_shell_command(shell)
execute_process(
  COMMAND ${shell} "-c" "${HERMETIC_FETCHCONTENT_goldilock_BIN} --version"
  RESULT_VARIABLE ret_code
  OUTPUT_VARIABLE command_stdoutput
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Goldilock version output:")
message(STATUS "${command_stdoutput}")