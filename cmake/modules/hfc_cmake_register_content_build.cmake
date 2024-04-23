include(hfc_log)
include(hfc_targets_cache_common)
include(hfc_required_args)
include(hfc_generate_external_project)
include(hfc_cmake_re_command)
include(hfc_custom_echo_command)
include(ProcessorCount)

# Register the libary for build
# 
# Usage:
# hfc_cmake_register_content_build(
#   HERMETIC_PROJECT_NAME     <name>              # name of the library project
#   HERMETIC_PROJECT_DEFINES  <list of defines>   # compile time definitions for the build
#   BUILD_AT_CONFIGURE_TIME   <ON|OFF>            # should the build be run at configure time or on demand when the library is consumed
#   [INSTALL_BYPRODUCTS...]     <byproduct paths>   # for linkable libraries, specify which are the produced binaries/build byproducts
# )
function(hfc_cmake_register_content_build content_name)  

  # arguments parsing
  set(options "")
  set(oneValueArgs_required 
    PROJECT_SOURCE_DIR
    PROJECT_BINARY_DIR
    PROJECT_INSTALL_PREFIX
    TOOLCHAIN_FILE
    REGISTER_BUILD_AT_CONFIGURE_TIME   
    HFC_INSTALL_MARKER_FILE
    OUT_BUILD_TARGET_NAME

    # Cache related
    ORIGIN
    REVISION
  )

  set(oneValueArgs 
    ${oneValueArgs_required}
    PROJECT_DEPENDENCIES
    PROJECT_SOURCE_SUBDIR
  )

  set(multiValueArgs
    INSTALL_BYPRODUCTS
    IMPORTED_TARGETS
    BUILD_TARGETS
  )

  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  

  hfc_required_args(FN_ARG ${oneValueArgs_required})

  if(PARAM_UNPARSED_ARGUMENTS)
    hfc_log(FATAL_ERROR
        "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
        "${PARAM_UNPARSED_ARGUMENTS}"
    )
  endif()

  # generate the -install project
  hfc_log_debug("Adding '${content_name}' as ExternalProject")
  
  set(build_externalproject_targets_dir "${FN_ARG_PROJECT_BINARY_DIR}-ep")
  set(build_externalproject_targets_cmakelist "${build_externalproject_targets_dir}/CMakeLists.txt")
  set(build_externalproject_project_name "hfc_content_project_${content_name}")
  set(build_externalproject_target_name "hfc_content_build_${content_name}")

  # notes:
  # the project was already configured correctly in the calling function, in this spot we just need
  # to setup the external project to build & install (on demand or at configure time depending on preferences)
  ProcessorCount(NUM_JOBS)
  set(build_command "${CMAKE_COMMAND} --build . -j ${NUM_JOBS}")
  set(install_commands_list "") 
  
  # Ensure that before installing we have a clean install tree, not used by anyone
  if (NOT CMAKE_RE_PATH)
    list(APPEND install_commands_list "${CMAKE_COMMAND} -E rm -rf -- ${FN_ARG_PROJECT_INSTALL_PREFIX}" )
  endif()

  if(FN_ARG_BUILD_TARGETS)

    string(APPEND build_command " --target")

    foreach(trg IN LISTS FN_ARG_BUILD_TARGETS)
      string(APPEND build_command " ${trg}")
      list(APPEND install_commands_list "${CMAKE_COMMAND} --install . --component ${trg}")  # must be done as a list then concatenated because --component doesn't take multiple targets to install
    endforeach()

  else()

    list(APPEND install_commands_list "${CMAKE_COMMAND} --install .")

  endif()

  list(JOIN install_commands_list " && " install_command)
  
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

  if (CMAKE_RE_PATH)

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

  if(DEFINED FN_ARG_PROJECT_SOURCE_SUBDIR)
    set(FN_ARG_PROJECT_SOURCE_SUBDIR "${FN_ARG_PROJECT_SOURCE_DIR}/${FN_ARG_PROJECT_SOURCE_SUBDIR}")
    string(REPLACE "//" "/" FN_ARG_PROJECT_SOURCE_SUBDIR "${FN_ARG_PROJECT_SOURCE_SUBDIR}")
  endif()

  hfc_log_debug(" - CMake project name: '${build_externalproject_project_name}'")
  hfc_log_debug(" - CMake project target: '${build_externalproject_target_name}'")
  hfc_log_debug(" - build tree at: ${build_externalproject_targets_dir}")
  hfc_log_debug(" - build command: ${build_command}")
  hfc_log_debug(" - install command: ${install_command}")
  hfc_log_debug(" - build byproducts: ${FN_ARG_INSTALL_BYPRODUCTS}")  
  hfc_log_debug(" - PROJECT_SOURCE_DIR = ${FN_ARG_PROJECT_SOURCE_DIR}")
  hfc_log_debug(" - PROJECT_BINARY_DIR = ${FN_ARG_PROJECT_BINARY_DIR}")
  hfc_log_debug(" - PROJECT_INSTALL_PREFIX = ${FN_ARG_PROJECT_INSTALL_PREFIX}")
  hfc_log_debug(" - BUILD_TARGETS = ${FN_ARG_BUILD_TARGETS}")

  hfc_generate_external_project(${content_name}
    EP_TARGETS_DIR ${build_externalproject_targets_dir}
    TARGET_NAME ${build_externalproject_target_name} 
    SOURCE_DIR ${FN_ARG_PROJECT_SOURCE_DIR}

    BINARY_DIR ${FN_ARG_PROJECT_BINARY_DIR}

    INSTALL_DIR ${FN_ARG_PROJECT_INSTALL_PREFIX}
    INSTALL_BYPRODUCTS ${FN_ARG_INSTALL_BYPRODUCTS}
    BUILD_CMD ${build_command}
    INSTALL_CMD ${install_command}

    DEPENDENCIES ${FN_ARG_PROJECT_DEPENDENCIES}
  )

  if (NOT DEFINED FORCE_SYSTEM_${content_name} AND (NOT "${FORCE_SYSTEM_${content_name}}"))
    # echo binary dir info for hfc_list_dependencies_build_dirs custom target
    hfc_custom_echo_command_append(hfc_list_dependencies_build_dirs_echo_cmd "${FN_ARG_PROJECT_BINARY_DIR}")
  endif()

  if(${FN_ARG_BUILD_AT_CONFIGURE_TIME})

    hfc_log_debug(" - Build at configure time / configuring ${content_name}")

    # bascially cd build && cmake ..
    execute_process(
      COMMAND ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR} -DCMAKE_TOOLCHAIN_FILE=${FN_ARG_TOOLCHAIN_FILE} .
      WORKING_DIRECTORY "${build_externalproject_targets_dir}"
      COMMAND_ECHO STDOUT
      COMMAND_ERROR_IS_FATAL ANY
    ) 
    
    hfc_log_debug(" - Build at configure time / building ${content_name}")
    
    # cmake --build
    execute_process(
      COMMAND ${CMAKE_COMMAND} --build .
      WORKING_DIRECTORY "${build_externalproject_targets_dir}"
      COMMAND_ECHO STDOUT
      COMMAND_ERROR_IS_FATAL ANY
    )

    hfc_log_debug(" - Build at configure time / ${content_name} [done]")

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
