# HermeticFetchContent / hfc_cmake_re_restore_install_tree
# 

#[=======================================================================[.rst:
hfc_run_project_configure
------------------------------------------------------------------------------------------

#]=======================================================================]
function(hfc_run_project_configure content_name)

  # arguments parsing
  set(options "")

  set(oneValueArgs_required 
     PROJECT_SOURCE_DIR
     PROJECT_BINARY_DIR
     PROJECT_INSTALL_PREFIX
     ORIGIN
     TOOLCHAIN_FILE
     HFC_CONFIGURE_MARKER_FILE
   )

  set(oneValueArgs 
    PROJECT_SOURCE_SUBDIR
    ${oneValueArgs_required}
  )
  set(multiValueArgs
    INSTALL_BYPRODUCTS
  )

  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  hfc_required_args(FN_ARG ${oneValueArgs_required})

  if(PARAM_UNPARSED_ARGUMENTS)
    hfc_log(FATAL_ERROR
        "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
        " ${PARAM_UNPARSED_ARGUMENTS}"
    )
  endif()

  if(DEFINED FN_ARG_PROJECT_SOURCE_SUBDIR)
    set(FN_ARG_PROJECT_SOURCE_DIR "${FN_ARG_PROJECT_SOURCE_DIR}/${FN_ARG_PROJECT_SOURCE_SUBDIR}")
    string(REPLACE "//" "/" FN_ARG_PROJECT_SOURCE_DIR "${FN_ARG_PROJECT_SOURCE_DIR}")
  endif()

  cmake_path(GET FN_ARG_HFC_CONFIGURE_MARKER_FILE PARENT_PATH configured_marker_parent_path)
  file(GLOB configure_markers "${configured_marker_parent_path}/hfc.*.configure.done")
  if(configure_markers) 
    hfc_log_debug(" - clearing old configure markers")
    file(REMOVE ${configure_markers})
  endif()

  set(cmake_command ${OVERRIDEN_CMAKE_COMMAND} "-G" "${CMAKE_GENERATOR}" "--install-prefix" "${FN_ARG_PROJECT_INSTALL_PREFIX}" "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}" "-S" "${FN_ARG_PROJECT_SOURCE_DIR}" "-B" "${FN_ARG_PROJECT_BINARY_DIR}" "-DCMAKE_TOOLCHAIN_FILE=${FN_ARG_TOOLCHAIN_FILE}")

  if (CMAKE_RE_ENABLE)
    # --origin
    list(APPEND cmake_command "--origin" "${FN_ARG_ORIGIN}")
    list(APPEND cmake_command "--host") # TODO: we should somehow be able to detect if this is actually a "sub-build" already
    list(APPEND cmake_command "-v")
  endif()
 
  # make sure to avoid issues that could occur when reconfiguring an existing tree
  if(EXISTS "${FN_ARG_PROJECT_BINARY_DIR}")
    file(REMOVE_RECURSE "${FN_ARG_PROJECT_BINARY_DIR}")
  endif()

  execute_process(
    COMMAND ${cmake_command} 
    WORKING_DIRECTORY ${FN_ARG_PROJECT_SOURCE_DIR}
    RESULT_VARIABLE cmd_return_code
    COMMAND_ECHO STDOUT
  ) 

  if(NOT ${cmd_return_code} EQUAL 0)
    message(FATAL_ERROR "Failed to configure ${content_name}")
  endif()

  hfc_log_debug(" - creating configure marker file at ${FN_ARG_HFC_CONFIGURE_MARKER_FILE}")
  file(TOUCH ${FN_ARG_HFC_CONFIGURE_MARKER_FILE})

endfunction()
