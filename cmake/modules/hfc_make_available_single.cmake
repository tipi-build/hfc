include(hfc_log)
include(hfc_autotools_restore_or_configure)
include(hfc_cmake_restore_or_configure)
include(hfc_saved_details)
include(hfc_determine_cache_id)
include(hfc_populate_project)

# This creates prefixes where HermeticFetchContent will reuse buildLocation or installed location
function(hfc_create_restore_prefixes content_name buildLocation installedLocation)
  if(NOT EXISTS "${buildLocation}")
    if(IS_SYMLINK "${buildLocation}")
      hfc_log_debug("Ignoring already in-place symlink at ${buildLocation}")  
    else()
      file(MAKE_DIRECTORY "${buildLocation}")
    endif()
  endif()

  if(NOT EXISTS "${installedLocation}")
    if(IS_SYMLINK "${installedLocation}")
      hfc_log_debug("Ignoring already in-place symlink at ${installedLocation}")  
    else()
      file(MAKE_DIRECTORY "${installedLocation}")
    endif()
  endif()
endfunction()

# Make a single Hermetic FetchContent content available  
# 
# Param <build_at_configure_time> selects the build stage at which the content will be installed
# setting this to ON will have the installation complete during the project configure phase
# whereas setting it to OFF will defer the descision as to the exact time of build to the
# actual build graph 
#
function(hfc_make_available_single content_name build_at_configure_time) 

  set(HERMETIC_FETCHCONTENT_MADE_CONTENT_AVAILABLE ON CACHE INTERNAL "Hermetic_FetchContent made content available") # just flag to prevent people from setting base dir etc after a first content was made available.

  hfc_saved_details_get(${content_name} __fetchcontent_arguments)

  set(options_params
    PRIVATE
    PUBLIC
  )  
  set(one_value_params
    # Official FetchContent arguments
    URL
    URL_HASH
    GIT_REPOSITORY
    GIT_TAG
    SOURCE_DIR
    SOURCE_SUBDIR
    BINARY_DIR
    BUILD_IN_SOURCE_TREE
    
    FIND_PACKAGE_ARGS

    # Custom Hermetic arguments
    HERMETIC_PREPATCHED_RESOLVER
    HERMETIC_CREATE_TARGET_ALIASES
    HERMETIC_TOOLCHAIN_EXTENSION
    HERMETIC_BUILD_SYSTEM
    MAKE_EXECUTABLES_FINDABLE
  
    HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION
    HERMETIC_CMAKE_ADDITIONAL_EXPORTS
    HERMETIC_DISCOVER_TARGETS_FILE_PATTERN
    
    HERMETIC_BUILD_AT_CONFIGURE_TIME
  )

  set(multi_value_params
    # Hermetic FetchContent arguments
    HERMETIC_FIND_PACKAGES
    BUILD_TARGETS
  )

  cmake_parse_arguments(
    __PARAMS
    "${options_params}"
    "${one_value_params}"
    "${multi_value_params}"
    ${__fetchcontent_arguments}
  )

  if(NOT __PARAMS_MAKE_EXECUTABLES_FINDABLE)
    set(__PARAMS_MAKE_EXECUTABLES_FINDABLE FALSE)
  endif()

  if(__PARAMS_PUBLIC AND __PARAMS_PRIVATE) 
    hfc_log(FATAL_ERROR "HFC taget description error for ${content_name}: properties PUBLIC and PRIVATE cannot be set simultaneously")
  endif()

  set(HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING FALSE)
  if(__PARAMS_PRIVATE)
    set(HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING TRUE)
  elseif(__PARAMS_PUBLIC)
    set(HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING FALSE)
  endif()

  set(cmake_contentInstallPath ${HERMETIC_FETCHCONTENT_INSTALL_DIR}/${content_name}-install)

  #
  # check if this content was already "HFC made available" elsewhere through the build tree
  # this emulates the behavior of FetchContent to use the content "first declared"
  #
  # In our case this comes down to using the content "made available first"
  HermeticFetchContent_ResolveContentNameAlias(${content_name} resolved_content_name) 
  if(("${resolved_content_name}" IN_LIST HERMETIC_FETCHCONTENT_CONTENTS_AVAILABLE_FROM_PARENT) OR ("${resolved_content_name}" IN_LIST HERMETIC_FETCHCONTENT_TARGETS_CACHE_CONSUMED_CONTENTS))    

    if(content_name STREQUAL resolved_content_name)
      hfc_log_debug("Making '${content_name}' available from target cache")
    else()
      hfc_log(STATUS "Making '${content_name}' available from target cache content '${resolved_content_name}' (alias)")
    endif()

    get_hermetic_target_cache_file_path(${resolved_content_name} target_cache_file)    
    set(cmake_contentInstallPath ${HERMETIC_FETCHCONTENT_INSTALL_DIR}/${resolved_content_name}-install)

    # Now load the targets in our context
    hfc_targets_cache_consume(
      ${resolved_content_name}
      MAKE_EXECUTABLES_FINDABLE "${__PARAMS_MAKE_EXECUTABLES_FINDABLE}"
      HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING  "${HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING}"
      TARGETS_CACHE_FILE "${target_cache_file}" 
      TARGET_INSTALL_PREFIX "${cmake_contentInstallPath}"
    )

    return()

  endif()


  if(NOT __PARAMS_BUILD_TARGETS)
    set(__PARAMS_BUILD_TARGETS FALSE)
  endif()

  hfc_saved_details_persist(${content_name} ${HERMETIC_FETCHCONTENT_INSTALL_DIR}/${content_name}.fetchcontent_details)
  hfc_log(STATUS "Acquiring hermetic dependency configuration lock for ${content_name}")
  set(hfc_configure_lock_file ${HERMETIC_FETCHCONTENT_INSTALL_DIR}/${content_name}-configure)
  hfc_goldilock_acquire("${hfc_configure_lock_file}" lock_success)

  if(NOT lock_success)
    hfc_log(FATAL_ERROR "Could not lock ${hfc_configure_lock_file}")
  endif()

  hfc_log(STATUS " -- Success")

  if (DEFINED FORCE_SYSTEM_${content_name})
    if (${FORCE_SYSTEM_${content_name}})
      hfc_log(WARNING "FORCE_SYSTEM_${content_name} is ON, HermeticFetchContent will only find_package the library on the system.")
      
      get_hermetic_target_cache_file_path(${content_name} target_cache_file)
      hfc_targets_cache_create_isolated(
        LOAD_TARGETS_CMAKE "[==[find_package(${content_name} REQUIRED ${__PARAMS_FIND_PACKAGE_ARGS} ) \n]==]"
        CACHE_DESTINATION_FILE "${target_cache_file}"
        TEMP_DIR "${HERMETIC_FETCHCONTENT_INSTALL_DIR}/targets_dump_tmp"
      )

      # Now load the targets in our context
      hfc_targets_cache_consume(
        ${content_name}
        MAKE_EXECUTABLES_FINDABLE "${__PARAMS_MAKE_EXECUTABLES_FINDABLE}"
        HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING  "${HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING}"
        TARGETS_CACHE_FILE "${target_cache_file}" 
        TARGET_INSTALL_PREFIX "unused-as-find-package-uses-absolute-system-path"
      )

      hfc_goldilock_release("${hfc_configure_lock_file}" lock_success)
      return()
    endif()
  endif()
  
  if (DEFINED __PARAMS_HERMETIC_BUILD_SYSTEM)
    set (__PARAMS_HERMETIC_BUILD_SYSTEM "${__PARAMS_HERMETIC_BUILD_SYSTEM}")
  else()
    set (__PARAMS_HERMETIC_BUILD_SYSTEM "cmake")
  endif()

  if (NOT DEFINED __PARAMS_HERMETIC_BUILD_AT_CONFIGURE_TIME)
    set(__PARAMS_HERMETIC_BUILD_AT_CONFIGURE_TIME ${build_at_configure_time})
  else()
    if(NOT "${build_at_configure_time}" STREQUAL "${__PARAMS_HERMETIC_BUILD_AT_CONFIGURE_TIME}")
      hfc_log(FATAL_ERROR "declaration for content '${content_name}' contains conflicting configuration for HERMETIC_BUILD_AT_CONFIGURE_TIME - cannot be made available at this stage")
    endif()
  endif()

  if(NOT DEFINED __PARAMS_HERMETIC_TOOLCHAIN_EXTENSION)
    set(__PARAMS_HERMETIC_TOOLCHAIN_EXTENSION "# (not provided)")
  endif()

  message(" - Hash of ${content_name} persisted details is ${${content_name}_DETAILS_HASH}" )
  set(hfc_install_marker_file ${cmake_contentInstallPath}/hfc.${content_name}.${${content_name}_DETAILS_HASH}.install.done)

  hfc_create_restore_prefixes(${content_name} ${__PARAMS_BINARY_DIR} ${cmake_contentInstallPath})

  # register the content/project specific populate() function
  hfc_populate_project_declare(${content_name})  

  hfc_determine_cache_id(${content_name})

  # select the project scheme based on build system name info
  if(NOT DEFINED __PARAMS_HERMETIC_BUILD_SYSTEM)
    hfc_log_debug("Defaulting to CMake build")
    set(__PARAMS_HERMETIC_BUILD_SYSTEM "cmake")
  endif()

  # echoes the source dir for that hfc content
  if(NOT TARGET hfc_${content_name}_source_dir)
    hfc_custom_echo_command_create("hfc_${content_name}_source_dir_echo_cmd" "===SOURCE_DIR===")
    add_custom_target(hfc_${content_name}_source_dir
      COMMENT "Listing interlocked FetchContent source dirs"
      DEPENDS hfc_${content_name}_source_dir_echo_cmd
    )
    hfc_custom_echo_command_append("hfc_${content_name}_source_dir_echo_cmd" "${__PARAMS_SOURCE_DIR}")
  endif()

  # echoes the install prefix for that hfc content
  add_custom_target(hfc_echo_${content_name}_install_dir
    COMMAND ${CMAKE_COMMAND} -E echo "===install_dir==="
    COMMAND ${CMAKE_COMMAND} -E echo "${cmake_contentInstallPath}"
  )

  if(__PARAMS_BUILD_IN_SOURCE_TREE)
    set(BUILD_IN_SOURCE_TREE_params BUILD_IN_SOURCE_TREE ${__PARAMS_BUILD_IN_SOURCE_TREE})
  endif()

  if(${__PARAMS_HERMETIC_BUILD_SYSTEM} STREQUAL "autotools" OR ${__PARAMS_HERMETIC_BUILD_SYSTEM} STREQUAL "openssl")

    if(${__PARAMS_HERMETIC_BUILD_SYSTEM} STREQUAL "openssl")
      set (cmake_adapter_generator generate_openssl_cmake_adapter)
    else()
      set (cmake_adapter_generator generate_autotools_cmake_adapter)
    endif()

    # built in source.
    set(hfc_configure_marker_file ${__PARAMS_BINARY_DIR}/hfc.${content_name}.${${content_name}_DETAILS_HASH}.configure.done)

    

    hfc_autotools_restore_or_configure(
      ${content_name}
      PROJECT_SOURCE_DIR ${__PARAMS_SOURCE_DIR}
      PROJECT_SOURCE_SUBDIR ${__PARAMS_SOURCE_SUBDIR}
      PROJECT_BINARY_DIR ${__PARAMS_BINARY_DIR}
      PROJECT_INSTALL_PREFIX ${cmake_contentInstallPath}
      ${BUILD_IN_SOURCE_TREE_params}

      CMAKE_EXPORT_LIBRARY_DECLARATION "${__PARAMS_HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION}"
      PROJECT_TOOLCHAIN_EXTENSION "${__PARAMS_HERMETIC_TOOLCHAIN_EXTENSION}"
      BUILD_AT_CONFIGURE_TIME "${__PARAMS_HERMETIC_BUILD_AT_CONFIGURE_TIME}"

      CMAKE_ADAPTER_GENERATOR_FN ${cmake_adapter_generator}

      FETCH_CONTENT_DETAILS_HASH ${${content_name}_DETAILS_HASH}
      HFC_INSTALL_MARKER_FILE ${hfc_install_marker_file}
      HFC_CONFIGURE_MARKER_FILE ${hfc_configure_marker_file}
      MAKE_EXECUTABLES_FINDABLE "${__PARAMS_MAKE_EXECUTABLES_FINDABLE}"
      HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING  "${HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING}"    
      ORIGIN ${${content_name}_origin}  
      REVISION ${${content_name}_revision}  
    )

  elseif (${__PARAMS_HERMETIC_BUILD_SYSTEM} STREQUAL "cmake")

    set(hfc_configure_marker_file ${__PARAMS_BINARY_DIR}/hfc.${content_name}.${${content_name}_DETAILS_HASH}.configure.done)
    
    hfc_cmake_restore_or_configure(
      ${content_name}
      PROJECT_SOURCE_DIR ${__PARAMS_SOURCE_DIR}
      PROJECT_SOURCE_SUBDIR ${__PARAMS_SOURCE_SUBDIR}
      PROJECT_BINARY_DIR ${__PARAMS_BINARY_DIR}
      PROJECT_INSTALL_PREFIX ${cmake_contentInstallPath}
      ${BUILD_IN_SOURCE_TREE_params}
      
      HERMETIC_FIND_PACKAGES "${__PARAMS_HERMETIC_FIND_PACKAGES}"

      CMAKE_EXPORT_LIBRARY_DECLARATION "${__PARAMS_HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION}"
      CMAKE_ADDITIONAL_EXPORTS "${__PARAMS_HERMETIC_CMAKE_ADDITIONAL_EXPORTS}"
      PROJECT_TOOLCHAIN_EXTENSION "${__PARAMS_HERMETIC_TOOLCHAIN_EXTENSION}"
      CREATE_TARGET_ALIASES "${__PARAMS_HERMETIC_CREATE_TARGET_ALIASES}"
      BUILD_AT_CONFIGURE_TIME "${__PARAMS_HERMETIC_BUILD_AT_CONFIGURE_TIME}"

      HFC_INSTALL_MARKER_FILE ${hfc_install_marker_file}
      HFC_CONFIGURE_MARKER_FILE ${hfc_configure_marker_file}
      MAKE_EXECUTABLES_FINDABLE "${__PARAMS_MAKE_EXECUTABLES_FINDABLE}"
      HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING  "${HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING}"
      HERMETIC_DISCOVER_TARGETS_FILE_PATTERN "${__PARAMS_HERMETIC_DISCOVER_TARGETS_FILE_PATTERN}"

      BUILD_TARGETS ${__PARAMS_BUILD_TARGETS}

      ORIGIN ${${content_name}_origin}  
      REVISION ${${content_name}_revision}
    )

  else()
  
    hfc_log(FATAL_ERROR "Hermetic FetchContent does not currently support the target build system ${__PARAMS_HERMETIC_BUILD_SYSTEM}. Please choose one of the following 'cmake' (default) 'autotools' 'openssl' in your FetchContent_MakeHermetic() declaration.")

  endif()

  hfc_goldilock_release("${hfc_configure_lock_file}" lock_success)

endfunction()
