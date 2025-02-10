include_guard()

# Create a custom echo command to which contents can be added using hfc_custom_echo_command_append(<command_name> <content>)
#
function(hfc_custom_echo_command_create command_name header_string)
  set(cmd_root "${CMAKE_BINARY_DIR}/.hfc_custom_echo_commands")
  if(NOT EXISTS "${cmd_root}")
    file(MAKE_DIRECTORY "${cmd_root}")
  endif()

  set(echo_script_name "${cmd_root}/${command_name}.cmake")
  set(data_script_name "${cmd_root}/${command_name}.data.cmake")

  block(SCOPE_FOR VARIABLES PROPAGATE HERMETIC_FETCHCONTENT_ROOT_DIR command_name header_string data_script_name echo_script_name)
    set(TEMPLATE_custom_command_name "${command_name}")
    set(TEMPLATE_custom_command_header "${header_string}")
    set(TEMPLATE_custom_command_data "")

    configure_file("${HERMETIC_FETCHCONTENT_ROOT_DIR}/templates/hfc_custom_echo_command.cmake.in" "${echo_script_name}" @ONLY)
    configure_file("${HERMETIC_FETCHCONTENT_ROOT_DIR}/templates/hfc_custom_echo_command.data.cmake.in" "${data_script_name}" @ONLY)
  endblock()

  add_custom_command(
    OUTPUT ${command_name}
    COMMAND ${CMAKE_COMMAND} -P ${echo_script_name}
  )

endfunction()


function(hfc_custom_echo_command_append command_name entry)

  set(cmd_root "${CMAKE_BINARY_DIR}/.hfc_custom_echo_commands")
  set(data_script_name "${cmd_root}/${command_name}.data.cmake")

  if(NOT EXISTS "${data_script_name}")
    message(FATAL_ERROR "hfc_custom_echo_command '${command_name}' does not exist")
  endif()

  # Note:
  # the script at ${data_script_name} contains a single variable named "custom_command_data"
  # we load the existing contents and then append $entry, remove all dupes and save it again

  set(custom_command_data "")
  include(${data_script_name})

  list(APPEND custom_command_data "${entry}")
  list(REMOVE_DUPLICATES custom_command_data)

  set(TEMPLATE_custom_command_data "${custom_command_data}")

  block(SCOPE_FOR VARIABLES PROPAGATE HERMETIC_FETCHCONTENT_ROOT_DIR TEMPLATE_custom_command_data)
    configure_file("${HERMETIC_FETCHCONTENT_ROOT_DIR}/templates/hfc_custom_echo_command.data.cmake.in" "${data_script_name}" @ONLY)
  endblock()

endfunction()