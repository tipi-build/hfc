include(hfc_log)
include(hfc_required_args)
include(hfc_git_helpers)
include(hfc_goldilock_helpers)
include(hfc_policy_helpers)
include(hfc_populate_cache_state)
include(FetchContent)
include(hfc_compute_subbuild_path)
include(hfc_compute_populate_build_path)

function(hfc_populate_project__get_function_name content_name OUT_function_name)
  set(${OUT_function_name} "hfc_populate_project_${content_name}" PARENT_SCOPE)
endfunction()


function(hfc_evaluate_prepatched_resolver)

  set(options_params)
  set(one_value_params
    # Official FetchContent arguments
    URL
    URL_HASH
    GIT_REPOSITORY
    GIT_TAG
    GIT_SUBMODULES
    SOURCE_DIR

    # How to prefix variables
    OUT_VAR_PREFIX

    # Custom Hermetic arguments
    HERMETIC_PREPATCHED_RESOLVER
  )

  set(multi_value_params)
  cmake_parse_arguments(FN_ARG "${options_params}" "${one_value_params}" "${multi_value_params}" ${ARGN})
  hfc_required_args(FN_ARG OUT_VAR_PREFIX HERMETIC_PREPATCHED_RESOLVER)

  # make just these available in the eval'ed code
  set(URL "${FN_ARG_URL}")
  set(URL_HASH "${FN_ARG_URL_HASH}")
  set(GIT_REPOSITORY "${FN_ARG_GIT_REPOSITORY}")
  set(GIT_TAG "${FN_ARG_GIT_TAG}")
  set(SOURCE_DIR "${FN_ARG_SOURCE_DIR}")

  set(RESOLVED_PATCH FALSE)
  block(SCOPE_FOR VARIABLES PROPAGATE content_name SOURCE_DIR URL URL_HASH GIT_REPOSITORY GIT_TAG RESOLVED_PATCH)
    hfc_log_debug("Evaluating prepatched resolve : '${FN_ARG_HERMETIC_PREPATCHED_RESOLVER}'")
    cmake_language(EVAL CODE ${FN_ARG_HERMETIC_PREPATCHED_RESOLVER})
  endblock()

  set(${FN_ARG_OUT_VAR_PREFIX}RESOLVED_PATCH ${RESOLVED_PATCH})

  if(${FN_ARG_OUT_VAR_PREFIX}RESOLVED_PATCH)

   if(URL)
      hfc_log_debug(" + Resolved pre-patched source: ${URL} / ${URL_HASH}")
    endif()

    if(GIT_REPOSITORY)
      hfc_log_debug(" + Resolved pre-patched source: ${GIT_REPOSITORY} / ${GIT_TAG}")
    endif()

    # mirror variable changes back to FN_ARG_*
    set(${FN_ARG_OUT_VAR_PREFIX}URL "${URL}")
    set(${FN_ARG_OUT_VAR_PREFIX}URL_HASH "${URL_HASH}")
    set(${FN_ARG_OUT_VAR_PREFIX}GIT_REPOSITORY "${GIT_REPOSITORY}")
    set(${FN_ARG_OUT_VAR_PREFIX}GIT_TAG "${GIT_TAG}")


    return(PROPAGATE
      ${FN_ARG_OUT_VAR_PREFIX}RESOLVED_PATCH

      ${FN_ARG_OUT_VAR_PREFIX}URL
      ${FN_ARG_OUT_VAR_PREFIX}URL_HASH
      ${FN_ARG_OUT_VAR_PREFIX}GIT_REPOSITORY
      ${FN_ARG_OUT_VAR_PREFIX}GIT_TAG
    )

  endif()

  return(PROPAGATE
    ${FN_ARG_OUT_VAR_PREFIX}RESOLVED_PATCH
  )
endfunction()

function(hfc_populate_project_declare content_name)

  hfc_log_debug("Registering project populate function for ${content_name}")
  hfc_populate_project__get_function_name(content_name populate_project_function_name)

  # define that project specific function
  function(${populate_project_function_name})

    set(options_params)
    set(one_value_params
      # Official FetchContent arguments
      URL
      URL_HASH
      GIT_REPOSITORY
      GIT_TAG
      GIT_SUBMODULES
      GIT_SHALLOW
      SOURCE_DIR
      BUILD_IN_SOURCE_TREE
      SOURCE_SUBDIR
      BINARY_DIR
      FIND_PACKAGE_ARGS

      # Custom Hermetic arguments
      HERMETIC_PREPATCHED_RESOLVER
      HERMETIC_CREATE_TARGET_ALIASES
      HERMETIC_TOOLCHAIN_EXTENSION
      HERMETIC_BUILD_SYSTEM
      HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION
      HERMETIC_BUILD_AT_CONFIGURE_TIME
    )

    set(multi_value_params
      # Hermetic FetchContent arguments
      HERMETIC_FIND_PACKAGES
      PATCH_COMMAND
    )
    cmake_parse_arguments(FN_ARG "${options_params}" "${one_value_params}" "${multi_value_params}" ${ARGN})

    set(prepatched_RESOLVED_PATCH FALSE)

    hfc_log_debug("Running populate function for ${content_name}")

    if(FN_ARG_HERMETIC_PREPATCHED_RESOLVER)
      if(FN_ARG_URL)
        hfc_log_debug(" - URL = ${FN_ARG_URL}")
        hfc_log_debug(" - URL_HASH = ${FN_ARG_URL_HASH}")
      elseif(FN_ARG_GIT_REPOSITORY)
        hfc_log_debug(" - GIT_REPOSITORY = ${FN_ARG_GIT_REPOSITORY}")
        hfc_log_debug(" - GIT_TAG = ${FN_ARG_GIT_TAG}")
      endif()

      hfc_evaluate_prepatched_resolver(
        OUT_VAR_PREFIX prepatched_
        HERMETIC_PREPATCHED_RESOLVER "${FN_ARG_HERMETIC_PREPATCHED_RESOLVER}"

        URL "${FN_ARG_URL}"
        URL_HASH "${FN_ARG_URL_HASH}"
        GIT_REPOSITORY "${FN_ARG_GIT_REPOSITORY}"
        GIT_TAG "${FN_ARG_GIT_TAG}"
        GIT_SUBMODULES "${FN_ARG_GIT_SUBMODULES}"
        SOURCE_DIR "${FN_ARG_SOURCE_DIR}"
      )

      if(prepatched_RESOLVED_PATCH)
        # mirror variable changes back to FN_ARG_*
        set(FN_ARG_URL "${prepatched_URL}")
        set(FN_ARG_URL_HASH "${prepatched_URL_HASH}")
        set(FN_ARG_GIT_REPOSITORY "${prepatched_GIT_REPOSITORY}")
        set(FN_ARG_GIT_TAG "${prepatched_GIT_TAG}")
      endif()

    endif()

    # acquire source dir lock
    set(lock_dir "${FN_ARG_SOURCE_DIR}")

    hfc_goldilock_acquire("${lock_dir}" lock_success)

    if(NOT lock_success)
      hfc_log(FATAL_ERROR "Could not acquire lock for ${lock_dir}")
    endif()

    # HFC owns warm/cold detection: FetchContent_Populate() only ever runs
    # against a COLD source dir (populating into an empty dir is safe under
    # both populate implementations; re-populating a warm shared cache is what
    # nukes/re-downloads it under CMP0168 direct population).
    hfc_compute_subbuild_path(${content_name} legacy_subbuild_path
      SOURCE_DIR "${FN_ARG_SOURCE_DIR}"
    )
    hfc_populate_check_cache_state(${content_name}
      SOURCE_DIR "${FN_ARG_SOURCE_DIR}"
      GIT_REPOSITORY "${FN_ARG_GIT_REPOSITORY}"
      GIT_TAG "${FN_ARG_GIT_TAG}"
      URL "${FN_ARG_URL}"
      LEGACY_SUBBUILD_DIR "${legacy_subbuild_path}"
      OUT_STATE cache_state
    )

    if(cache_state STREQUAL "UPDATE")
      # valid clone at another revision: fetch + force-clean checkout in place,
      # never a re-clone (an unreachable origin only matters if the revision is
      # missing locally)
      hfc_populate_update_git_worktree(${content_name}
        SOURCE_DIR "${FN_ARG_SOURCE_DIR}"
        GIT_TAG "${FN_ARG_GIT_TAG}"
        OUT_SUCCESS update_success
      )
      if(update_success)
        set(cache_state "WARM")
      else()
        hfc_log(STATUS "🧹 Could not update ${FN_ARG_SOURCE_DIR} in place; re-populating from scratch")
        set(cache_state "CORRUPT")
      endif()
    endif()

    if(cache_state STREQUAL "CORRUPT")
      # non-empty dir that is neither a valid clone nor a completed populate
      # (e.g. .git removed, half-extracted archive): clear every layout and
      # re-populate. Deliberate dev checkouts stay valid git repos and never
      # land here.
      hfc_log(STATUS "🧹 ${FN_ARG_SOURCE_DIR} is not a usable populated state; clearing for a fresh populate")
      hfc_invalidate_project_population(${content_name} "${FN_ARG_SOURCE_DIR}")
      set(cache_state "COLD")
    endif()

    if(cache_state STREQUAL "WARM")
      if(FN_ARG_GIT_REPOSITORY)
        repo_is_clean(REPOSITORY_DIR "${FN_ARG_SOURCE_DIR}"
          CHECK_IGNORED
          OUT_RESULT warm_repo_is_clean
        )
        if(warm_repo_is_clean)
          hfc_log(STATUS "🟢 Repository ${FN_ARG_SOURCE_DIR} at ${FN_ARG_GIT_TAG} and clean")
        else()
          hfc_log(STATUS "🟢 Repository ${FN_ARG_SOURCE_DIR} at ${FN_ARG_GIT_TAG} with local modifications")
          if(NOT prepatched_RESOLVED_PATCH AND NOT FN_ARG_PATCH_COMMAND)
            hfc_log(WARNING "🔴 Repository ${FN_ARG_SOURCE_DIR} has local modifications; reusing them. Are they intentional ?")
          endif()
        endif()
      else()
        hfc_log(STATUS "🟢 Reusing populated sources at ${FN_ARG_SOURCE_DIR}")
      endif()

      hfc_populate_marker_write(${content_name} "${FN_ARG_SOURCE_DIR}")

      # note: this function might be invoked from a scripted context
      # so we might not be able to FetchContent_SetPopulated() because it internally
      # set_property(GLOBAL) which is not scriptable
      if(NOT CMAKE_SCRIPT_MODE_FILE)

        FetchContent_SetPopulated(${content_name}
          SOURCE_DIR "${FN_ARG_SOURCE_DIR}"
          BINARY_DIR "${FN_ARG_BINARY_DIR}"
        )

      endif()

      hfc_goldilock_release("${lock_dir}" success)

      # \ö/
      return()
    endif()

    #
    # build arguments for FetchContent_populate()
    set(populate_args "")

    if(FN_ARG_URL)
      hfc_log_debug(" - URL = ${FN_ARG_URL}")
      hfc_log_debug(" - URL_HASH = ${FN_ARG_URL_HASH}")
      list(APPEND populate_args "URL" ${FN_ARG_URL} "URL_HASH" ${FN_ARG_URL_HASH})
    elseif(FN_ARG_GIT_REPOSITORY)
      hfc_log_debug(" - GIT_REPOSITORY = ${FN_ARG_GIT_REPOSITORY}")
      hfc_log_debug(" - GIT_TAG = ${FN_ARG_GIT_TAG}")
      list(APPEND populate_args "GIT_REPOSITORY" ${FN_ARG_GIT_REPOSITORY} "GIT_TAG" ${FN_ARG_GIT_TAG})
    else()
      hfc_log(FATAL_ERROR "Hermetic FetchContent currently supports only URL or GIT_REPOSITORY download schemes")
    endif()

    if (FN_ARG_GIT_SUBMODULES)
      list(APPEND populate_args "GIT_SUBMODULES" ${FN_ARG_GIT_SUBMODULES})
    endif()

    if (NOT "${FN_ARG_BUILD_IN_SOURCE_TREE}" STREQUAL "")

      if(FN_ARG_URL)
        list(APPEND populate_args "DOWNLOAD_NO_EXTRACT" TRUE)
      endif()
      list(APPEND populate_args "SOURCE_DIR" ${FN_ARG_SOURCE_DIR})
    else()
      list(APPEND populate_args "SOURCE_DIR" ${FN_ARG_SOURCE_DIR})
    endif()

    hfc_compute_subbuild_path(${content_name} subbuild_path
      SOURCE_DIR "${FN_ARG_SOURCE_DIR}"
    )

    hfc_compute_populate_build_path(${content_name} populate_build_path
      SOURCE_DIR "${FN_ARG_SOURCE_DIR}"
    )

    list(APPEND populate_args "SUBBUILD_DIR" ${subbuild_path})
    list(APPEND populate_args "BINARY_DIR" ${populate_build_path})

    if(prepatched_RESOLVED_PATCH)
      list(APPEND populate_args "PATCH_COMMAND" "") # don't try to patch already patched things
    elseif(FN_ARG_PATCH_COMMAND)
      list(APPEND populate_args "PATCH_COMMAND" "${FN_ARG_PATCH_COMMAND}")
      list(APPEND populate_args "UPDATE_DISCONNECTED" "1")  # avoid issues with repeated builds, which would "repatch"
    endif()

    if(FN_ARG_GIT_SHALLOW)
      list(APPEND populate_args "GIT_SHALLOW" ${FN_ARG_GIT_SHALLOW})
    endif()

    # cold populate into the (empty) cache dir; the completed populate is
    # recorded by the marker, which is what makes later configures and other
    # build trees reuse the cache without re-downloading
    hfc_log_debug(" - populating (${populate_args})")
    hfc_fetchcontent_populate(
      ${content_name}
      ${populate_args}
    )

    hfc_populate_marker_write(${content_name} "${FN_ARG_SOURCE_DIR}")

    FetchContent_SetPopulated(${content_name}
      SOURCE_DIR "${FN_ARG_SOURCE_DIR}"
      BINARY_DIR "${FN_ARG_BINARY_DIR}"
    )

    hfc_goldilock_release("${lock_dir}" success)

    # Remove extraneous folder after population
    file(REMOVE_RECURSE ${populate_build_path})
  endfunction()


  function(${populate_project_function_name}_clone_in_build_folder_if_required)

    set(options_params)
    set(one_value_params
      # Official FetchContent arguments
      URL
      URL_HASH
      GIT_REPOSITORY
      GIT_TAG
      GIT_SUBMODULES
      SOURCE_DIR
      BUILD_IN_SOURCE_TREE
      SOURCE_SUBDIR
      BINARY_DIR
      FIND_PACKAGE_ARGS

      # Custom Hermetic arguments
      HERMETIC_PREPATCHED_RESOLVER
      HERMETIC_CREATE_TARGET_ALIASES
      HERMETIC_TOOLCHAIN_EXTENSION
      HERMETIC_BUILD_SYSTEM
      HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION
      HERMETIC_BUILD_AT_CONFIGURE_TIME
    )

    set(multi_value_params
      # Hermetic FetchContent arguments
      HERMETIC_FIND_PACKAGES
      PATCH_COMMAND
    )
    cmake_parse_arguments(FN_ARG "${options_params}" "${one_value_params}" "${multi_value_params}" ${ARGN})

    if (NOT "${FN_ARG_BUILD_IN_SOURCE_TREE}" STREQUAL "")

      # Ensure the binary dir exists before populating into it — needed when
      # cmake-re creates a fresh worktree (e.g. after a toolchain fingerprint change).
      # It may already be a symlink (created by cmake-re) pointing to the worktree.
      if(IS_SYMLINK "${FN_ARG_BINARY_DIR}")
        file(READ_SYMLINK "${FN_ARG_BINARY_DIR}" _hfc_binary_dir_target)
        file(MAKE_DIRECTORY "${_hfc_binary_dir_target}")
      elseif(NOT EXISTS "${FN_ARG_BINARY_DIR}")
        file(MAKE_DIRECTORY "${FN_ARG_BINARY_DIR}")
      endif()

      set(lock_dir "${FN_ARG_SOURCE_DIR}")
      hfc_goldilock_acquire("${lock_dir}" lock_success)
      set(content_name_clone_in_build_folder ${content_name}_clone_to_build_folder)
      set(clone_source_in_build_populate_args "")

      if(FN_ARG_URL)
        hfc_log_debug(" - URL = ${FN_ARG_URL}")
        hfc_log_debug(" - URL_HASH = ${FN_ARG_URL_HASH}")
        cmake_path(GET FN_ARG_URL FILENAME DOWNLOAD_NO_EXTRACT_archive_name)
        list(APPEND clone_source_in_build_populate_args "URL" ${FN_ARG_SOURCE_DIR}/${DOWNLOAD_NO_EXTRACT_archive_name} "URL_HASH" ${FN_ARG_URL_HASH})
      elseif(FN_ARG_GIT_REPOSITORY)
        hfc_log_debug(" - GIT_REPOSITORY = ${FN_ARG_GIT_REPOSITORY}")
        hfc_log_debug(" - GIT_TAG = ${FN_ARG_GIT_TAG}")
        list(APPEND clone_source_in_build_populate_args "GIT_REPOSITORY" ${FN_ARG_SOURCE_DIR} "GIT_TAG" ${FN_ARG_GIT_TAG})
      else()
        hfc_log(FATAL_ERROR "Hermetic FetchContent currently supports only URL or GIT_REPOSITORY download schemes")
      endif()

      # We will FetchContent locally from the fetched source dir by the first ${populate_project_function_name} into the BINARY_DIR
      # This is useful for build systems that do not support out-of-source-tree builds.
      set(SOURCE_DIR_IN_BINARY_DIR "${FN_ARG_BINARY_DIR}/src")
      list(APPEND clone_source_in_build_populate_args "SOURCE_DIR" "${SOURCE_DIR_IN_BINARY_DIR}")

      hfc_compute_subbuild_path(${content_name_clone_in_build_folder} subbuild_path
        SOURCE_DIR "${SOURCE_DIR_IN_BINARY_DIR}"
      )

      hfc_compute_populate_build_path(${content_name_clone_in_build_folder} populate_build_path
        SOURCE_DIR "${SOURCE_DIR_IN_BINARY_DIR}"
      )

      list(APPEND clone_source_in_build_populate_args "SUBBUILD_DIR" ${subbuild_path})
      list(APPEND clone_source_in_build_populate_args "BINARY_DIR" ${populate_build_path})

      # git in-source trees are self-validating (a valid clone with HEAD at the
      # declared tag IS the correct state) and reuse the warm/update state
      # machine. URL in-source trees are not: the tree is per-build-tree and is
      # rearranged behind HFC's back in cmake-re mode (mirror worktree
      # re-pointing, source snapshot restore), so no marker can be trusted —
      # always re-extract from the locally cached archive (cheap, offline).
      if(FN_ARG_URL)
        set(in_build_cache_state "CORRUPT")
      else()
        hfc_populate_check_cache_state(${content_name_clone_in_build_folder}
          SOURCE_DIR "${SOURCE_DIR_IN_BINARY_DIR}"
          GIT_REPOSITORY "${FN_ARG_SOURCE_DIR}"
          GIT_TAG "${FN_ARG_GIT_TAG}"
          URL "${FN_ARG_URL}"
          LEGACY_SUBBUILD_DIR "${subbuild_path}"
          OUT_STATE in_build_cache_state
        )
      endif()

      if(in_build_cache_state STREQUAL "UPDATE")
        hfc_populate_update_git_worktree(${content_name_clone_in_build_folder}
          SOURCE_DIR "${SOURCE_DIR_IN_BINARY_DIR}"
          GIT_TAG "${FN_ARG_GIT_TAG}"
          OUT_SUCCESS in_build_update_success
        )
        if(in_build_update_success)
          set(in_build_cache_state "WARM")
        else()
          set(in_build_cache_state "CORRUPT")
        endif()
      endif()

      if(in_build_cache_state STREQUAL "CORRUPT")
        hfc_log(STATUS "🧹 ${SOURCE_DIR_IN_BINARY_DIR} is not a usable populated state; clearing for a fresh populate")
        hfc_invalidate_project_population(${content_name_clone_in_build_folder} "${SOURCE_DIR_IN_BINARY_DIR}")
        set(in_build_cache_state "COLD")
      endif()

      if(in_build_cache_state STREQUAL "WARM")
        hfc_log_debug(" - reusing populated in-source tree at ${SOURCE_DIR_IN_BINARY_DIR}")
        hfc_populate_marker_write(${content_name_clone_in_build_folder} "${SOURCE_DIR_IN_BINARY_DIR}")
      else()
        hfc_log_debug(" - populating (${clone_source_in_build_populate_args})")
        hfc_fetchcontent_populate(
          ${content_name_clone_in_build_folder}
          ${clone_source_in_build_populate_args}
        )
        hfc_populate_marker_write(${content_name_clone_in_build_folder} "${SOURCE_DIR_IN_BINARY_DIR}")
      endif()

      FetchContent_SetPopulated(${content_name_clone_in_build_folder}_to_build_folder
        SOURCE_DIR "${SOURCE_DIR_IN_BINARY_DIR}"
        BINARY_DIR "${FN_ARG_BINARY_DIR}"
      )

      hfc_goldilock_release("${lock_dir}" success)
      # Remove extraneous folder after population
      file(REMOVE_RECURSE ${populate_build_path})
    endif()

  endfunction()

endfunction()

function(hfc_populate_project_invoke_internal content_name)
  hfc_log_debug("Invoking project populate function for ${content_name}")
  hfc_populate_project__get_function_name(content_name populate_project_function_name)
  cmake_language(CALL ${populate_project_function_name} ${ARGN})
endfunction()

# Invoke the populate function for ${content_name}
function(hfc_populate_project_invoke content_name)
  hfc_saved_details_get(${content_name} __fetchcontent_arguments)
  hfc_populate_project_invoke_internal(${content_name} ${__fetchcontent_arguments})
endfunction()


function(hfc_populate_project_invoke_clone_in_build_folder_if_required_internal content_name)
  hfc_log_debug("Invoking project populate function for ${content_name}_clone_in_build_folder_if_required")
  hfc_populate_project__get_function_name(content_name populate_project_function_name)
  cmake_language(CALL ${populate_project_function_name}_clone_in_build_folder_if_required ${ARGN})
endfunction()

# Invoke the populate function for ${content_name}
function(hfc_populate_project_invoke_clone_in_build_folder_if_required content_name)
  hfc_saved_details_get(${content_name} __fetchcontent_arguments)
  hfc_populate_project_invoke_clone_in_build_folder_if_required_internal(${content_name} ${__fetchcontent_arguments})
endfunction()

# Forces a fresh population of the project: removes the populate marker, the
# source dir itself and the populate state of BOTH FetchContent
# implementations (the legacy sub-build next to the source cache, and the
# per-build-tree dirs of CMP0168 direct population), so no stale stamp can
# make a later populate skip or half-run its steps.
function(hfc_invalidate_project_population content_name source_dir)
  string(TOLOWER ${content_name} content_name_lower)

  hfc_populate_marker_remove("${source_dir}")

  if(NOT "${source_dir}" STREQUAL "" AND EXISTS "${source_dir}")
    file(REMOVE_RECURSE "${source_dir}")
  endif()

  # legacy sub-build populate layout (lives next to the source cache)
  hfc_compute_subbuild_path(${content_name} subbuild_path SOURCE_DIR ${source_dir})
  if(EXISTS "${subbuild_path}")
    file(REMOVE_RECURSE "${subbuild_path}")
  endif()

  # direct-population layout (per consumer build tree)
  file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/CMakeFiles/fc-stamp/${content_name_lower}")
  file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/CMakeFiles/fc-tmp/${content_name_lower}")
  if(FETCHCONTENT_BASE_DIR)
    file(REMOVE_RECURSE "${FETCHCONTENT_BASE_DIR}/${content_name_lower}-tmp")
  endif()
endfunction()