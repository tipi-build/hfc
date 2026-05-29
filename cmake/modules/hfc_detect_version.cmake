# HermeticFetchContent / version detection
#

include(hfc_log)

#
# Detect the version of an installed package from its metadata files
#
# Tries ConfigVersion.cmake first, then falls back to pkg-config .pc files.
#
# Usage:
# hfc_detect_version(
#   <content_name>
#   INSTALL_PREFIX <path>
#   OUT_VERSION_ARG <variable name>  # set to 'VERSION "<detected>"' or "" if not found
# )
function(hfc_detect_version content_name)

  set(options "")
  set(oneValueArgs INSTALL_PREFIX OUT_VERSION_ARG)
  set(multiValueArgs "")
  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FN_ARG_INSTALL_PREFIX)
    hfc_log(FATAL_ERROR "hfc_detect_version requires INSTALL_PREFIX")
  endif()

  if(NOT FN_ARG_OUT_VERSION_ARG)
    hfc_log(FATAL_ERROR "hfc_detect_version requires OUT_VERSION_ARG")
  endif()

  set(_detected_version "")
  string(TOLOWER "${content_name}" _content_name_lower)

  # Try ConfigVersion.cmake / config-version.cmake
  file(GLOB _config_version_files
    "${FN_ARG_INSTALL_PREFIX}/lib/cmake/${content_name}/*ConfigVersion.cmake"
    "${FN_ARG_INSTALL_PREFIX}/lib/cmake/${content_name}/*-config-version.cmake"
    "${FN_ARG_INSTALL_PREFIX}/lib/cmake/${_content_name_lower}/*ConfigVersion.cmake"
    "${FN_ARG_INSTALL_PREFIX}/lib/cmake/${_content_name_lower}/*-config-version.cmake"
  )

  hfc_log_debug("Version detection for ${content_name}: config_version_files=${_config_version_files}")

  if(_config_version_files)
    list(GET _config_version_files 0 _config_version_file)
    file(READ "${_config_version_file}" _config_version_content)
    string(REGEX MATCH "set\\(PACKAGE_VERSION \"([0-9]+([.][0-9]+)*)\"" _version_match "${_config_version_content}")
    if(CMAKE_MATCH_1)
      set(_detected_version "${CMAKE_MATCH_1}")
    endif()
    hfc_log_debug("Version detection for ${content_name}: detected=${_detected_version}")
  endif()

  if(NOT _detected_version)
    file(GLOB _pc_files
      "${FN_ARG_INSTALL_PREFIX}/lib/pkgconfig/${_content_name_lower}*.pc"
      "${FN_ARG_INSTALL_PREFIX}/lib/pkgconfig/${content_name}*.pc"
    )
    if(_pc_files)
      list(GET _pc_files 0 _pc_file)
      file(STRINGS "${_pc_file}" _pc_version_line REGEX "^Version:")
      if(_pc_version_line)
        string(REGEX REPLACE "^Version:[ \t]*" "" _detected_version "${_pc_version_line}")
      endif()
    endif()
  endif()

  if(_detected_version)
    set(${FN_ARG_OUT_VERSION_ARG} VERSION "${_detected_version}" PARENT_SCOPE)
  else()
    set(${FN_ARG_OUT_VERSION_ARG} "" PARENT_SCOPE)
  endif()

endfunction()
