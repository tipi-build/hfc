include_guard()

include(hfc_log)
include(hfc_targets_cache_common)

#[=======================================================================[.rst:
hfc_generate_cmake_proxy_toolchain
------------------------------------------------------------------------------------------
Generate the proxy toolchain used to transmit hermetic parent project variables
as passed by HERMETIC_TOOLCHAIN_EXTENSION or when non-hermetic all dumped variables.

  ``content_name``
  Which content is going to be fetched.

  ``PROJECT_DEPENDENCIES``
  ``PROJECT_SOURCE_DIR``
  ``PROJECT_TOOLCHAIN_EXTENSION``
  ``DESTINATION_TOOLCHAIN_PATH``
`
#]=======================================================================]
function(hfc_generate_cmake_proxy_toolchain content_name)

  set(options_params "")
  set(one_value_params
    PROJECT_TOOLCHAIN_EXTENSION
    PROJECT_SOURCE_DIR
    PROJECT_SOURCE_SUBDIR
    DESTINATION_TOOLCHAIN_PATH
  )

  set(multi_value_params 
    HERMETIC_FIND_PACKAGES
  )

  cmake_parse_arguments(FN_ARG "${options_params}" "${one_value_params}" "${multi_value_params}" ${ARGN})
  
  # replicate the behavior of cmake when it resolves the toolchain path
  # as per documentations this is:
  # "Relative paths are allowed and are interpreted first as relative to the build directory, and if not found, relative to the source directory."
  if(CMAKE_TOOLCHAIN_FILE)
    if(NOT IS_ABSOLUTE "${CMAKE_TOOLCHAIN_FILE}")
      cmake_path(APPEND CMAKE_BINARY_DIR ${CMAKE_TOOLCHAIN_FILE} OUTPUT_VARIABLE toolchain_rel_CMAKE_BINARY_DIR)
      cmake_path(APPEND CMAKE_SOURCE_DIR ${CMAKE_TOOLCHAIN_FILE} OUTPUT_VARIABLE toolchain_rel_CMAKE_SOURCE_DIR)
      if(EXISTS "${toolchain_rel_CMAKE_BINARY_DIR}")
        get_filename_component(toolchain_path_abs "${toolchain_rel_CMAKE_BINARY_DIR}" REALPATH)
      elseif(EXISTS "${toolchain_rel_CMAKE_SOURCE_DIR}")
        get_filename_component(toolchain_path_abs "${toolchain_rel_CMAKE_SOURCE_DIR}" REALPATH)
      else()
        hfc_log(FATAL_ERROR "The provided CMAKE_TOOLCHAIN_FILE path does not resolve to an existing file relative to CMAKE_BINARY_DIR (${CMAKE_BINARY_DIR} or CMAKE_SOURCE_DIR (${CMAKE_SOURCE_DIR})")
      endif()

      hfc_log_debug("Resolved relative CMAKE_TOOLCHAIN_FILE to absolute path ${toolchain_path_abs}")
    else()
      get_filename_component(toolchain_path_abs "${CMAKE_TOOLCHAIN_FILE}" REALPATH)
      hfc_log_debug("Resolved CMAKE_TOOLCHAIN_FILE to absolute path ${toolchain_path_abs}")

      if(NOT EXISTS "${toolchain_path_abs}")
        hfc_log(FATAL_ERROR "The provided CMAKE_TOOLCHAIN_FILE path (${CMAKE_TOOLCHAIN_FILE}) does not resolve to an existing file")
      endif()
    endif()
  else()

    if(NOT HERMETIC_FETCHCONTENT_IGNORE_NO_TOOLCHAIN)
      hfc_log(WARNING "No CMAKE_TOOLCHAIN_FILE defined. This is probably not what you want!")
    endif()

  endif()

  # gather targets information
  set(project_dependency_contents "include(hfc_targets_cache_common)\n")
  foreach(package IN LISTS FN_ARG_HERMETIC_FIND_PACKAGES)
  
    string(APPEND 
      project_dependency_contents 
      "hfc_targets_cache_register_dependency_for_provider(${package} "
        "TARGETS_INSTALL_PREFIX \"${HERMETIC_FETCHCONTENT_${package}_INSTALL_PREFIX}\" "
        "TARGETS_CACHE_FILE \"${HERMETIC_FETCHCONTENT_${package}_TARGETS_CACHE_FILE}\" "
      ")\n"
    )

  endforeach()

  # forward important information from the command line arguments down via the proxy toolchains
  set(proxy_toolchain_forwarded_cmake_variables_content "")

  foreach(cmake_variable IN LISTS HERMETIC_FETCHCONTENT_FORWARDED_CMAKE_VARIABLES)

    if(DEFINED ${cmake_variable})

      set(additional_set_args "")
      get_property(var_type_in_cache CACHE ${cmake_variable} PROPERTY TYPE)
      
      if(var_type_in_cache)
        set(additional_set_args "CACHE ${var_type_in_cache} \"\" FORCE")
      endif()

      string(APPEND
        proxy_toolchain_forwarded_cmake_variables_content
        "set(${cmake_variable} \"${${cmake_variable}}\" ${additional_set_args})\n"
      )

    endif()
  endforeach()

  # forward content alias information for all contents that were consumed so far
  set(hfc_contents_forwarding_code "")
  string(APPEND hfc_contents_forwarding_code "set(HERMETIC_FETCHCONTENT_CONTENTS_AVAILABLE_FROM_PARENT \"${HERMETIC_FETCHCONTENT_CONTENTS_AVAILABLE_TO_DEPENDENT_PROJECTS}\")\n")

  foreach(consumed_content_name IN LISTS HERMETIC_FETCHCONTENT_ALIASED_CONTENTS)
    __HermeticFetchContent_GetAliasesForContentVariableName("${consumed_content_name}" consumed_content_aliases)
    foreach(alias_name IN LISTS ${consumed_content_aliases})
      string(APPEND hfc_contents_forwarding_code "HermeticFetchContent_AddContentAliases(${consumed_content_name} \"${alias_name}\")\n")      
    endforeach()        
  endforeach()

  #
  set(destination_file_tmp "${FN_ARG_DESTINATION_TOOLCHAIN_PATH}.tmp")

  # generate the proxy toolchain (isolate ourselves from variable polution from parent scope)
  block(SCOPE_FOR VARIABLES 
    PROPAGATE 
      toolchain_path_abs
      content_name
      proxy_toolchain_forwarded_cmake_variables_content
      HERMETIC_FETCHCONTENT_ROOT_DIR
      HERMETIC_FETCHCONTENT_BYPASS_PROVIDER_FOR_PACKAGES
      HERMETIC_FETCHCONTENT_goldilock_BIN
      project_dependency_contents 
      destination_file_tmp 
      FN_ARG_PROJECT_TOOLCHAIN_EXTENSION 
      FN_ARG_HERMETIC_FIND_PACKAGES
      FN_ARG_PROJECT_SOURCE_DIR
      FN_ARG_PROJECT_SOURCE_SUBDIR
      HERMETIC_FETCHCONTENT_CACHED_GOLDILOCK_VERSION
      HERMETIC_FETCHCONTENT_ROOT_PROJECT_SOURCE_DIR
      HERMETIC_FETCHCONTENT_ROOT_PROJECT_BINARY_DIR
      HERMETIC_FETCHCONTENT_INSTALL_DIR
      FETCHCONTENT_BASE_DIR
      hfc_contents_forwarding_code
  )
    set(HERMETIC_FETCHCONTENT_CMAKE_TOOLCHAIN_FILE "${toolchain_path_abs}")
    set(HERMETIC_FETCHCONTENT_TOOLCHAIN_EXTENSION "${FN_ARG_PROJECT_TOOLCHAIN_EXTENSION}")
    set(HERMETIC_FETCHCONTENT_FORWARDED_CMAKE_VARIABLES_CONTENT "${proxy_toolchain_forwarded_cmake_variables_content}")

    set(HERMETIC_FETCHCONTENT_FIND_PACKAGES "${FN_ARG_HERMETIC_FIND_PACKAGES}")
    set(HERMETIC_FETCHCONTENT_PROJECT_DEPENDENCIES_CONTENTS "${project_dependency_contents}")

    cmake_path(GET HERMETIC_FETCHCONTENT_goldilock_BIN PARENT_PATH goldilock_BIN_dir)
    set(HERMETIC_FETCHCONTENT_GOLDILOCKS_INSTALL_DIR "${goldilock_BIN_dir}")

    get_hermetic_target_cache_summary_file_path(${content_name} HFC_SUMMARY_FILE)
    set(HFC_SUMMARY_CONTENT_NAME "${content_name}")
    set(HFC_DEPENDENCY_SOURCE_DIR "${FN_ARG_PROJECT_SOURCE_DIR}")

    if(FN_ARG_PROJECT_SOURCE_SUBDIR)
      set(HFC_DEPENDENCY_SOURCE_DIR "${HFC_DEPENDENCY_SOURCE_DIR}/${FN_ARG_PROJECT_SOURCE_SUBDIR}")
    endif()

    set(HFC_AVAILABLE_CONTENTS_CODE "${hfc_contents_forwarding_code}")

    set(proxy_toolchain_template_path "${HERMETIC_FETCHCONTENT_ROOT_DIR}/templates/hfc_hermetic_proxy_toolchain.cmake.in") 
    configure_file("${proxy_toolchain_template_path}" "${destination_file_tmp}" @ONLY)
  endblock()

  # now check if any existing toolchain file is the same as the newly generated one
  # this should avoid some reconfigure if the file can remain completely unchanged.
  file(COPY_FILE "${destination_file_tmp}" "${FN_ARG_DESTINATION_TOOLCHAIN_PATH}" ONLY_IF_DIFFERENT)
  file(REMOVE "${destination_file_tmp}") # clear out the temp file

  hfc_log_debug("Generated proxy toolchain at: ${FN_ARG_DESTINATION_TOOLCHAIN_PATH}")
endfunction()