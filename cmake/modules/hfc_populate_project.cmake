include(hfc_log)
include(hfc_required_args)
include(hfc_git_helpers)
include(hfc_goldilock_helpers)
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

    # figure out if this is already the correct state or if we've got to do things.
    is_git_repository(REPOSITORY_DIR "${FN_ARG_SOURCE_DIR}" OUT_RESULT SOURCE_DIR_is_git_repo)

    if(SOURCE_DIR_is_git_repo)

      repo_get_head_id(REPOSITORY_DIR "${FN_ARG_SOURCE_DIR}"
        OUT_COMMIT_ID initial_commit_id
      )

      repo_is_clean(REPOSITORY_DIR "${FN_ARG_SOURCE_DIR}"
        CHECK_IGNORED
        OUT_RESULT initial_repo_is_clean
      )

      if((initial_commit_id STREQUAL FN_ARG_GIT_TAG) AND initial_repo_is_clean)
        hfc_log(STATUS "🟢 Repository ${FN_ARG_SOURCE_DIR} at ${FN_ARG_GIT_TAG} and clean")
        set(done TRUE)
      else()
        # We explicitely do not reset, and trust the stamp files
        #hfc_log(STATUS "Repository ${FN_ARG_SOURCE_DIR} currently at ${initial_commit_id} and is_clean=${initial_repo_is_clean}")
        #checkout_revision_force_clean(
        #  REPOSITORY_DIR "${FN_ARG_SOURCE_DIR}"
        #  GIT_REVISION "${FN_ARG_GIT_TAG}"
        #  OUT_SUCCESS done
        #)
        if (NOT prepatched_RESOLVED_PATCH AND NOT FN_ARG_PATCH_COMMAND)
          hfc_log(WARNING "🔴 Repository ${FN_ARG_SOURCE_DIR} currently at ${initial_commit_id} is not clean. is_clean=${initial_repo_is_clean}. Are the local modifications intentional ?")
        endif()
      endif()

      if(done)
      
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
    
    # we used to fix issues that could occur if the sources are missing but the stamp file were still around
    # we now explictely trust the stamps file, this allow better debugging for developers
    # as this allows modifying the sources of a dependency while debugging configure step
    #hfc_invalidate_project_population(${content_name} "${FN_ARG_SOURCE_DIR}")

    # 
    hfc_log_debug(" - populating (${populate_args})")
    FetchContent_Populate(
      ${content_name}
      ${populate_args}
    )

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

      # fix issues that could occur if the sources are missing but the stamp file is still around
      hfc_invalidate_project_population(${content_name_clone_in_build_folder} "${SOURCE_DIR_IN_BINARY_DIR}")

      # 
      hfc_log_debug(" - populating (${clone_source_in_build_populate_args})")
      FetchContent_Populate(
        ${content_name_clone_in_build_folder}
        ${clone_source_in_build_populate_args}
      )
  
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

# Removes the stamps file to force a new population of the project ( reuses existing archives )
function(hfc_invalidate_project_population content_name source_dir)
  hfc_compute_subbuild_path(${content_name} subbuild_path SOURCE_DIR ${source_dir})

  string(TOLOWER ${content_name} content_name_lower)
  file(REMOVE_RECURSE "${subbuild_path}/${content_name_lower}-populate-prefix/src/${content_name_lower}-populate-stamp")
  file(REMOVE_RECURSE "${subbuild_path}/${content_name_lower}-populate-prefix/tmp")
  file(REMOVE_RECURSE "${subbuild_path}/CMakeFiles/${content_name_lower}-populate-complete")
endfunction()