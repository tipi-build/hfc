include(hfc_log)
include(hfc_targets_cache_create)
include(hfc_cmake_targets_cache)
include(hfc_targets_cache_common)
include(hfc_targets_cache_consume)
include(hfc_cmake_targets_discover)
include(hfc_cmake_register_content_build)
include(hfc_cmake_re_restore_install_tree)
include(hfc_run_project_configure)
include(hfc_generate_cmake_proxy_toolchain)
include(hfc_populate_project)

#[=======================================================================[.rst:
hfc_cmake_restore_or_configure
------------------------------------------------------------------------------------------

  ``PROJECT_SOURCE_DIR```
    The autootools source directory expected from the FetchContent_Populate call

  ``PROJECT_BINARY_DIR``
    The build dir, will only be used to run the cmake-adapter build which runs autotools configure.

  ``PROJECT_INSTALL_PREFIX``
    Where the `make install` should be done for the autootols library.
    Passed to `./configure --prefix=`

  ``ORIGIN``
    Cache ID

  ``REVISION``
    Revision to pull from cache

#]=======================================================================]
function(hfc_cmake_restore_or_configure content_name)

  set(options_params)
  set(one_value_params
    PROJECT_SOURCE_DIR
    PROJECT_SOURCE_SUBDIR
    PROJECT_BINARY_DIR
    PROJECT_INSTALL_PREFIX
    CMAKE_EXPORT_LIBRARY_DECLARATION
    CMAKE_ADDITIONAL_EXPORTS
    PROJECT_TOOLCHAIN_EXTENSION
    CREATE_TARGET_ALIASES
    BUILD_AT_CONFIGURE_TIME
    HFC_INSTALL_MARKER_FILE
    HFC_CONFIGURE_MARKER_FILE
    HERMETIC_TOOLCHAIN_EXTENSION
    MAKE_EXECUTABLES_FINDABLE
    HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING
    HERMETIC_DISCOVER_TARGETS_FILE_PATTERN

    # Cache related
    ORIGIN
    REVISION
  )

  set(multi_value_params
    HERMETIC_FIND_PACKAGES
    BUILD_TARGETS
    CUSTOM_INSTALL_TARGETS
  )

  cmake_parse_arguments(
    FN_ARG
    "${options_params}"
    "${one_value_params}"
    "${multi_value_params}"
    ${ARGN}
  )

  if(HERMETIC_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
        "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
        " ${HERMETIC_UNPARSED_ARGUMENTS}"
    )
  endif()

  hfc_get_content_proxy_toolchain_dir(${content_name} proxy_toolchain_dir)
  hfc_get_content_proxy_toolchain_path(${content_name} proxy_toolchain_path)
  file(LOCK ${proxy_toolchain_dir} DIRECTORY GUARD FUNCTION)

  hfc_generate_cmake_proxy_toolchain(${content_name}
    HERMETIC_FIND_PACKAGES "${FN_ARG_HERMETIC_FIND_PACKAGES}"
    PROJECT_TOOLCHAIN_EXTENSION "${FN_ARG_PROJECT_TOOLCHAIN_EXTENSION}"
    DESTINATION_TOOLCHAIN_PATH "${proxy_toolchain_path}"
    PROJECT_SOURCE_DIR "${FN_ARG_PROJECT_SOURCE_DIR}"
    PROJECT_SOURCE_SUBDIR "${FN_ARG_PROJECT_SOURCE_SUBDIR}"
  )

  make_directory(${FN_ARG_PROJECT_INSTALL_PREFIX})
  make_directory(${FN_ARG_PROJECT_BINARY_DIR})
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
        ${FN_ARG_PROJECT_SOURCE_DIR} ${FN_ARG_PROJECT_BINARY_DIR}
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
      hfc_log_debug(" + populating sources")
      hfc_populate_project_invoke(${content_name})
    endif()


    hfc_run_project_configure(
      ${content_name}
      PROJECT_SOURCE_DIR "${FN_ARG_PROJECT_SOURCE_DIR}"
      PROJECT_SOURCE_SUBDIR "${FN_ARG_PROJECT_SOURCE_SUBDIR}"
      PROJECT_BINARY_DIR "${FN_ARG_PROJECT_BINARY_DIR}"
      PROJECT_INSTALL_PREFIX "${FN_ARG_PROJECT_INSTALL_PREFIX}"
      TOOLCHAIN_FILE "${proxy_toolchain_path}"
      ORIGIN "${FN_ARG_ORIGIN}"
      HFC_CONFIGURE_MARKER_FILE "${FN_ARG_HFC_CONFIGURE_MARKER_FILE}"
    )

  endif()

  if (NOT "${FN_ARG_CUSTOM_INSTALL_TARGETS}" STREQUAL "")
    set(targets_search_path "${FN_ARG_PROJECT_BINARY_DIR}")
  else()
    if(dep_need_install)
      set(targets_search_path "${FN_ARG_PROJECT_BINARY_DIR}")
    else()
      set(targets_search_path "${FN_ARG_PROJECT_INSTALL_PREFIX}")
    endif()
  endif()

  set(targets_cache_created FALSE)
  get_hermetic_target_cache_file_path(${content_name} target_cache_file)

  if(FN_ARG_CMAKE_EXPORT_LIBRARY_DECLARATION)

    # Create imported targets for autotools build
    hfc_targets_cache_create_from_export_declaration(
      ${content_name}
      PROJECT_INSTALL_PREFIX "${FN_ARG_PROJECT_INSTALL_PREFIX}"
      PROJECT_BINARY_DIR "${FN_ARG_PROJECT_BINARY_DIR}"
      CMAKE_EXPORT_LIBRARY_DECLARATION "${FN_ARG_CMAKE_EXPORT_LIBRARY_DECLARATION}"
      TOOLCHAIN_FILE "${proxy_toolchain_path}"
      OUT_TARGETS_CACHE_FILE target_cache_file
    )

    set(targets_cache_created TRUE)

  else()

    hfc_log_debug(" + discovering target files in: ${targets_search_path}")

    if(FN_ARG_HERMETIC_DISCOVER_TARGETS_FILE_PATTERN)
      set(discover_find_target_files_additional_arg TARGETS_FILE_PATTERN "${FN_ARG_HERMETIC_DISCOVER_TARGETS_FILE_PATTERN}")
    else()
      set(discover_find_target_files_additional_arg "")
    endif()

    hfc_cmake_targets_discover_find_target_files(SEARCH_PATH "${targets_search_path}" ${discover_find_target_files_additional_arg} RESULT_LIST found_target_files_list)

    if(found_target_files_list)

      hfc_log_debug(" - target files found / generating targets cache in isolated context")

      hfc_cmake_targets_cache_isolated(
        TARGET_SEARCH_PATH "${targets_search_path}"
        CACHE_DESTINATION_FILE "${target_cache_file}"
        CREATE_TARGET_ALIASES "${FN_ARG_CREATE_TARGET_ALIASES}"
        TOOLCHAIN_FILE "${proxy_toolchain_path}"
        TEMP_DIR "${CMAKE_BINARY_DIR}/_deps/targets_dump_tmp"
        CMAKE_ADDITIONAL_EXPORTS "${FN_ARG_CMAKE_ADDITIONAL_EXPORTS}"
        ${discover_find_target_files_additional_arg}
      )

      set(targets_cache_created TRUE)

    endif()

  endif()


  if(targets_cache_created)

    hfc_log_debug(" - reading targets cache from ${target_cache_file}")
    hfc_targets_cache_consume(
      ${content_name}
      TARGETS_CACHE_FILE "${target_cache_file}"
      TARGET_INSTALL_PREFIX "${FN_ARG_PROJECT_INSTALL_PREFIX}"
      TARGET_SOURCE_DIR "${FN_ARG_PROJECT_SOURCE_DIR}"
      TARGET_BINARY_DIR "${FN_ARG_PROJECT_BINARY_DIR}"
      MAKE_EXECUTABLES_FINDABLE "${FN_ARG_MAKE_EXECUTABLES_FINDABLE}"
      HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING  "${FN_ARG_HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING}"
      OUT_IMPORTED_LIBRARIES imported_libraries
      OUT_LIBRARY_BYPRODUCTS library_byproducts
    )

    list(APPEND library_byproducts "${FN_ARG_HFC_INSTALL_MARKER_FILE}")

    if (dep_need_install)
      hfc_log_debug(" + registering ${content_name} build target")

      hfc_cmake_register_content_build(
        "${content_name}"
        PROJECT_SOURCE_DIR "${FN_ARG_PROJECT_SOURCE_DIR}"
        PROJECT_SOURCE_SUBDIR "${FN_ARG_PROJECT_SOURCE_SUBDIR}"
        PROJECT_BINARY_DIR "${FN_ARG_PROJECT_BINARY_DIR}"
        PROJECT_INSTALL_PREFIX "${FN_ARG_PROJECT_INSTALL_PREFIX}"
        PROJECT_DEPENDENCIES "${FN_ARG_PROJECT_DEPENDENCIES}"
        TOOLCHAIN_FILE "${proxy_toolchain_path}"
        REGISTER_BUILD_AT_CONFIGURE_TIME "${FN_ARG_BUILD_AT_CONFIGURE_TIME}"
        HFC_INSTALL_MARKER_FILE "${FN_ARG_HFC_INSTALL_MARKER_FILE}"
        ORIGIN "${FN_ARG_ORIGIN}"
        REVISION "${FN_ARG_REVISION}"
        INSTALL_BYPRODUCTS "${library_byproducts}"
        IMPORTED_TARGETS "${imported_libraries}"
        BUILD_TARGETS "${FN_ARG_BUILD_TARGETS}"
        CUSTOM_INSTALL_TARGETS "${FN_ARG_CUSTOM_INSTALL_TARGETS}"
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

    endif()

  else()

    hfc_log_debug(" - no target files found / using project with add_subdirectory()")

    # If not restored download the sources
    if(NOT ${content_name}_POPULATED)
      hfc_populate_project_invoke(${content_name})
    endif()

    #
    # rationale: in order to not polute the current directory scope with
    # HERMETIC_TOOLCHAIN_EXTENSION variables we are generating a CMakeLists.txt
    # that will contain the toolchain extension code before add_subdirectory()-ing
    # the project
    set(no_target_extension_dir "${HERMETIC_FETCHCONTENT_INSTALL_DIR}/${content_name}-toolchain/no_target_ext_add_subdirectory")
    set(no_target_extension_bin_dir "${no_target_extension_dir}/bin")
    set(no_target_extension_src_dir "${no_target_extension_dir}/src")
    set(no_target_extension_cmakelist "${no_target_extension_src_dir}/CMakeLists.txt")

    block(SCOPE_FOR VARIABLES PROPAGATE
      no_target_extension_dir
      no_target_extension_cmakelist
      content_name
      FN_ARG_PROJECT_SOURCE_DIR
      FN_ARG_PROJECT_SOURCE_SUBDIR
      FN_ARG_PROJECT_BINARY_DIR
      FN_ARG_HERMETIC_TOOLCHAIN_EXTENSION
      HERMETIC_FETCHCONTENT_ROOT_DIR
    )
      set(TEMPLATE_SOURCE_DIR ${FN_ARG_PROJECT_SOURCE_DIR})
      if(DEFINED FN_ARG_PROJECT_SOURCE_SUBDIR)
        set(TEMPLATE_SOURCE_DIR "${TEMPLATE_SOURCE_DIR}${FN_ARG_PROJECT_SOURCE_SUBDIR}")
        string(REPLACE "//" "/" TEMPLATE_SOURCE_DIR "${TEMPLATE_SOURCE_DIR}")
      endif()

      set(TEMPLATE_HERMETIC_TOOLCHAIN_EXTENSION "${FN_ARG_HERMETIC_TOOLCHAIN_EXTENSION}")
      set(TEMPLATE_CONTENT_NAME ${content_name})
      set(TEMPLATE_BINARY_DIR ${FN_ARG_PROJECT_BINARY_DIR})

      file(MAKE_DIRECTORY "${no_target_extension_src_dir}")
      configure_file("${HERMETIC_FETCHCONTENT_ROOT_DIR}/templates/no_target_ext_add_subdirectory.CMakeLists.txt.in" "${no_target_extension_cmakelist}" @ONLY)

      hfc_log_debug("Generate 'no target extension cmakelist' at ${no_target_extension_cmakelist}")

    endblock()

    add_subdirectory("${no_target_extension_src_dir}" "${no_target_extension_bin_dir}")

  endif()

endfunction()
