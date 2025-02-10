include(hfc_goldilock_helpers)

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

  FetchContent_Populate(${package_name}
    ${populate_args}
    SOURCE_DIR ${content_source_dir}
    SUBBUILD_DIR "${HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR}/${package_name}-${source_hash_short}-subbuild"
  )

  if(EXISTS "${content_source_dir}/CMakeLists.txt")
    add_subdirectory("${content_source_dir}" "${FN_ARG_BINARY_DIR}")
  elseif(FN_ARG_SOURCE_SUBDIR AND EXISTS "${content_source_dir}/${FN_ARG_SOURCE_SUBDIR}/CMakeLists.txt")
    add_subdirectory("${content_source_dir}/${FN_ARG_SOURCE_SUBDIR}" "${FN_ARG_BINARY_DIR}")
  endif()

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