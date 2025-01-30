include(hfc_log)
include(hfc_targets_cache_common)
include(hfc_targets_cache_consume)
include(hfc_targets_cache_alias)
include(hfc_provide_dependency_FETCHCONTENT)
include(hfc_provide_dependency_FINDPACKAGE)

macro(hfc_provide_dependency method package_name)
  if("${method}" STREQUAL "FETCHCONTENT_MAKEAVAILABLE_SERIAL")
    hfc_provide_dependency_FETCHCONTENT(${method} ${package_name} ${ARGN})
  else()
    hfc_provide_dependency_FINDPACKAGE(${method} ${package_name} ${ARGN})
  endif()
endmacro()

hfc_log_debug("Registering Hermetic FetchContent as CMake dependency provider")

cmake_language(
  SET_DEPENDENCY_PROVIDER hfc_provide_dependency
  SUPPORTED_METHODS FIND_PACKAGE FETCHCONTENT_MAKEAVAILABLE_SERIAL

)