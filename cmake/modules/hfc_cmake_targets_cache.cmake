include(hfc_log)
include(hfc_targets_cache_common)
include(hfc_cmake_targets_discover)
include(hfc_targets_cache_create)
include(hfc_targets_cache_common)

#
# load target information from <library>Targets.cmake (or exports.cmake) files in a
# library build or install tree
#
# Usage:
# hfc_cmake_targets_cache(
#   TARGET_SEARCH_PATH <search_path>      # library build or install tree to scan
#   CACHE_DESTINATION_FILE <path>         # path to the target info cache to write. Parent directory will be created if not already present. Destination file will be overwritten if present
#   CMAKE_ADDITIONAL_EXPORTS <cmake code> # code that defines addtional export targets for a library build
# )
function(hfc_cmake_targets_cache)

  # arguments parsing
  set(options "")
  set(oneValueArgs TARGET_SEARCH_PATH CACHE_DESTINATION_FILE CREATE_TARGET_ALIASES_FN CMAKE_ADDITIONAL_EXPORTS TARGETS_FILE_PATTERN)
  set(multiValueArgs )
  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FN_ARG_TARGET_SEARCH_PATH)
    Hermetic_FetchContent_SetFunctionOverride_Enabled(OFF)
    hfc_log(FATAL_ERROR "hfc_cmake_targets_cache needs a value for CACHE_DESTINATION_FILE parameter to be defined")
  endif()

  if(NOT FN_ARG_CACHE_DESTINATION_FILE)
    Hermetic_FetchContent_SetFunctionOverride_Enabled(OFF)
    hfc_log(FATAL_ERROR "hfc_cmake_targets_cache needs a value for CACHE_DESTINATION_FILE parameter to be defined")
  endif()

  string(RANDOM LENGTH 10 random_fn_suffix)
  set(temp_fn_name "hfc_tmpfn_discover_targets_${random_fn_suffix}")

  #
  # declare a "temporary" function that does the call to hfc_cmake_targets_discover() to load
  # target files
  function(${temp_fn_name})
    hfc_log_debug("Calling ${temp_fn_name} to discover targets in ${FN_ARG_TARGET_SEARCH_PATH}")

    if(FN_ARG_TARGETS_FILE_PATTERN)
      set(discover_find_target_files_additional_arg TARGETS_FILE_PATTERN "${FN_ARG_TARGETS_FILE_PATTERN}")
    else()
      set(discover_find_target_files_additional_arg "")
    endif()

    hfc_cmake_targets_discover(
      SEARCH_PATH "${FN_ARG_TARGET_SEARCH_PATH}"
      CMAKE_ADDITIONAL_EXPORTS "${FN_ARG_CMAKE_ADDITIONAL_EXPORTS}"
      ${discover_find_target_files_additional_arg}
      RESULT found_targets
    )

    hfc_log_debug(" - found targets: ${found_targets}")
  endfunction()

  # note: uses the function above
  hfc_targets_cache_create(
    LOAD_TARGETS_COMMAND "${temp_fn_name}"
    CACHE_DESTINATION_FILE "${FN_ARG_CACHE_DESTINATION_FILE}"
    CREATE_TARGET_ALIASES_FN "${FN_ARG_CREATE_TARGET_ALIASES_FN}"
  )

endfunction()

#
# runs hfc_cmake_targets_cache in an isolated context (in a separate CMake process)
#
# Usage:
# hfc_cmake_targets_cache_isolated(
#   TARGET_SEARCH_PATH <search_path>      # library build or install tree to scan
#   CACHE_DESTINATION_FILE <path>         # path to the target info cache to write. Parent directory will be created if not already present. Destination file will be overwritten if present
#   TEMP_DIR                              # temporary folder to
# )
function(hfc_cmake_targets_cache_isolated)

  # arguments parsing
  set(options "")
  set(oneValueArgs TARGET_SEARCH_PATH CACHE_DESTINATION_FILE TOOLCHAIN_FILE TEMP_DIR CREATE_TARGET_ALIASES CMAKE_ADDITIONAL_EXPORTS TARGETS_FILE_PATTERN)
  set(multiValueArgs )
  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FN_ARG_TARGET_SEARCH_PATH)
    Hermetic_FetchContent_SetFunctionOverride_Enabled(OFF)
    hfc_log(FATAL_ERROR "hfc_cmake_targets_cache_isolated needs a value for CACHE_DESTINATION_FILE parameter to be defined")
  endif()

  if(NOT FN_ARG_CACHE_DESTINATION_FILE)
    Hermetic_FetchContent_SetFunctionOverride_Enabled(OFF)
    hfc_log(FATAL_ERROR "hfc_cmake_targets_cache_isolated needs a value for CACHE_DESTINATION_FILE parameter to be defined")
  endif()

  if(NOT FN_ARG_TEMP_DIR)
    Hermetic_FetchContent_SetFunctionOverride_Enabled(OFF)
    hfc_log(FATAL_ERROR "hfc_cmake_targets_cache_isolated needs a value for TEMP_DIR parameter to be defined")
  endif()


  set(HERMETIC_FETCHCONTENT_ROOT_DIR "${HERMETIC_FETCHCONTENT_ROOT_DIR}")
  set(HERMETIC_FETCHCONTENT_DUMPBUILD_TARGETS_SEARCH_PATH "${FN_ARG_TARGET_SEARCH_PATH}")
  set(HERMETIC_FETCHCONTENT_DUMPBUILD_DESTINATION_CACHE_LIB_FILE "${FN_ARG_CACHE_DESTINATION_FILE}")
  set(HERMETIC_FETCHCONTENT_DUMPBUILD_TARGETS_CREATE_TARGET_ALIASES "${FN_ARG_CREATE_TARGET_ALIASES}")
  set(HERMETIC_FETCHCONTENT_CMAKE_ADDITIONAL_EXPORTS "${FN_ARG_CMAKE_ADDITIONAL_EXPORTS}")
  set(HERMETIC_FETCHCONTENT_TARGETS_FILE_PATTERN "${FN_ARG_TARGETS_FILE_PATTERN}")

  string(RANDOM LENGTH 10 rand_str)
  set(tmp_proj_dir "${FN_ARG_TEMP_DIR}/tmp_${rand_str}")

  file(MAKE_DIRECTORY "${tmp_proj_dir}")

  configure_file(
    "${HERMETIC_FETCHCONTENT_ROOT_DIR}/templates/dump_build_targets.CMakeLists.txt.in"
    "${tmp_proj_dir}/CMakeLists.txt"
    @ONLY
  )

  hfc_log_debug(" - Dump build project in: ${tmp_proj_dir}")

  execute_process(
    COMMAND ${CMAKE_COMMAND} "." "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}" "-DCMAKE_TOOLCHAIN_FILE=${FN_ARG_TOOLCHAIN_FILE}"
    WORKING_DIRECTORY "${tmp_proj_dir}"
    RESULT_VARIABLE CONFIGURE_RESULT
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO STDOUT
  )

  file(REMOVE_RECURSE "${tmp_proj_dir}")

endfunction()