include(hfc_goldilock_helpers)

macro(hfc_provide_dependency_FETCHCONTENT method package_name)
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
  
  FetchContent_Populate(${package_name}
    ${ARGN}
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