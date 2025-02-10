include_guard()

# Wraps a given execute_process COMMAND list in a valid system shell string which clears
# the environments variables before execution
#
# Usage : hfc_get_command_in_cleared_env("${configure_command_list}" configure_command)
#
# ``command_list``
#   The list of argument for execute_process (important to quote it so that it doesn't unwrap)
#  ``command_out_name```
#   Variable name containing the wrapped command  as a wrapped strings to pass to a shell
function(hfc_get_command_in_cleared_env command_list command_out_name)

  set(final_command_string)
  foreach(item IN LISTS command_list)
    string(APPEND final_command_string " \"${item}\"")
  endforeach()

  hfc_get_command_string_in_cleared_env(${final_command_string}  ${command_out_name})
  return(PROPAGATE ${command_out_name})
endfunction()


function(hfc_get_command_string_in_cleared_env command_string command_out_name)
  set(clear_vars_shell_script "${HERMETIC_FETCHCONTENT_ROOT_DIR}/scripts/clear-all.sh")

  set(shell_env
      .
      ${clear_vars_shell_script}
      &&
  )

  string(REPLACE ";" " " final_command_string_with_shell_env "${shell_env} ${command_string}")
  set(${command_out_name} "${final_command_string_with_shell_env}")

  return(PROPAGATE ${command_out_name})
endfunction()