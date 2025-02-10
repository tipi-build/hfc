# HermeticFetchContent / targets cache common
#

include(hfc_log)
include(hfc_required_args)

#
#
set(HERMETIC_FETCHCONTENT_TARGET_LIST_NAME "HERMETIC_FETCHCONTENT_all_targets")
set(HERMETIC_FETCHCONTENT_TARGET_VARIABLE_PREFIX "HERMETIC_FETCHCONTENT_target_var_")
set(HERMETIC_FETCHCONTENT_TARGET_FOUND_PROPERTIES_PREFIX "HERMETIC_FETCHCONTENT_")
set(HERMETIC_FETCHCONTENT_TARGET_FOUND_PROPERTIES_SUFFIX "_found_properties")


#
# constants
set(HERMETIC_FETCHCONTENT_CONST_PREFIX_PLACEHOLDER "@HFC_PREFIX_PLACEHOLDER@")
set(HERMETIC_FETCHCONTENT_CONST_SOURCE_DIR_PLACEHOLDER "@HFC_SOURCE_DIR_PLACEHOLDER@")
set(HERMETIC_FETCHCONTENT_CONST_BINARY_DIR_PLACEHOLDER "@HFC_BINARY_DIR_PLACEHOLDER@")

#
# globals as settings

# enable message() behavior override
set_property(GLOBAL PROPERTY Hermetic_FetchContent_CMakeTargetsDiscover_Function_MessageOverride_Enabled OFF)
set_property(GLOBAL PROPERTY Hermetic_FetchContent_CMakeTargetsDiscover_Function_GetFilenameComponentOverride_Enabled OFF)

# if set to OFF FATAL_ERROR messages will be output as warnings, otherwise they will be silenced
set_property(GLOBAL PROPERTY Hermetic_FetchContent_CMakeTargetsDiscover_Message_Override_Silent ON)


#
# Replaces :: with _ in target names
function(Hermetic_FetchContent_CMakeTargetsDiscover_escape_target_name input OUT_result)
  string(REPLACE "::" "_" escaped_target_name "${input}")
  set("${OUT_result}" "${escaped_target_name}" PARENT_SCOPE)
endfunction()

#
# Replace chars that cannot be used in function names or variables
function(Hermetic_FetchContent_CMakeTargetsDiscover_escape_content_name input OUT_result)
  string(REGEX REPLACE "[^A-Za-z0-9_]" "_" escaped_content_name "${input}")
  set("${OUT_result}" "${escaped_content_name}" PARENT_SCOPE)
endfunction()

#
# Enable or disable the message() override functionality
function(Hermetic_FetchContent_SetFunctionOverride_Enabled value)
  set_property(GLOBAL PROPERTY Hermetic_FetchContent_CMakeTargetsDiscover_Function_MessageOverride_Enabled "${value}")
  set_property(GLOBAL PROPERTY Hermetic_FetchContent_CMakeTargetsDiscover_Function_GetFilenameComponentOverride_Enabled "${value}")
endfunction()


#
# compute the variable name for a found target property
function(Hermetic_FetchContent_TargetsCache_get_found_property_variable_name target_name OUT_result)
  Hermetic_FetchContent_CMakeTargetsDiscover_escape_target_name("${target_name}" escaped_target_name)
  set(${OUT_result} "${HERMETIC_FETCHCONTENT_TARGET_FOUND_PROPERTIES_PREFIX}${escaped_target_name}${HERMETIC_FETCHCONTENT_TARGET_FOUND_PROPERTIES_SUFFIX}" PARENT_SCOPE)
endfunction()


#
# compute the export variable name
#
# Usage:
# Hermetic_FetchContent_TargetsCache_getExportVariableName(
#     TARGET_NAME <name>
#     PROPERTY_NAME <name>
#     EXPORT_VARIABLE_PREFIX <prefix>
#     RESULT <output variable name>
# )
function(Hermetic_FetchContent_TargetsCache_getExportVariableName)

  # arguments parsing
  set(options "")
  set(oneValueArgs TARGET_NAME PROPERTY_NAME EXPORT_VARIABLE_PREFIX RESULT)
  set(multiValueArgs "")
  cmake_parse_arguments(FN_ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FN_ARGS_TARGET_NAME AND FN_ARGS_PROPERTY_NAME AND FN_ARGS_EXPORT_VARIABLE_PREFIX AND FN_ARGS_RESULT)
    Hermetic_FetchContent_SetFunctionOverride_Enabled(OFF)
    hfc_log(FATAL_ERROR "Hermetic_FetchContent_TargetsCache_getExportVariableName needs TARGET_NAME, PROPERTY_NAME, EXPORT_VARIABLE_PREFIX and RESULT to be set")
  endif()

  Hermetic_FetchContent_CMakeTargetsDiscover_escape_target_name("${FN_ARGS_TARGET_NAME}" escaped_target_name)
  set("${FN_ARGS_RESULT}" "${FN_ARGS_EXPORT_VARIABLE_PREFIX}${escaped_target_name}_${FN_ARGS_PROPERTY_NAME}" PARENT_SCOPE)
endfunction()

#
# compute the export variable name
#
# Usage:
# Hermetic_FetchContent_TargetsCache_getExportVariable(
#     TARGET_NAME <name>
#     PROPERTY_NAME <name>
#     EXPORT_VARIABLE_PREFIX <prefix>
#     RESULT <output value>
# )
function(Hermetic_FetchContent_TargetsCache_getExportVariable)

  # arguments parsing
  set(options "")
  set(oneValueArgs TARGET_NAME PROPERTY_NAME EXPORT_VARIABLE_PREFIX RESULT)
  set(multiValueArgs "")
  cmake_parse_arguments(FN_ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FN_ARGS_TARGET_NAME AND FN_ARGS_PROPERTY_NAME AND FN_ARGS_EXPORT_VARIABLE_PREFIX AND FN_ARGS_RESULT)
    Hermetic_FetchContent_SetFunctionOverride_Enabled(OFF)
    hfc_log(FATAL_ERROR "Hermetic_FetchContent_TargetsCache_getExportVariableName needs TARGET_NAME, PROPERTY_NAME, EXPORT_VARIABLE_PREFIX and RESULT to be set")
  endif()


  Hermetic_FetchContent_TargetsCache_getExportVariableName(
    TARGET_NAME "${FN_ARGS_TARGET_NAME}"
    PROPERTY_NAME "${FN_ARGS_PROPERTY_NAME}"
    EXPORT_VARIABLE_PREFIX "${FN_ARGS_EXPORT_VARIABLE_PREFIX}"
    RESULT export_var_name
  )

  set("${FN_ARGS_RESULT}" "${${export_var_name}}" PARENT_SCOPE)
endfunction()

#
# Compute the name of a Hermetic FetchContent target cache file based on the FetchContent content-name
function(get_hermetic_target_cache_file_path content_name result_variable)
  set(${result_variable} "${HERMETIC_FETCHCONTENT_ROOT_PROJECT_BINARY_DIR}/_deps/hermetic_targetcaches/${content_name}.cmake" PARENT_SCOPE)
endfunction()

#
# Compute the name of a Hermetic FetchContent target cache summary (those contain the list of contents consumed by <content_name>) file based on the FetchContent content-name
function(get_hermetic_target_cache_summary_file_path content_name result_variable)
  set(${result_variable} "${HERMETIC_FETCHCONTENT_ROOT_PROJECT_BINARY_DIR}/_deps/hermetic_targetcaches/${content_name}.hfcsummary.cmake" PARENT_SCOPE)
endfunction()

#
# Registers a target cache file for later consumption by the Hermetic FetchContent's dependency_provider
#
# Usage:
# hfc_targets_cache_register_dependency_for_provider(
#   TARGETS_INSTALL_PREFIX <target install prefix> # path to the installed/ tree
#   TARGETS_CACHE_FILE <library cache file>       # library cache file
# )
function(hfc_targets_cache_register_dependency_for_provider content_name)

  # arguments parsing
  set(options "")
  set(oneValueArgs
    TARGETS_INSTALL_PREFIX
    TARGETS_CACHE_FILE
  )
  set(multiValueArgs "")

  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  hfc_required_args(FN_ARG ${oneValueArgs_required})

  if(PARAM_UNPARSED_ARGUMENTS)
    hfc_log(FATAL_ERROR
        "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
        " ${PARAM_UNPARSED_ARGUMENTS}"
    )
  endif()

  set(HERMETIC_FETCHCONTENT_${content_name}_FOUND ON CACHE INTERNAL "Hermetic dependency ${content_name} is known")
  set(HERMETIC_FETCHCONTENT_${content_name}_INSTALL_PREFIX "${FN_ARG_TARGETS_INSTALL_PREFIX}" CACHE INTERNAL "Hermetic dependency install prefix for ${content_name}")
  set(HERMETIC_FETCHCONTENT_${content_name}_TARGETS_CACHE_FILE "${FN_ARG_TARGETS_CACHE_FILE}" CACHE INTERNAL "Hermetic dependency targets cache files for ${content_name}")

endfunction()


#
# Retrieve info for registered target cache dependencies for a given content_name
#
# Usage:
# hfc_targets_cache_register_dependency_for_provider(
#   OUT_FOUND <variable name>                           # TRUE if the dependency was found
#   OUT_TARGETS_INSTALL_PREFIX <target install prefix>  # path to the installed/ tree
#   OUT_TARGETS_CACHE_FILE <library cache file>         # path to the targets cache file
# )
function(hfc_targets_cache_get_registered_info content_name)

  # arguments parsing
  set(options "")
  set(oneValueArgs
    OUT_FOUND
    OUT_TARGETS_INSTALL_PREFIX
    OUT_TARGETS_CACHE_FILE
  )
  set(multiValueArgs "")

  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  hfc_required_args(FN_ARG ${oneValueArgs_required})

  if(PARAM_UNPARSED_ARGUMENTS)
    hfc_log(FATAL_ERROR
        "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
        " ${PARAM_UNPARSED_ARGUMENTS}"
    )
  endif()

  if (DEFINED HERMETIC_FETCHCONTENT_${content_name}_FOUND)
    set(package_found ${HERMETIC_FETCHCONTENT_${content_name}_FOUND})
  else()
    set(package_found FALSE)
  endif()

  if(package_found)
    set(${FN_ARG_OUT_FOUND} TRUE PARENT_SCOPE)
    set(${FN_ARG_OUT_TARGETS_INSTALL_PREFIX} "${HERMETIC_FETCHCONTENT_${content_name}_INSTALL_PREFIX}" PARENT_SCOPE)
    set(${FN_ARG_OUT_TARGETS_CACHE_FILE} "${HERMETIC_FETCHCONTENT_${content_name}_TARGETS_CACHE_FILE}" PARENT_SCOPE)
  else()
    set(${FN_ARG_OUT_FOUND} FALSE PARENT_SCOPE)
  endif()

endfunction()
