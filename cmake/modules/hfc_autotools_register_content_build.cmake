include_guard()

include(ProcessorCount)
include(ExternalProject)
include(hfc_log)
include(hfc_required_args)
include(hfc_generate_external_project)
include(hfc_cmake_re_command)
include(hfc_get_command_in_cleared_env)
include(hfc_custom_echo_command)

# Register an autotools project in the build graph
# 
# Usage:
# hfc_autotools_register_content_build(
#   content_name              <name>              # name of the library project
#   PROJECT_BINARY_DIR                            # where the built artifacts are expected to land
#   REGISTER_BUILD_AT_CONFIGURE_TIME  ON|OFF      # Perform build at configure time
#   HFC_INSTALL_MARKER_FILE               # In which file should we keep track of the hashed fetch content details
#   INSTALL_TARGET                                # The targets that needs to be run to perform the installation. (used to differentiate between autotools install and openssl install_sw.
#   [INSTALL_BYPRODUCTS...]     <byproduct paths>   # for linkable libraries, specify which are the produced binaries/build byproducts
# )
function(hfc_autotools_register_content_build content_name)

  # arguments parsing
  set(options "")
  set(oneValueArgs 
    ADAPTER_SOURCE_DIR
    PROJECT_SOURCE_DIR
    PROJECT_BINARY_DIR
    PROJECT_INSTALL_PREFIX
    REGISTER_BUILD_AT_CONFIGURE_TIME
    HFC_INSTALL_MARKER_FILE
    INSTALL_TARGET
    TOOLCHAIN_FILE
    ORIGIN
    REVISION
    BUILD_IN_SOURCE_TREE
    OUT_BUILD_TARGET_NAME
  )
  set(multiValueArgs
    IMPORTED_TARGETS
    INSTALL_BYPRODUCTS
  )
  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  hfc_required_args(FN_ARG ${oneValueArgs})

  if(FN_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
        "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
        " ${FN_ARG_UNPARSED_ARGUMENTS}"
    )
  endif()

  hfc_log_debug("Adding '${content_name}' as ExternalProject")

  set(build_externalproject_targets_dir "${FN_ARG_PROJECT_SOURCE_DIR}-ep-build")
  set(build_externalproject_target_name "hfc_content_build_${content_name}")



  # mkdir build/
  FILE(MAKE_DIRECTORY "${FN_ARG_PROJECT_BINARY_DIR}")

  hfc_log_debug("Build targets for ${content_name} at: ${build_externalproject_targets_dir}")

  if(NOT DEFINED MAKE_PROGRAM)
    find_program(MAKE_PROGRAM make)
  endif()
  set (AUTOTOOLS_MAKE_PROGRAM "${MAKE_PROGRAM}")

  ProcessorCount(NUM_JOBS)
  set (build_command "${MAKE_PROGRAM} -j ${NUM_JOBS}")

  if (CMAKE_RE_ENABLE)
    # CMake RE manages the install tree by version
    set(install_command "${MAKE_PROGRAM} ${FN_ARG_INSTALL_TARGET}")
  else()
    # Plain CMake: Ensure that before installing we have a clean install tree,
    # not used by any previous version
    set(install_command "${CMAKE_COMMAND} -E rm -rf ${FN_ARG_PROJECT_INSTALL_PREFIX}" )
    string(APPEND install_command " && ${MAKE_PROGRAM} ${FN_ARG_INSTALL_TARGET}")
  endif()

   #
  # build the install done marker cleaner
  set(install_command_done_marker_cleaner "")

  cmake_path(GET FN_ARG_HFC_INSTALL_MARKER_FILE PARENT_PATH installed_marker_parent_path)
  file(GLOB install_done_markers "${installed_marker_parent_path}/hfc.*.install.done")

  if(install_done_markers) 
    set(install_command_done_marker_cleaner "${CMAKE_COMMAND} -E remove ")
    foreach(found_marker_file IN LISTS install_done_markers)
      string(APPEND install_command_done_marker_cleaner " ${found_marker_file}")
    endforeach()

    string(APPEND install_command " && ${install_command_done_marker_cleaner}")
  endif()

  # this marks this build as installed
  string(APPEND install_command " && ${CMAKE_COMMAND} -E touch ${FN_ARG_HFC_INSTALL_MARKER_FILE}")
  
  if (CMAKE_RE_ENABLE)

    hfc_get_cmake_re_command_string(
      populate_cache_command
      ORIGIN "${FN_ARG_ORIGIN}"
      CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}"
      TOOLCHAIN_FILE "${FN_ARG_TOOLCHAIN_FILE}"
      GENERATOR "${CMAKE_GENERATOR}"
      INSTALL_PREFIX "${FN_ARG_PROJECT_INSTALL_PREFIX}"
      
      CACHE_OP_POPULATE REVISION "${FN_ARG_REVISION}"
    )

    set(install_command "${install_command} && ${populate_cache_command}")

  endif()
  
  if(HERMETIC_FETCHCONTENT_REMOVE_BUILD_DIR_AFTER_INSTALL)
    string(APPEND install_command " && ${CMAKE_COMMAND} -E rm -rf ${FN_ARG_PROJECT_BINARY_DIR} ")
  endif()
  
  if(HERMETIC_FETCHCONTENT_REMOVE_SOURCE_DIR_AFTER_INSTALL)
    string(APPEND install_command " && ${CMAKE_COMMAND} -E rm -rf ${FN_ARG_PROJECT_SOURCE_DIR} ")
  endif()

  #hfc_get_command_string_in_cleared_env("${build_command}" build_command)
  hfc_get_command_string_in_cleared_env("${install_command}" install_command)

  hfc_log_debug(" - install command: ${install_command}")
  hfc_log_debug(" - build byproducts: ${FN_ARG_INSTALL_BYPRODUCTS}")

  hfc_generate_external_project(${content_name}
    EP_TARGETS_DIR ${build_externalproject_targets_dir}
    TARGET_NAME ${build_externalproject_target_name} 

    # We always are in the case BUILD_IN_SOURCE_TREE, so we pass the
    # BINARY_DIR as source to build. The fetch function
    # _clone_in_build_folder_if_required takes care of putting the 
    # sources in ${FN_ARG_PROJECT_BINARY_DIR}/src. Nesting in /src is
    # necessary to have the CMakeLists.txt driving the compilation be 
    # able to put it's ninja files + CMakeCache in ${FN_ARG_PROJECT_BINARY_DIR}.
    SOURCE_DIR ${FN_ARG_PROJECT_BINARY_DIR}/src
    BINARY_DIR ${FN_ARG_PROJECT_BINARY_DIR}/src

    INSTALL_DIR ${FN_ARG_PROJECT_INSTALL_PREFIX}
    INSTALL_BYPRODUCTS "${FN_ARG_INSTALL_BYPRODUCTS}"
    BUILD_CMD ${build_command}
    INSTALL_CMD ${install_command}

    # DEPENDENCIES : we don't pass this, Autotools cannot search for cmake targets
  )

  # don't output a path if we are using the system provided $content
  if (NOT DEFINED FORCE_SYSTEM_${content_name} AND (NOT "${FORCE_SYSTEM_${content_name}}"))
    # echo binary dir info for hfc_list_dependencies_build_dirs custom target
    hfc_custom_echo_command_append(hfc_list_dependencies_build_dirs_echo_cmd "${FN_ARG_PROJECT_BINARY_DIR}")
  endif()

  hfc_log_debug("Build at configure time project written to: ${build_externalproject_targets_dir}")

  if (FN_ARG_REGISTER_BUILD_AT_CONFIGURE_TIME)
    # Build now 
    execute_process(
      COMMAND ${CMAKE_COMMAND} -G${CMAKE_GENERATOR} -DCMAKE_TOOLCHAIN_FILE=${FN_ARG_TOOLCHAIN_FILE} .
      WORKING_DIRECTORY "${build_externalproject_targets_dir}"
      COMMAND_ECHO STDOUT
      COMMAND_ERROR_IS_FATAL ANY
    ) 
    execute_process(
      COMMAND ${CMAKE_COMMAND} --build .
      WORKING_DIRECTORY "${build_externalproject_targets_dir}"
      COMMAND_ECHO STDOUT
      COMMAND_ERROR_IS_FATAL ANY
    ) 
  else()
    # Register to Build later in main project build graph 
    hfc_log_debug("Adding subdirectory ${build_externalproject_targets_dir}")
    add_subdirectory(${build_externalproject_targets_dir} ${build_externalproject_targets_dir})

    # We have to create the include directories of the imported targets ahead
    # of time even if they are empty    
    foreach(target IN LISTS FN_ARG_IMPORTED_TARGETS)
      hfc_log_debug("Creating INTERFACE_INCLUDE_DIRECTORIES for target '${target}' ahead of time")
  
      get_target_property(include_directories ${target} INTERFACE_INCLUDE_DIRECTORIES)

      if(include_directories)
        foreach(dir IN LISTS include_directories)
          file(MAKE_DIRECTORY ${dir})        
        endforeach()
      endif()

    endforeach()

  endif()

  set(${FN_ARG_OUT_BUILD_TARGET_NAME} "${build_externalproject_target_name}")
  return(PROPAGATE ${FN_ARG_OUT_BUILD_TARGET_NAME})

endfunction()
