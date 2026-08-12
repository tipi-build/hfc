include(hfc_goldilock_helpers)
include(hfc_policy_helpers)
include(hfc_populate_cache_state)

macro(hfc_provide_dependency_FETCHCONTENT method package_name)

  set(options OVERRIDE_FIND_PACKAGE)
  set(oneValueArgs
        GIT_REPOSITORY
        GIT_TAG
        URL
        URL_HASH
        SOURCE_SUBDIR
        BINARY_DIR
  )
  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )

  set(CONTENT_SOURCE_HASH "")

  if(FN_ARG_GIT_REPOSITORY)
    set(CONTENT_SOURCE_HASH "${FN_ARG_GIT_TAG}")
  else()
    set(CONTENT_SOURCE_HASH "${FN_ARG_URL_HASH}")

    if(CONTENT_SOURCE_HASH MATCHES ".+:.+")
      string(REGEX REPLACE ".+:" "" CONTENT_SOURCE_HASH "${CONTENT_SOURCE_HASH}")
    else()
      string(SHA1 CONTENT_SOURCE_HASH "${CONTENT_SOURCE_HASH}") # it's weird so we hash it to know what we got
    endif()
  endif()

  string(SUBSTRING "${CONTENT_SOURCE_HASH}" 0 8 source_hash_short)
  set(content_source_dir "${HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR}/${package_name}-${source_hash_short}-src")
  file(MAKE_DIRECTORY ${FN_ARG_BINARY_DIR})

  hfc_goldilock_acquire("${content_source_dir}" lock_success)

  if(NOT lock_success)
    message(FATAL_ERROR "Could not acquire lock for ${content_source_dir}")
  endif()

  # filter args not supported by FetchContent_Populate:
  #  The following do not relate to populating content with FetchContent_Populate() and therefore are not supported:
  #  - EXCLUDE_FROM_ALL
  #  - SYSTEM
  #  - OVERRIDE_FIND_PACKAGE
  #  - FIND_PACKAGE_ARGS
  set(populate_args ${ARGN})

  list(REMOVE_ITEM populate_args "EXCLUDE_FROM_ALL")
  list(REMOVE_ITEM populate_args "SYSTEM")
  list(REMOVE_ITEM populate_args "OVERRIDE_FIND_PACKAGE")

  if("FIND_PACKAGE_ARGS" IN_LIST populate_args)
    # special cookie: (from the doc)
    # Everything after the FIND_PACKAGE_ARGS keyword is appended to the find_package() call, so all other <contentOptions> must come before the FIND_PACKAGE_ARGS keyword.
    #
    # This means that we discard everything after that argument
    list(FIND populate_args "FIND_PACKAGE_ARGS" FIND_PACKAGE_ARGS_ix)
    list(SUBLIST populate_args 0 ${FIND_PACKAGE_ARGS_ix} populate_args)
  endif()

  # HFC owns warm/cold detection: only populate a COLD source dir (see
  # hfc_populate_cache_state.cmake) so a warm shared cache is never
  # re-downloaded or wiped, whatever the FetchContent populate implementation
  set(hfc_provider_legacy_subbuild_dir "${HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR}/${package_name}-${source_hash_short}-subbuild")
  hfc_populate_check_cache_state(${package_name}
    SOURCE_DIR "${content_source_dir}"
    GIT_REPOSITORY "${FN_ARG_GIT_REPOSITORY}"
    GIT_TAG "${FN_ARG_GIT_TAG}"
    URL "${FN_ARG_URL}"
    LEGACY_SUBBUILD_DIR "${hfc_provider_legacy_subbuild_dir}"
    OUT_STATE hfc_provider_cache_state
  )

  if(hfc_provider_cache_state STREQUAL "UPDATE")
    hfc_populate_update_git_worktree(${package_name}
      SOURCE_DIR "${content_source_dir}"
      GIT_TAG "${FN_ARG_GIT_TAG}"
      OUT_SUCCESS hfc_provider_update_success
    )
    if(hfc_provider_update_success)
      set(hfc_provider_cache_state "WARM")
    else()
      set(hfc_provider_cache_state "CORRUPT")
    endif()
  endif()

  if(hfc_provider_cache_state STREQUAL "CORRUPT")
    hfc_log(STATUS "🧹 ${content_source_dir} is not a usable populated state; clearing for a fresh populate")
    hfc_populate_marker_remove("${content_source_dir}")
    file(REMOVE_RECURSE "${content_source_dir}")
    file(REMOVE_RECURSE "${hfc_provider_legacy_subbuild_dir}")
    string(TOLOWER "${package_name}" hfc_provider_package_lower)
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/CMakeFiles/fc-stamp/${hfc_provider_package_lower}")
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/CMakeFiles/fc-tmp/${hfc_provider_package_lower}")
    set(hfc_provider_cache_state "COLD")
  endif()

  if(hfc_provider_cache_state STREQUAL "WARM")
    hfc_log(STATUS "🟢 Reusing populated sources at ${content_source_dir}")
    hfc_populate_marker_write(${package_name} "${content_source_dir}")
  else()
    hfc_fetchcontent_populate(${package_name}
      ${populate_args}
      SOURCE_DIR ${content_source_dir}
      SUBBUILD_DIR "${hfc_provider_legacy_subbuild_dir}"
    )
    hfc_populate_marker_write(${package_name} "${content_source_dir}")
  endif()

  hfc_resolve_policy_version_minimum("" hfc_policy_version_minimum)
  hfc_push_policy_version_minimum("${hfc_policy_version_minimum}")

  if(EXISTS "${content_source_dir}/CMakeLists.txt")
    add_subdirectory("${content_source_dir}" "${FN_ARG_BINARY_DIR}")
  elseif(FN_ARG_SOURCE_SUBDIR AND EXISTS "${content_source_dir}/${FN_ARG_SOURCE_SUBDIR}/CMakeLists.txt")
    add_subdirectory("${content_source_dir}/${FN_ARG_SOURCE_SUBDIR}" "${FN_ARG_BINARY_DIR}")
  endif()

  hfc_pop_policy_version_minimum()

  if(NOT TARGET hfc_${package_name}_source_dir)
    hfc_custom_echo_command_create("hfc_${package_name}_source_dir_echo_cmd" "===SOURCE_DIR===")
    add_custom_target(hfc_${package_name}_source_dir
      COMMENT "Listing interlocked FetchContent source dirs"
      DEPENDS hfc_${package_name}_source_dir_echo_cmd
    )
    hfc_custom_echo_command_append("hfc_${package_name}_source_dir_echo_cmd" "${content_source_dir}")
  endif()

  hfc_goldilock_release("${content_source_dir}" unlock_success)

  if(NOT unlock_success)
    message(FATAL_ERROR "Could not release lock for ${content_source_dir}")
  endif()

  FetchContent_SetPopulated(${package_name}
    SOURCE_DIR ${content_source_dir}
    BINARY_DIR ${FN_ARG_BINARY_DIR}
  )

endmacro()