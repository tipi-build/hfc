#[=======================================================================[.rst:
hfc_policy_helpers
------------------------------------------------------------------------------------------
Policy scope helpers for HFC's FetchContent_Populate call sites.

FetchContent_Populate() reads CMP0168 from the policy scope of its caller at
call time. When the consuming project sets a policy version >= 3.30 (the
common case on CMake 4.x), CMP0168 defaults to NEW and FetchContent switches
to "direct population", which ignores SUBBUILD_DIR and keeps its stamp files
in the consumer's CMAKE_BINARY_DIR/CMakeFiles/fc-stamp. HFC's shared source
cache relies on the sub-build implementation keeping stamps next to the cache
(so they survive build-folder wipes and are shared across consumers); under
direct population a fresh build folder would rm -rf and re-clone the shared
cache. Pin CMP0168 to OLD strictly around our own populate calls.

The push/pop pair keeps the pin from leaking into the surrounding scope --
several call sites are macros expanding in consumer scope.
#]=======================================================================]

# Call FetchContent_Populate() with CMP0168 pinned to OLD.
#
# FetchContent_Populate() reads CMP0168 from its caller's policy scope at call
# time (cmake_policy(GET ... PARENT_SCOPE)). This wrapper function is that
# caller: the pin set in its own policy scope is what FetchContent sees, and
# it never leaks to the surrounding scope (function policy scopes are
# isolated). Note a cmake_policy(PUSH)/POP pair around the call site would NOT
# work from helper macros: CMake requires PUSH/POP to balance within a single
# macro invocation.
#
# The <contentName>_SOURCE_DIR / _BINARY_DIR / _POPULATED variables that
# FetchContent_Populate() sets in its caller are re-exported to our caller.
function(hfc_fetchcontent_populate content_name)
  if(POLICY CMP0168)
    cmake_policy(SET CMP0168 OLD) # keep the sub-build populate implementation
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
