# HermeticFetchContent / version detection
#

include(hfc_log)

#
# Detect the version of an installed package from its metadata files
#
# Tries ConfigVersion.cmake first (via include() with mocked inputs, matching
# how CMake's own find_package evaluates version files), then falls back to
# pkg-config .pc files (regex-based, since .pc files are purely declarative).
#
# Usage:
# hfc_detect_version(
#   <content_name>
#   INSTALL_PREFIX <path>
#   OUT_VERSION <variable name>  # set to detected version string, or "" if not found
# )
function(hfc_detect_version content_name)

  set(options "")
  set(oneValueArgs INSTALL_PREFIX OUT_VERSION)
  set(multiValueArgs "")
  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FN_ARG_INSTALL_PREFIX)
    hfc_log(FATAL_ERROR "hfc_detect_version requires INSTALL_PREFIX")
  endif()

  if(NOT FN_ARG_OUT_VERSION)
    hfc_log(FATAL_ERROR "hfc_detect_version requires OUT_VERSION")
  endif()

  set(_detected_version "")
  string(TOLOWER "${content_name}" _content_name_lower)

  # Try ConfigVersion.cmake / config-version.cmake
  # Search under lib/, lib64/, share/ to cover all common CMake install layouts.
  set(_hfc_version_search_dirs
    "${FN_ARG_INSTALL_PREFIX}/lib"
    "${FN_ARG_INSTALL_PREFIX}/lib64"
    "${FN_ARG_INSTALL_PREFIX}/share"
  )

  set(_config_version_files "")
  foreach(_search_dir IN LISTS _hfc_version_search_dirs)
    file(GLOB_RECURSE _hfc_found_files
      "${_search_dir}/*/${content_name}*ConfigVersion.cmake"
      "${_search_dir}/*/${content_name}*-config-version.cmake"
      "${_search_dir}/*/${_content_name_lower}*ConfigVersion.cmake"
      "${_search_dir}/*/${_content_name_lower}*-config-version.cmake"
    )
    if(_hfc_found_files)
      list(APPEND _config_version_files ${_hfc_found_files})
      unset(_hfc_found_files)
    endif()
  endforeach()

  hfc_log_debug("Version detection for ${content_name}: config_version_files=${_config_version_files}")

  # Evaluate ConfigVersion files by including them with mocked inputs
  # (same mechanism as CMake's find_package config-mode version selection)
  foreach(_config_version_file IN LISTS _config_version_files)
    unset(PACKAGE_VERSION)
    unset(PACKAGE_VERSION_COMPATIBLE)
    unset(PACKAGE_VERSION_EXACT)
    unset(PACKAGE_VERSION_UNSUITABLE)
    set(PACKAGE_FIND_NAME "${content_name}")
    set(PACKAGE_FIND_VERSION "")
    set(PACKAGE_FIND_VERSION_MAJOR 0)
    set(PACKAGE_FIND_VERSION_MINOR 0)
    set(PACKAGE_FIND_VERSION_PATCH 0)
    set(PACKAGE_FIND_VERSION_TWEAK 0)
    set(PACKAGE_FIND_VERSION_COUNT 0)
    include("${_config_version_file}" OPTIONAL RESULT_VARIABLE _hfc_version_include_result)
    if(NOT _hfc_version_include_result)
      continue()
    endif()
    if(DEFINED PACKAGE_VERSION AND NOT PACKAGE_VERSION STREQUAL "")
      set(_detected_version "${PACKAGE_VERSION}")
      break()
    endif()
  endforeach()

  hfc_log_debug("Version detection for ${content_name}: detected=${_detected_version}")

  # Fallback to pkg-config .pc files
  if(NOT _detected_version)
    file(GLOB _pc_files
      "${FN_ARG_INSTALL_PREFIX}/lib/pkgconfig/${_content_name_lower}*.pc"
      "${FN_ARG_INSTALL_PREFIX}/lib/pkgconfig/${content_name}*.pc"
      "${FN_ARG_INSTALL_PREFIX}/lib64/pkgconfig/${_content_name_lower}*.pc"
      "${FN_ARG_INSTALL_PREFIX}/lib64/pkgconfig/${content_name}*.pc"
    )
    if(_pc_files)
      list(GET _pc_files 0 _pc_file)
      file(STRINGS "${_pc_file}" _pc_version_line REGEX "^Version:")
      if(_pc_version_line)
        string(REGEX REPLACE "^Version:[ \t]*" "" _detected_version "${_pc_version_line}")
      endif()
    endif()
  endif()

  set(${FN_ARG_OUT_VERSION} "${_detected_version}" PARENT_SCOPE)

endfunction()
