include_guard()

include(hfc_autotools_register_content_build)
include(hfc_log)
include(hfc_targets_cache_create)
include(hfc_targets_cache_consume)
include(hfc_targets_cache_common)
include(hfc_required_args)
include(hfc_populate_project)

function(generate_autotools_cmake_adapter destination PROJECT_NAME PROJECT_SOURCE_DIR PROJECT_INSTALL_PREFIX )

  make_directory(${destination})
  configure_file("${HERMETIC_FETCHCONTENT_ROOT_DIR}/templates/CMakeLists_to_configure_autotools.txt.in" "${destination}/CMakeLists.txt" @ONLY)

endfunction()

function(generate_autotools_cmake_adapter_get_install_target OUT_INSTALL_TARGET)
  set(${OUT_INSTALL_TARGET} install)
  return(PROPAGATE ${OUT_INSTALL_TARGET})
endfunction()

function(generate_openssl_cmake_adapter destination PROJECT_NAME PROJECT_SOURCE_DIR PROJECT_INSTALL_PREFIX )

  make_directory(${destination})
  configure_file("${HERMETIC_FETCHCONTENT_ROOT_DIR}/templates/CMakeLists_to_configure_openssl.txt.in" "${destination}/CMakeLists.txt" @ONLY)

endfunction()

function(generate_openssl_cmake_adapter_get_install_target OUT_INSTALL_TARGET)
  set(${OUT_INSTALL_TARGET} install_sw)
  return(PROPAGATE ${OUT_INSTALL_TARGET})
endfunction()


function(hfc_autootols_configure 
  content_name
  # The path to the generated adapter for the configuration
  cmake_adapter_parent_path 
  # The install prefix  
  HERMETIC_PROJECT_INSTALL_PREFIX
  # The autotools in-source-tree build dir (where ./configure lies)
  AUTOTOOLS_IN_SOURCE_TREE_BUILD_DIR
  # The toolchain file impacting the build
  toolchain_file 
  # The original project adapted and getting cache entries
  origin
  # revision
  revision
  # File to consider the configure step done
  already_configured_file
  )
  if (NOT EXISTS "${already_configured_file}")
    set(cmake_command ${OVERRIDEN_CMAKE_COMMAND} "-G" "${CMAKE_GENERATOR}" "--install-prefix" "${HERMETIC_PROJECT_INSTALL_PREFIX}" "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}" "-S" "${cmake_adapter_parent_path}" "-B" "${AUTOTOOLS_IN_SOURCE_TREE_BUILD_DIR}" "-DCMAKE_TOOLCHAIN_FILE=${toolchain_file}")

    if (CMAKE_RE_ENABLE)
      # --origin
      list(APPEND cmake_command "--host") # TODO: we should somehow be able to detect if this is actually a "sub-build" already
      list(APPEND cmake_command "--origin" "${origin}")
    endif()

    cmake_path(GET already_configured_file PARENT_PATH configured_marker_parent_path)
    file(GLOB configure_markers "${configured_marker_parent_path}/hfc.*.configure.done")
    if(configure_markers) 
      hfc_log_debug(" - clearing old configure markers")
      file(REMOVE ${configure_markers})
    endif()

    execute_process(
      COMMAND ${cmake_command}
      RESULT_VARIABLE CONFIGURE_RESULT
      COMMAND_ECHO STDOUT
    ) 

    if(${CONFIGURE_RESULT} EQUAL 0)
      file(TOUCH "${already_configured_file}")
    else()
      message(FATAL_ERROR "Failed to configure ${content_name}")
    endif()
  endif()

endfunction()

# Prepares the mirror and build tree to ensure that ${AUTOTOOLS_IN_SOURCE_TREE_BUILD_DIR} can be used to  download + extract the autootools sources.
# 
# This is the reason why for autotools we download in an overriden SRC_DIR, namely inside that folder in AUTOTOOLS_IN_SOURCE_TREE_BUILD_DIR/src 
# (extraction would otherwise delete the mirror symlink)
function(hfc_autootols_prepare_mirror_build_tree_to_host_configure
  content_name
  # The path to the generated adapter for the configuration
  cmake_adapter_parent_path 
  # The install prefix  
  HERMETIC_PROJECT_INSTALL_PREFIX
  # The autotools in-source-tree build dir (where ./configure lies)
  AUTOTOOLS_IN_SOURCE_TREE_BUILD_DIR
  # The toolchain file impacting the build
  toolchain_file 
  # The original project adapted and getting cache entries
  origin)

  if (CMAKE_RE_ENABLE) 
    set(cmake_command ${OVERRIDEN_CMAKE_COMMAND} "--only-mirror" "--host" "-G" "${CMAKE_GENERATOR}" "--install-prefix" "${HERMETIC_PROJECT_INSTALL_PREFIX}" "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}" "-S" "${cmake_adapter_parent_path}" "-B" "${AUTOTOOLS_IN_SOURCE_TREE_BUILD_DIR}" "-DCMAKE_TOOLCHAIN_FILE=${toolchain_file}")

    # --origin
    list(APPEND cmake_command "--origin" "${origin}")

    execute_process(
      COMMAND ${cmake_command}
      RESULT_VARIABLE CONFIGURE_RESULT
      COMMAND_ECHO STDOUT
    ) 

    if(NOT ${CONFIGURE_RESULT} EQUAL 0)
      message(FATAL_ERROR "Failed to prepare mirror for ${content_name}")
    endif()
  endif()

endfunction()

#[=======================================================================[.rst:
hfc_restore_or_configure_autotools
------------------------------------------------------------------------------------------

  ``PROJECT_SOURCE_DIR```
    The autootools source directory expected from the FetchContent_Populate call

  ``PROJECT_BINARY_DIR``
    The build dir, will only be used to run the cmake-adapter build which runs autotools configure.

  ``PROJECT_INSTALL_PREFIX``
    Where the `make install` should be done for the autootols library. 
    Passed to `./configure --prefix=`

  ``CMAKE_EXPORT_LIBRARY_DECLARATION``
    target library & property declarations as CMake statements

  ``CMAKE_ADAPTER_GENERATOR_FN``
    Which function should be used to generate the call to configure via a CMake adapter.
      A function respecting the generate_autotools_cmake_adapter or generate_openssl_cmake_adapter calling prototype.

  ``FETCH_CONTENT_DETAILS_HASH```
    The hash of the input parametet to the FetchContent_Declare, this helps creating marker done files to determine 
    if work is required for the specific call or if it was already carried by another configure or build invocation.
  
  ``ORIGIN`` 
    Cache ID 

  ``REVISION``
    Revision to pull from cache

#]=======================================================================]
function(hfc_autotools_restore_or_configure content_name)

  set(options_params)

  set(oneValueArgs_required
    PROJECT_SOURCE_DIR
    PROJECT_BINARY_DIR
    PROJECT_INSTALL_PREFIX

    CMAKE_EXPORT_LIBRARY_DECLARATION
    PROJECT_TOOLCHAIN_EXTENSION

    FETCH_CONTENT_DETAILS_HASH
    HFC_INSTALL_MARKER_FILE
    HFC_CONFIGURE_MARKER_FILE
    BUILD_AT_CONFIGURE_TIME
    MAKE_EXECUTABLES_FINDABLE
    HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING
    BUILD_IN_SOURCE_TREE
    CMAKE_ADAPTER_GENERATOR_FN

    # Cache related
    ORIGIN
    REVISION
  )
  set(one_value_params
    PROJECT_SOURCE_SUBDIR
    ${oneValueArgs_required}
  )

  set(multi_value_params )

  cmake_parse_arguments(
    FN_ARG
    "${options_params}"
    "${one_value_params}"
    "${multi_value_params}"
    ${ARGN}
  )
  hfc_required_args(FN_ARG ${oneValueArgs_required})

  if(FN_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
        "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
        " ${FN_ARG_UNPARSED_ARGUMENTS}"
    )
  endif()

  if(DEFINED FN_ARG_PROJECT_SOURCE_SUBDIR)
    hfc_log(WARNING "The autotools support in Hermetic FetchContent does NOT currently respect the PROJECT_SOURCE_SUBDIR parameter")
  endif()

  # the adapter should be located in next to the build directory
  cmake_path(GET FN_ARG_PROJECT_BINARY_DIR PARENT_PATH binary_dir_parent_path)
  set(autotools_cmake_adapter_destination "${binary_dir_parent_path}/${content_name}-adapter")

  if (NOT EXISTS "${FN_ARG_HFC_CONFIGURE_MARKER_FILE}")
    cmake_language(CALL ${FN_ARG_CMAKE_ADAPTER_GENERATOR_FN} ${configure_command} ${autotools_cmake_adapter_destination} ${content_name} ${FN_ARG_PROJECT_BINARY_DIR} ${FN_ARG_PROJECT_INSTALL_PREFIX} )
  endif()
  cmake_language(CALL ${FN_ARG_CMAKE_ADAPTER_GENERATOR_FN}_get_install_target install_target_name)

  # Toolchain to forward arguments
  set(proxy_toolchain_path "${HERMETIC_FETCHCONTENT_INSTALL_DIR}/${content_name}-toolchain/hfc_hermetic_proxy_toolchain.cmake")
  hfc_generate_cmake_proxy_toolchain(content_name
    # PROJECT_DEPENDENCIES not passed : Autotools cannot have dependencies, it doesn't "cmake-find_package"
    PROJECT_TOOLCHAIN_EXTENSION "${FN_ARG_PROJECT_TOOLCHAIN_EXTENSION}"
    DESTINATION_TOOLCHAIN_PATH "${proxy_toolchain_path}"
    PROJECT_SOURCE_DIR "${FN_ARG_PROJECT_SOURCE_DIR}"
  )  

  make_directory(${FN_ARG_PROJECT_INSTALL_PREFIX})
  make_directory(${FN_ARG_PROJECT_SOURCE_DIR})

  set(dep_need_configure ON)
  set(dep_need_install ON)

  if(EXISTS ${FN_ARG_HFC_INSTALL_MARKER_FILE})
    set(dep_need_configure OFF)
    set(dep_need_install OFF)
  endif()

  if (dep_need_install)
    hfc_cmake_re_restore_install_tree(
      ${FN_ARG_ORIGIN} ${FN_ARG_REVISION}
      ${autotools_cmake_adapter_destination} ${FN_ARG_PROJECT_SOURCE_DIR} 
      ${FN_ARG_PROJECT_INSTALL_PREFIX} 
      "${proxy_toolchain_path}"
      cmake_re_restore_command_return_code)
    if (cmake_re_restore_command_return_code EQUAL 0)
      set(dep_need_configure OFF)
      set(dep_need_install OFF)
      hfc_log(STATUS "Successfuly restored ${content_name} from L1 Cache")
    endif()
  endif()



  if(EXISTS ${FN_ARG_HFC_CONFIGURE_MARKER_FILE})
    set(dep_need_configure OFF)
  endif()

  # we may need to start with a populate then
  if(dep_need_configure)
    if(NOT ${content_name}_POPULATED)

      # If not restored, prepare the build tree (autotools sources need to be downloaded in the CMake project build tree, as the autotools sources can only build in-source-tree.
      # The CMake project sources are in the autotools_cmake_adapter_destination
      hfc_autootols_prepare_mirror_build_tree_to_host_configure(${content_name} ${autotools_cmake_adapter_destination} ${FN_ARG_PROJECT_INSTALL_PREFIX} ${FN_ARG_PROJECT_SOURCE_DIR} ${proxy_toolchain_path} "${FN_ARG_ORIGIN}")

      if (NOT EXISTS "${FN_ARG_PROJECT_SOURCE_DIR}")
        # If we are here once again, it's because abi-hash changed for example: then the build tree is empty.
        # And as we download autotools sources in the build tree We need to redownload. 
        hfc_invalidate_project_population(${content_name} "${FN_ARG_PROJECT_SOURCE_DIR}")
      endif()

      if(IS_SYMLINK "${FN_ARG_PROJECT_SOURCE_DIR}")

        file(READ_SYMLINK  "${FN_ARG_PROJECT_SOURCE_DIR}" symlink_destination)

        if(EXISTS "${symlink_destination}") 
          file(REMOVE_RECURSE "${symlink_destination}")
        endif()

        file(MAKE_DIRECTORY "${symlink_destination}")
      endif()
      
      # then download the sources
      hfc_populate_project_invoke(${content_name})
    endif()

    # If required by build-systems that do not support in-source-tree build, 
    # Do a secondary invoke to copy sources in build tree and do the actual config there
    hfc_populate_project_invoke_clone_in_build_folder_if_required(${content_name})
  endif()

  # Create imported targets for autotools build
  hfc_targets_cache_create_from_export_declaration(
    ${content_name}
    PROJECT_INSTALL_PREFIX "${FN_ARG_PROJECT_INSTALL_PREFIX}"
    CMAKE_EXPORT_LIBRARY_DECLARATION "${FN_ARG_CMAKE_EXPORT_LIBRARY_DECLARATION}"
    TOOLCHAIN_FILE "${proxy_toolchain_path}"
    OUT_TARGETS_CACHE_FILE target_cache_file
  )

  hfc_targets_cache_consume(
    ${content_name}
    TARGETS_CACHE_FILE "${target_cache_file}" 
    TARGET_INSTALL_PREFIX "${FN_ARG_PROJECT_INSTALL_PREFIX}"
    MAKE_EXECUTABLES_FINDABLE "${FN_ARG_MAKE_EXECUTABLES_FINDABLE}"
    HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING  "${FN_ARG_HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING}"
    OUT_IMPORTED_LIBRARIES imported_libraries
    OUT_LIBRARY_BYPRODUCTS library_byproducts
    
  )

  if(dep_need_configure)
    hfc_log_debug(" - need to run configure ${FN_ARG_HFC_CONFIGURE_MARKER_FILE}")
    hfc_autootols_configure(${content_name} "${autotools_cmake_adapter_destination}" "${FN_ARG_PROJECT_INSTALL_PREFIX}" "${FN_ARG_PROJECT_BINARY_DIR}" "${proxy_toolchain_path}" "${FN_ARG_ORIGIN}" "${FN_ARG_REVISION}" "${FN_ARG_HFC_CONFIGURE_MARKER_FILE}")
  else()
    hfc_log_debug(" - NO need to run configure")
  endif()

  if (dep_need_install)
    hfc_log_debug(" - need to run install")

    # If not restored Configure and register build targets   
    hfc_autotools_register_content_build(${content_name}
      ADAPTER_SOURCE_DIR ${autotools_cmake_adapter_destination}
      PROJECT_SOURCE_DIR ${FN_ARG_PROJECT_BINARY_DIR}
      PROJECT_BINARY_DIR ${FN_ARG_PROJECT_BINARY_DIR}
      PROJECT_INSTALL_PREFIX ${FN_ARG_PROJECT_INSTALL_PREFIX}
      REGISTER_BUILD_AT_CONFIGURE_TIME ${FN_ARG_BUILD_AT_CONFIGURE_TIME} 
      HFC_INSTALL_MARKER_FILE ${FN_ARG_HFC_INSTALL_MARKER_FILE}
      INSTALL_BYPRODUCTS "${library_byproducts}"
      IMPORTED_TARGETS "${imported_libraries}"
      INSTALL_TARGET "${install_target_name}"
      TOOLCHAIN_FILE "${proxy_toolchain_path}"
      ORIGIN "${FN_ARG_ORIGIN}"
      REVISION "${FN_ARG_REVISION}"
      BUILD_IN_SOURCE_TREE "${FN_ARG_BUILD_IN_SOURCE_TREE}"
      OUT_BUILD_TARGET_NAME registered_build_target_name
    )

    # if build a configure time is ON we don't need that dependency that
    # is only there to ensure that the ${registered_build_target_name} target 
    # is run when someone consumes any of the $imported_libraries 
    if(NOT ${FN_ARG_BUILD_AT_CONFIGURE_TIME})

      foreach(lib IN LISTS imported_libraries)
        hfc_log_debug(" + creating build target dependency between '${lib}' and '${registered_build_target_name}'")
        add_dependencies(${lib} ${registered_build_target_name})
      endforeach()

    endif()

  else()
    hfc_log_debug(" - NO need to run install")
  endif()



endfunction()
