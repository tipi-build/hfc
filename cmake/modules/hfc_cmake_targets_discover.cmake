# HermeticFetchContent / discover CMake targets
#
# Extracts target information from configure or install trees
# by provinging the Targets/Exports files a way to continue executing
# even if target files are (not yet) built
#
# Overrides CMake's message() function to specially handle FATAL_ERROR messages
#

include(hfc_log)
include(hfc_targets_cache_common)


#
# inject the message() and get_filename_component() overrides once
function(__override_cmake_inbuild_functions_for_discover_cmake_targets)

  # inject ourselves only once
  get_property(override_applied GLOBAL PROPERTY Hermetic_FetchContent_CMakeTargetsDiscover_Function_Overrides_Applied)

  if(NOT override_applied)
    hfc_log_debug("Overriding CMake built-in message() and get_filename_component()")
    set_property(GLOBAL PROPERTY Hermetic_FetchContent_CMakeTargetsDiscover_Function_Overrides_Applied ON)

    #
    # redefine message() with a version that doesn't stop on FATAL_ERROR
    function(message)
      list(GET ARGN 0 log_category)
      list(SUBLIST ARGN 1 -1 remaining_args)

      get_property(override_enabled GLOBAL PROPERTY Hermetic_FetchContent_CMakeTargetsDiscover_Function_MessageOverride_Enabled)
      get_property(silent GLOBAL PROPERTY Hermetic_FetchContent_CMakeTargetsDiscover_Message_Override_Silent)

      if(override_enabled AND "${log_category}" STREQUAL "FATAL_ERROR")
        if(NOT silent)
          _message(WARNING "Ignoring fatal error: ${remaining_args}")
        endif()
      else()
        _message("${log_category}" "${remaining_args}")
      endif()
    endfunction()

    #
    # redefine get_filename_component() to one that lies when _IMPORT_PREFIX is passed as output variable
    # this relies on *Targets.cmake files using that variable name to build _IMPORT_PREFIX from the CMAKE_CURRENT_LIST_FILE
    function(get_filename_component)
      list(GET ARGN 0 out_variable_name)

      get_property(override_enabled GLOBAL PROPERTY Hermetic_FetchContent_CMakeTargetsDiscover_Function_GetFilenameComponentOverride_Enabled)

      if(override_enabled AND "${CMAKE_IMPORT_FILE_VERSION}" EQUAL 1 AND "${out_variable_name}" STREQUAL "_IMPORT_PREFIX")
        set(${out_variable_name} "${HERMETIC_FETCHCONTENT_CONST_PREFIX_PLACEHOLDER}" PARENT_SCOPE)
      else()

        list(LENGTH ARGN _list_length)
        list(GET ARGN 1 arg_FileName)
        list(GET ARGN 2 arg_mode)

        list(FIND ARGN "CACHE" ix_CACHE)
        if("${ix_CACHE}" GREATER_EQUAL 0)
          set(arg_CACHE ON)
        endif()

        list(FIND ARGN "BASE_DIR" ix_BASE_DIR)
        if("${ix_BASE_DIR}" GREATER_EQUAL 0)
          MATH(EXPR ix_BASE_DIR_value "${ix_BASE_DIR}+1")
          list(GET ARGN ${ix_BASE_DIR_value} arg_BASE_DIR_VALUE)
          set(arg_BASE_DIR_ARG "BASE_DIR")
        endif()

        list(FIND ARGN "PROGRAM_ARGS" ix_PROGRAM_ARGS)
        if("${ix_PROGRAM_ARGS}" GREATER_EQUAL 0)
          MATH(EXPR ix_PROGRAM_ARGS_value "${ix_PROGRAM_ARGS}+1")
          list(GET ARGN ${ix_PROGRAM_ARGS_value} arg_PROGRAM_ARGS_VARIABLE)
          set(arg_PROGRAM_ARGS_FOUND ON)
        endif()

        if("${arg_PROGRAM_ARGS_FOUND}")
          _get_filename_component(result "${arg_FileName}" "${arg_mode}" PROGRAM_ARGS result_program_args "${arg_BASE_DIR_ARG}" "${arg_BASE_DIR_VALUE}")
        else()
          _get_filename_component(result "${arg_FileName}" "${arg_mode}" "${arg_BASE_DIR_ARG}" "${arg_BASE_DIR_VALUE}")
        endif()

        if("${arg_CACHE}")
          if(DEFINED CACHE{${out_variable_name}})
            set(${out_variable_name} "$CACHE{${out_variable_name}}" PARENT_SCOPE)
          else()
            set(${out_variable_name} "${result}" PARENT_SCOPE)
            set(${out_variable_name} "${result}" CACHE STRING "")
          endif()
        else()
          set(${out_variable_name} "${result}" PARENT_SCOPE)
        endif()

        if("${arg_CACHE}" AND "${arg_PROGRAM_ARGS_FOUND}")
          if(DEFINED CACHE{${arg_PROGRAM_ARGS_VARIABLE}})
            set(${arg_PROGRAM_ARGS_VARIABLE} "$CACHE{${arg_PROGRAM_ARGS_VARIABLE}}" PARENT_SCOPE)
          else()
            set(${arg_PROGRAM_ARGS_VARIABLE} "${result_program_args}" PARENT_SCOPE)
            set(${arg_PROGRAM_ARGS_VARIABLE} "${result_program_args}" CACHE STRING "")
          endif()
        else()
          set(${arg_PROGRAM_ARGS_VARIABLE} "${result_program_args}" PARENT_SCOPE)
        endif()

      endif()
    endfunction()

  endif()

endfunction()


#
# Scans for target files in the specified directory
#
# Usage:
# hfc_cmake_targets_discover_find_target_files(
#   SEARCH_PATH <path to search for target files>
#   RESULT_LIST <output list of found files>
#   TARGETS_FILE_PATTERN <optional regex used to match the targets  export file(s)>
# )
function(hfc_cmake_targets_discover_find_target_files)

  # arguments parsing
  set(options "")
  set(oneValueArgs SEARCH_PATH RESULT_LIST TARGETS_FILE_PATTERN)
  set(multiValueArgs "")
  cmake_parse_arguments(FN_ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FN_ARGS_SEARCH_PATH AND FN_ARGS_RESULT_LIST)
    Hermetic_FetchContent_SetFunctionOverride_Enabled(OFF)
    hfc_log(FATAL_ERROR "hfc_cmake_targets_discover_find_target_files needs SEARCH_PATH and RESULT_LIST parameters to be set")
  endif()

  set(findTargetFiles_result "")

  # find the *([tT]argets)|(export).cmake files
  file(GLOB_RECURSE matched_files "${FN_ARGS_SEARCH_PATH}/*")

  foreach(found_file IN LISTS matched_files)

    set(file_matches FALSE)

    if(NOT "${FN_ARGS_TARGETS_FILE_PATTERN}" STREQUAL "" AND found_file MATCHES "${FN_ARGS_TARGETS_FILE_PATTERN}")
      set(file_matches TRUE)
    elseif(found_file MATCHES "([Tt]argets|[Ee]xport(s?))\\.cmake$")
      set(file_matches TRUE)
    endif()

    if(file_matches)
      if(found_file MATCHES "^${FN_ARGS_SEARCH_PATH}/_deps")
        hfc_log_debug("Skipping target file in nested fetchcontent subfolder: ${found_file}")
        continue()
      endif()

      hfc_log_debug("Found target file: ${found_file}")
      list(APPEND findTargetFiles_result "${found_file}")
    endif()

  endforeach()

  set(${FN_ARGS_RESULT_LIST} "${findTargetFiles_result}" PARENT_SCOPE)

endfunction()



#
# load target information from <library>Targets.cmake (or exports.cmake) files in a
# library build or install tree
#
# Usage:
# hfc_cmake_targets_discover(
#   RESULT <output_list>      # variable to write the found target names to
#
#   SEARCH_PATH <path>        # absolute path to scan for targets/export files in
#
#   EXPORT_VARIABLE_PREFIX    # prefix to add to the exported variable names, defaults to "DiscoverTargets_"
#
#   EXPORT_PROPERTIES <list>  # list of TARGET_PROPERTIES to set in the PARENT_SCOPE
#                             # if "NAME;TYPE" is provided and the found target name is "BoringSSL::ssl"
#                             # then the variables "${EXPORT_VARIABLE_PREFIX}BoringSSL_ssl_NAME" and
#                             # "${EXPORT_VARIABLE_PREFIX}BoringSSL_ssl_TYPE" will be available in the
#                             # caller scope after executing load_targets()
#
#   CMAKE_ADDITIONAL_EXPORTS  # <cmake code to create additional targets>
#
#   TARGETS_FILE_PATTERN      # optional regex used to match the targets  export file(s)
#
# )
function(hfc_cmake_targets_discover)
  set(OUT_targets_list "")
  get_property(override_enabled_initially GLOBAL PROPERTY Hermetic_FetchContent_CMakeTargetsDiscover_Message_Override_Enabled)
  __override_cmake_inbuild_functions_for_discover_cmake_targets()

  # arguments parsing
  set(options "")
  set(oneValueArgs RESULT SEARCH_PATH CMAKE_ADDITIONAL_EXPORTS EXPORT_VARIABLE_PREFIX TARGETS_FILE_PATTERN)
  set(multiValueArgs EXPORT_PROPERTIES)
  cmake_parse_arguments(LOAD_TARGETS_ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT LOAD_TARGETS_ARGS_EXPORT_PROPERTIES)
    set(LOAD_TARGETS_ARGS_EXPORT_PROPERTIES "")
  endif()

  if(NOT LOAD_TARGETS_ARGS_EXPORT_VARIABLE_PREFIX)
    set(LOAD_TARGETS_ARGS_EXPORT_VARIABLE_PREFIX "DiscoverTargets_")
  endif()

  if(NOT LOAD_TARGETS_ARGS_SEARCH_PATH)
    Hermetic_FetchContent_SetFunctionOverride_Enabled(OFF)
    hfc_log(FATAL_ERROR "load_targets needs SEARCH_PATH parameter to be defined")
  endif()

  if(NOT LOAD_TARGETS_ARGS_RESULT)
    Hermetic_FetchContent_SetFunctionOverride_Enabled(OFF)
    hfc_log(FATAL_ERROR "load_targets needs RESULT parameter to be defined")
  endif()

  # have a baseline so we only process new targets
  get_property(targets_before_scan DIRECTORY PROPERTY IMPORTED_TARGETS)
  Hermetic_FetchContent_SetFunctionOverride_Enabled(ON)

  if(LOAD_TARGETS_ARGS_TARGETS_FILE_PATTERN)
    set(discover_find_target_files_additional_arg TARGETS_FILE_PATTERN "${LOAD_TARGETS_ARGS_TARGETS_FILE_PATTERN}")
  else()
    set(discover_find_target_files_additional_arg "")
  endif()

  # find the *([tT]argets)|(export).cmake files
  hfc_cmake_targets_discover_find_target_files(SEARCH_PATH "${LOAD_TARGETS_ARGS_SEARCH_PATH}" ${discover_find_target_files_additional_arg} RESULT_LIST target_files)
  foreach(found_file IN LISTS target_files)
    hfc_log(STATUS "Processing targets / export file: ${found_file}")
    include("${found_file}")
  endforeach()

  if(FN_ARG_CMAKE_ADDITIONAL_EXPORTS)
    hfc_log_debug(" - invoking additional exports callback")
    cmake_language(EVAL CODE "${FN_ARG_CMAKE_ADDITIONAL_EXPORTS}")
    hfc_log_debug(" -> aliases: ${aliases}")
  endif()

  # gather imported targets
  get_property(targets_after_scan DIRECTORY PROPERTY IMPORTED_TARGETS)

  foreach(target IN LISTS targets_after_scan)

    if(NOT "${target}" IN_LIST targets_before_scan)
      list(APPEND OUT_targets_list "${target}")

      hfc_log_debug("Found target: '${target}'")

      # publish properties in the parent scope
      if(NOT "${LOAD_TARGETS_ARGS_EXPORT_PROPERTIES}" EQUAL "")

        foreach(property_name IN LISTS LOAD_TARGETS_ARGS_EXPORT_PROPERTIES)
          get_target_property(property_value ${target} "${property_name}")

          if(NOT "${property_value}" STREQUAL "property_value-NOTFOUND")
            hfc_log_debug(" - target property: ${property_name} = ${property_value}")

            Hermetic_FetchContent_TargetsCache_getExportVariableName(
              TARGET_NAME "${target}"
              PROPERTY_NAME "${property_name}"
              EXPORT_VARIABLE_PREFIX "${LOAD_TARGETS_ARGS_EXPORT_VARIABLE_PREFIX}"
              RESULT export_variable_name
            )

            set(${export_variable_name} "${property_value}" PARENT_SCOPE)
          endif()
        endforeach()

      endif()
    endif()

  endforeach()

  set("${LOAD_TARGETS_ARGS_RESULT}" "${OUT_targets_list}" PARENT_SCOPE)
  Hermetic_FetchContent_SetFunctionOverride_Enabled("${override_enabled_initially}") # reset to initial state
endfunction()


#
# run internal tests to validate that the overridden get_filename_component() behaves as
# desired when enable / disabled
#set(HERMETIC_FETCHCONTENT_RUN_INTERNAL_DISCOVER_TARGETS_TEST ON)
if(${HERMETIC_FETCHCONTENT_RUN_INTERNAL_DISCOVER_TARGETS_TEST})
  hfc_log(STATUS "Running Internal tests for get_filename_component() override")

  set(test_success ON)

  # Assertion macro
  macro(check desc actual expect)
    if(NOT "x${actual}" STREQUAL "x${expect}")
      message(SEND_ERROR "${desc}: got \"${actual}\", not \"${expect}\"")
      set(test_success OFF)
    endif()
  endmacro()

  macro(hermetic_fetchcontent_INTERNAL_TEST override_enabled)

    # clear vars from CACHE so we can do two runs in a row
    unset(test_cache CACHE)
    unset(test_cache_program_args_1 CACHE)
    unset(test_cache_program_name_2 CACHE)
    unset(test_cache_program_args_3 CACHE)
    unset(test_cache_program_name_4 CACHE)

    FILE(WRITE "/tmp/hermetic_fetchcontent_knowncommand.sh" "")

    # General test of all component types given an absolute path.
    set(filename "/path/to/filename.ext.in")
    set(expect_DIRECTORY "/path/to")
    set(expect_NAME "filename.ext.in")
    set(expect_EXT ".ext.in")
    set(expect_NAME_WE "filename")
    set(expect_LAST_EXT ".in")
    set(expect_NAME_WLE "filename.ext")
    set(expect_PATH "/path/to")
    foreach(c DIRECTORY NAME EXT NAME_WE LAST_EXT NAME_WLE PATH)
      get_filename_component(actual_${c} "${filename}" ${c})
      check("${c}" "${actual_${c}}" "${expect_${c}}")
      list(APPEND non_cache_vars actual_${c})
    endforeach()

    # Test Windows paths with DIRECTORY component and an absolute Windows path.
    get_filename_component(test_slashes "C:\\path\\to\\filename.ext.in" DIRECTORY)
    check("DIRECTORY from backslashes" "${test_slashes}" "C:/path/to")
    list(APPEND non_cache_vars test_slashes)

    get_filename_component(test_winroot "C:\\filename.ext.in" DIRECTORY)
    check("DIRECTORY in windows root" "${test_winroot}" "C:/")
    list(APPEND non_cache_vars test_winroot)

    # Test finding absolute paths.
    get_filename_component(test_absolute "/path/to/a/../filename.ext.in" ABSOLUTE)
    check("ABSOLUTE" "${test_absolute}" "/path/to/filename.ext.in")

    get_filename_component(test_absolute "/../path/to/filename.ext.in" ABSOLUTE)
    check("ABSOLUTE .. in root" "${test_absolute}" "/path/to/filename.ext.in")
    get_filename_component(test_absolute "C:/../path/to/filename.ext.in" ABSOLUTE)
    check("ABSOLUTE .. in windows root" "${test_absolute}" "C:/path/to/filename.ext.in")

    list(APPEND non_cache_vars test_absolute)

    # Test finding absolute paths from various base directories.

    get_filename_component(test_abs_base "testdir1" ABSOLUTE)
    check("ABSOLUTE .. from default base" "${test_abs_base}"
          "${CMAKE_CURRENT_SOURCE_DIR}/testdir1")

    get_filename_component(test_abs_base "../testdir2" ABSOLUTE
                          BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dummydir")
    check("ABSOLUTE .. from dummy base to parent" "${test_abs_base}"
          "${CMAKE_CURRENT_SOURCE_DIR}/testdir2")

    get_filename_component(test_abs_base "testdir3" ABSOLUTE
                          BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dummydir")
    check("ABSOLUTE .. from dummy base to child" "${test_abs_base}"
          "${CMAKE_CURRENT_SOURCE_DIR}/dummydir/testdir3")

    list(APPEND non_cache_vars test_abs_base)

    # Test finding absolute paths with CACHE parameter.  (Note that more
    # rigorous testing of the CACHE parameter comes later with PROGRAM).

    get_filename_component(test_abs_base_1 "testdir4" ABSOLUTE CACHE)
    check("ABSOLUTE CACHE 1" "${test_abs_base_1}"
          "${CMAKE_CURRENT_SOURCE_DIR}/testdir4")
    list(APPEND cache_vars test_abs_base_1)

    get_filename_component(test_abs_base_2 "testdir5" ABSOLUTE
                          BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dummydir"
                          CACHE)
    check("ABSOLUTE CACHE 2" "${test_abs_base_2}"
          "${CMAKE_CURRENT_SOURCE_DIR}/dummydir/testdir5")
    list(APPEND cache_vars test_abs_base_2)

    # Test the PROGRAM component type.
    get_filename_component(test_program_name "/ arg1 arg2" PROGRAM)
    check("PROGRAM with no args output" "${test_program_name}" "/")

    get_filename_component(test_program_name "/ arg1 arg2" PROGRAM
      PROGRAM_ARGS test_program_args)
    check("PROGRAM with args output: name" "${test_program_name}" "/")
    check("PROGRAM with args output: args" "${test_program_args}" " arg1 arg2")

    get_filename_component(test_program_name " " PROGRAM)
    check("PROGRAM with just a space" "${test_program_name}" "")

    get_filename_component(test_program_name "/tmp/hermetic_fetchcontent_knowncommand.sh" PROGRAM)
    check("PROGRAM specified explicitly without quoting" "${test_program_name}" "/tmp/hermetic_fetchcontent_knowncommand.sh")

    get_filename_component(test_program_name "\"/tmp/hermetic_fetchcontent_knowncommand.sh\" arg1 arg2" PROGRAM
      PROGRAM_ARGS test_program_args)
    check("PROGRAM specified explicitly with arguments: name" "${test_program_name}" "/tmp/hermetic_fetchcontent_knowncommand.sh")
    check("PROGRAM specified explicitly with arguments: args" "${test_program_args}" " arg1 arg2")

    list(APPEND non_cache_vars test_program_name)
    list(APPEND non_cache_vars test_program_args)

    # Test CACHE parameter for most component types.
    get_filename_component(test_cache "/path/to/filename.ext.in" DIRECTORY CACHE)
    check("CACHE 1" "${test_cache}" "/path/to")
    # Make sure that the existing CACHE entry from previous is honored:
    get_filename_component(test_cache "/path/to/other/filename.ext.in" DIRECTORY CACHE)
    check("CACHE 2" "${test_cache}" "/path/to")
    unset(test_cache CACHE)
    get_filename_component(test_cache "/path/to/other/filename.ext.in" DIRECTORY CACHE)
    check("CACHE 3" "${test_cache}" "/path/to/other")

    list(APPEND cache_vars test_cache)

    # Test the PROGRAM component type with CACHE specified.

    # 1. Make sure it makes a cache variable in the first place for basic usage:
    get_filename_component(test_cache_program_name_1 "/ arg1 arg2" PROGRAM CACHE)
    check("PROGRAM CACHE 1 with no args output" "${test_cache_program_name_1}" "/")
    list(APPEND cache_vars test_cache_program_name_1)

    # 2. Set some existing cache variables & make sure the function returns them:
    set(test_cache_program_name_2 DummyProgramName CACHE FILEPATH "")
    get_filename_component(test_cache_program_name_2 "/ arg1 arg2" PROGRAM CACHE)
    check("PROGRAM CACHE 2 with no args output" "${test_cache_program_name_2}"
      "DummyProgramName")
    list(APPEND cache_vars test_cache_program_name_2)

    # 3. Now test basic usage when PROGRAM_ARGS is used:
    get_filename_component(test_cache_program_name_3 "/ arg1 arg2" PROGRAM
      PROGRAM_ARGS test_cache_program_args_3 CACHE)
    check("PROGRAM CACHE 3 name" "${test_cache_program_name_3}" "/")
    check("PROGRAM CACHE 3 args" "${test_cache_program_args_3}" " arg1 arg2")
    list(APPEND cache_vars test_cache_program_name_3)
    list(APPEND cache_vars test_cache_program_args_3)

    # 4. Test that existing cache variables are returned when PROGRAM_ARGS is used:
    set(test_cache_program_name_4 DummyPgm CACHE FILEPATH "")
    set(test_cache_program_args_4 DummyArgs CACHE STRING "")
    get_filename_component(test_cache_program_name_4 "/ arg1 arg2" PROGRAM
      PROGRAM_ARGS test_cache_program_args_4 CACHE)
    check("PROGRAM CACHE 4 name" "${test_cache_program_name_4}" "DummyPgm")
    check("PROGRAM CACHE 4 args" "${test_cache_program_args_4}" "DummyArgs")
    list(APPEND cache_vars test_cache_program_name_4)
    list(APPEND cache_vars test_cache_program_name_4)

    # Test that ONLY the expected cache variables were created.
    get_cmake_property(current_cache_vars CACHE_VARIABLES)
    get_cmake_property(current_vars VARIABLES)

    foreach(thisVar ${cache_vars})
      if(NOT thisVar IN_LIST current_cache_vars)
        message(SEND_ERROR "${thisVar} expected in cache but was not found.")
      endif()
    endforeach()

    foreach(thisVar ${non_cache_vars})
      if(thisVar IN_LIST current_cache_vars)
        message(SEND_ERROR "${thisVar} unexpectedly found in cache.")
      endif()
      if(NOT thisVar IN_LIST current_vars)
        # Catch likely typo when appending to non_cache_vars:
        message(SEND_ERROR "${thisVar} not found in regular variable list.")
      endif()
    endforeach()



    if(${override_enabled})

      unset(CMAKE_IMPORT_FILE_VERSION)
      get_filename_component(_IMPORT_PREFIX "/some/path" PATH)
      check("import prefix override" "${_IMPORT_PREFIX}" "/some")

      unset(_IMPORT_PREFIX)
      set(CMAKE_IMPORT_FILE_VERSION "1")
      get_filename_component(_IMPORT_PREFIX "/some/path" PATH)
      check("import prefix override" "${_IMPORT_PREFIX}" "${HERMETIC_FETCHCONTENT_CONST_PREFIX_PLACEHOLDER}")

    else()

      unset(CMAKE_IMPORT_FILE_VERSION)
      get_filename_component(_IMPORT_PREFIX "/some/path" PATH)
      check("import prefix override" "${_IMPORT_PREFIX}" "/some")

      unset(_IMPORT_PREFIX)
      set(CMAKE_IMPORT_FILE_VERSION "1")
      get_filename_component(_IMPORT_PREFIX "/some/path" PATH)
      check("import prefix override" "${_IMPORT_PREFIX}" "/some")

    endif()

  endmacro()


  __override_cmake_inbuild_functions_for_discover_cmake_targets()
  set_property(GLOBAL PROPERTY Hermetic_FetchContent_CMakeTargetsDiscover_Function_MessageOverride_Enabled OFF) # we want to stop if something goes wrong

  set_property(GLOBAL PROPERTY Hermetic_FetchContent_CMakeTargetsDiscover_Function_GetFilenameComponentOverride_Enabled OFF)
  hermetic_fetchcontent_INTERNAL_TEST(OFF)

  set_property(GLOBAL PROPERTY Hermetic_FetchContent_CMakeTargetsDiscover_Function_GetFilenameComponentOverride_Enabled ON)
  hermetic_fetchcontent_INTERNAL_TEST(On)

  if(NOT "${test_success}")
    hfc_log(FATAL_ERROR "Failed test")
  endif()

endif()