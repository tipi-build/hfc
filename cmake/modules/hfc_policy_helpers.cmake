#[=======================================================================[.rst:
hfc_policy_helpers
------------------------------------------------------------------------------------------
Policy scope helpers for HFC's FetchContent_Populate call sites.

FetchContent_Populate() reads CMP0168 from the policy scope of its caller at
call time; the consumer's own policy version must never decide which populate
implementation HFC's cache machinery runs on. HFC's call sites only ever
populate a COLD (missing/empty) source dir — warm caches are detected and
skipped beforehand (see hfc_populate_cache_state.cmake) — which makes
CMP0168 NEW ("direct population", no sub-build) both safe and preferable:
there is nothing in the dir for its clone step to destroy, and it saves a
nested CMake configure per dependency. On CMake < 3.30 the policy does not
exist and the sub-build implementation is the only one.

Setting HERMETIC_FETCHCONTENT_FORCE_SUBBUILD=ON restores the previous
behavior (CMP0168 pinned to OLD, sub-build populate with its stamps next to
the source cache) as an escape hatch.
#]=======================================================================]

# Call FetchContent_Populate() with a deterministic CMP0168 setting.
#
# FetchContent_Populate() reads CMP0168 from its caller's policy scope at call
# time (cmake_policy(GET ... PARENT_SCOPE)). This wrapper function is that
# caller: the mode set in its own policy scope is what FetchContent sees, and
# it never leaks to the surrounding scope (function policy scopes are
# isolated). Note a cmake_policy(PUSH)/POP pair around the call site would NOT
# work from helper macros: CMake requires PUSH/POP to balance within a single
# macro invocation.
#
# Only call this against a cold source dir (see module docs). SUBBUILD_DIR in
# the arguments is used by the sub-build implementation and ignored by direct
# population, so passing it is always safe.
#
# The <contentName>_SOURCE_DIR / _BINARY_DIR / _POPULATED variables that
# FetchContent_Populate() sets in its caller are re-exported to our caller.
function(hfc_fetchcontent_populate content_name)
  if(POLICY CMP0168)
    if(HERMETIC_FETCHCONTENT_FORCE_SUBBUILD)
      cmake_policy(SET CMP0168 OLD) # escape hatch: legacy sub-build populate
    else()
      cmake_policy(SET CMP0168 NEW) # direct population, cold dirs only

      # HFC decided this content is COLD: direct population's per-build-tree
      # step stamps must not contradict that (e.g. reconfiguring the same
      # build tree after HFC_V1_REMOVE_SOURCE_DIR_AFTER_INSTALL deleted the
      # cache sources: a surviving download.stamp would skip the download and
      # leave the source dir empty)
      string(TOLOWER "${content_name}" content_name_lower)
      file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/CMakeFiles/fc-stamp/${content_name_lower}")
      file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/CMakeFiles/fc-tmp/${content_name_lower}")
      if(FETCHCONTENT_BASE_DIR)
        file(REMOVE_RECURSE "${FETCHCONTENT_BASE_DIR}/${content_name_lower}-tmp")
      endif()
    endif()
  endif()

  FetchContent_Populate(${content_name} ${ARGN})

  foreach(suffix IN ITEMS SOURCE_DIR BINARY_DIR POPULATED)
    if(DEFINED ${content_name}_${suffix})
      set(${content_name}_${suffix} "${${content_name}_${suffix}}" PARENT_SCOPE)
    endif()
  endforeach()
endfunction()

# Resolve the effective CMAKE_POLICY_VERSION_MINIMUM for a content.
#
# CMake 4 errors on projects declaring cmake_minimum_required(VERSION < 3.5);
# CMAKE_POLICY_VERSION_MINIMUM is the documented escape hatch for building
# unmodified legacy third-party sources. HFC exposes it as:
#  - per-content: HERMETIC_POLICY_VERSION_MINIMUM on FetchContent_MakeHermetic()
#  - global:      HERMETIC_FETCHCONTENT_POLICY_VERSION_MINIMUM
# The per-content value wins; an empty result means "feature off".
function(hfc_resolve_policy_version_minimum content_specific_value OUT_result)
  set(result "${content_specific_value}")
  if(result STREQUAL "" AND DEFINED HERMETIC_FETCHCONTENT_POLICY_VERSION_MINIMUM)
    set(result "${HERMETIC_FETCHCONTENT_POLICY_VERSION_MINIMUM}")
  endif()
  set(${OUT_result} "${result}" PARENT_SCOPE)
endfunction()

# Apply CMAKE_POLICY_VERSION_MINIMUM around an add_subdirectory() of
# third-party sources without leaking it into the surrounding scope (call
# sites can be macros expanding in consumer scope). A value the consumer set
# themselves always wins: we only set it when it is not already defined.
macro(hfc_push_policy_version_minimum value)
  set(_hfc_policy_version_minimum_pushed FALSE)
  if(NOT "${value}" STREQUAL "" AND NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
    set(CMAKE_POLICY_VERSION_MINIMUM "${value}")
    set(_hfc_policy_version_minimum_pushed TRUE)
  endif()
endmacro()

macro(hfc_pop_policy_version_minimum)
  if(_hfc_policy_version_minimum_pushed)
    unset(CMAKE_POLICY_VERSION_MINIMUM)
  endif()
  unset(_hfc_policy_version_minimum_pushed)
endmacro()
