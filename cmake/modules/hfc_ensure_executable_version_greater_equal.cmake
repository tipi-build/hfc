include_guard()

include(hfc_required_args)
include(hfc_log)
include(hfc_goldilock_helpers)
function(hfc_ensure_executable_version_greater_equal is_executable_and_correct_version)
  set(options "")
  set(oneValueArgs
    EXECUTABLE_PATH
    MINIMUM_VERSION
  )
  set(multiValueArgs "")
  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  hfc_required_args(FN_ARG ${oneValueArgs})

  __get_env_shell_command(shell)
  set(return_code 1)
  set(correct_version_and_is_executable FALSE)
  execute_process(
    COMMAND ${shell} "-c" "${FN_ARG_EXECUTABLE_PATH} --version"
    RESULT_VARIABLE return_code
    OUTPUT_VARIABLE command_stdoutput
    ERROR_QUIET
  )

  if(return_code EQUAL 0)
    set(VERSION_REGEX "v([0-9]+\\.[0-9]+\\.[0-9])")
    string(REGEX MATCH "${VERSION_REGEX}" _match_ver "${command_stdoutput}")
    set(current_goldilock_version ${CMAKE_MATCH_1})
    if("${current_goldilock_version}" VERSION_LESS  "${FN_ARG_MINIMUM_VERSION}")
      hfc_log(WARNING "${FN_ARG_EXECUTABLE_PATH} is at version v${current_goldilock_version} and does not meet the minimum version which is v${FN_ARG_MINIMUM_VERSION}")
    else()
      hfc_log(TRACE "- goldilock version info: ${command_stdoutput}")
      set(correct_version_and_is_executable TRUE)
    endif()
  endif()

  set(${is_executable_and_correct_version} "${correct_version_and_is_executable}" PARENT_SCOPE)
endfunction()
