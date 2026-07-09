include(hfc_log)
include(hfc_targets_cache_common)
include(hfc_targets_cache_consume)
include(hfc_targets_cache_alias)

# Variables saved and restored around hermetic find_package() BYPASS_PROVIDER calls
# via _hfc_find_scope_save/_hfc_find_scope_restore.
# The native-forward branch sets CMAKE_FIND_ROOT_PATH_MODE_*=BOTH and
# CMAKE_FIND_USE_*=FALSE so only <Pkg>_ROOT is searched.
# The bypass/FORCE_SYSTEM branch sets CMAKE_FIND_ROOT_PATH_MODE_*=BOTH so
# that packages outside the sysroot can still be found.
set(_HFC_FIND_SCOPE_VARS
    CMAKE_FIND_ROOT_PATH_MODE_PROGRAM
    CMAKE_FIND_ROOT_PATH_MODE_LIBRARY
    CMAKE_FIND_ROOT_PATH_MODE_INCLUDE
    CMAKE_FIND_ROOT_PATH_MODE_PACKAGE
    CMAKE_FIND_USE_CMAKE_PATH
    CMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH
    CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH
    CMAKE_FIND_USE_CMAKE_SYSTEM_PATH
    CMAKE_FIND_USE_PACKAGE_REGISTRY
    CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY
)

macro(_hfc_find_scope_save)
  foreach(_hfc_fsv IN LISTS _HFC_FIND_SCOPE_VARS)
    if(DEFINED ${_hfc_fsv})
      set(_hfc_backup_${_hfc_fsv} "${${_hfc_fsv}}")
    endif()
  endforeach()
  unset(_hfc_fsv)
endmacro()

macro(_hfc_find_scope_restore)
  foreach(_hfc_fsv IN LISTS _HFC_FIND_SCOPE_VARS)
    if(DEFINED _hfc_backup_${_hfc_fsv})
      set(${_hfc_fsv} "${_hfc_backup_${_hfc_fsv}}")
      unset(_hfc_backup_${_hfc_fsv})
    else()
      unset(${_hfc_fsv})
    endif()
  endforeach()
  unset(_hfc_fsv)
endmacro()


macro(hfc_provide_dependency_FINDPACKAGE method _hfc_prov_content_name_arg)
  set(_hfc_prov_content_name "${_hfc_prov_content_name_arg}")
  hfc_log_debug("Received find_package() request for package name ${_hfc_prov_content_name}")

  HermeticFetchContent_ResolveContentNameAlias(${_hfc_prov_content_name} _hfc_prov_package_name) # resolve alias is available...

  if(NOT _hfc_prov_package_name STREQUAL _hfc_prov_content_name)
    hfc_log_debug(" - resolved to alias ${_hfc_prov_package_name}")
  endif()

  string(TOUPPER "${_hfc_prov_package_name}" _hfc_prov_package_name_uppercase)

  set(_hfc_prov_targetcache_version "")
  hfc_targets_cache_get_registered_info(
    ${_hfc_prov_package_name}
    OUT_FOUND _hfc_prov_package_found
    OUT_TARGETS_INSTALL_PREFIX _hfc_prov_targetcache_install_prefix
    OUT_TARGETS_CACHE_FILE _hfc_prov_targetscache_file
    OUT_FIND_PACKAGE_FORWARD_TO_NATIVE _hfc_prov_package_forward_to_native
    OUT_VERSION _hfc_prov_targetcache_version
  )

  if("${_hfc_prov_package_found}" AND "${_hfc_prov_package_forward_to_native}" AND (NOT "${FORCE_SYSTEM_${_hfc_prov_package_name}}"))
    hfc_log_debug(" - forwarding to native find_package() with HFC install root: ${_hfc_prov_targetcache_install_prefix}")
    set(${_hfc_prov_package_name}_ROOT "${_hfc_prov_targetcache_install_prefix}")
    set(${_hfc_prov_package_name_uppercase}_ROOT "${_hfc_prov_targetcache_install_prefix}")

    # Save, override, call, restore _HFC_FIND_SCOPE_VARS above.
    # CMAKE_FIND_ROOT_PATH_MODE_* --> BOTH: search sysroot AND normal paths.
    # CMAKE_FIND_USE_*            --> FALSE: only <Pkg>_ROOT is searched
    _hfc_find_scope_save()
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
    set(CMAKE_FIND_USE_CMAKE_PATH FALSE)
    set(CMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH FALSE)
    set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
    set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH FALSE)
    set(CMAKE_FIND_USE_PACKAGE_REGISTRY FALSE)
    set(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY FALSE)
    find_package(${_hfc_prov_package_name} ${ARGN} BYPASS_PROVIDER)
    _hfc_find_scope_restore()

    # reflect back the <pkg>_FOUND status to _hfc_prov_package_found so downstream code doesn't
    # override with the semantically slightly different meaning of _hfc_prov_package_found (which
    # if forward_to_native is requested means that this is a registered dependency not that it was
    # ever found by cmake
    set(_hfc_prov_package_found ${${_hfc_prov_package_name}_FOUND})

  elseif("${_hfc_prov_package_found}" AND (NOT "${FORCE_SYSTEM_${_hfc_prov_package_name}}"))
    hfc_log_debug(" - loading target cache from ${_hfc_prov_targetscache_file}")

    hfc_targets_cache_consume(
      ${_hfc_prov_package_name}
      TARGETS_CACHE_FILE "${_hfc_prov_targetscache_file}"
      TARGET_INSTALL_PREFIX "${_hfc_prov_targetcache_install_prefix}"
      HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING ON
      OUT_IMPORTED_LIBRARIES _hfc_prov_imported_libraries
      OUT_LIBRARY_BYPRODUCTS _hfc_prov_library_byproducts
    )

    hfc_log_debug(" - emulating found package: ${_hfc_prov_package_name}")
    set(${_hfc_prov_package_name_uppercase}_ROOT_DIR "${_hfc_prov_targetcache_install_prefix}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${_hfc_prov_targetcache_install_prefix}")

    hfc_log_debug(" - found targets: ${_hfc_prov_imported_libraries}")

    #
    # emulating findpackage
    set(${_hfc_prov_package_name_uppercase}_ROOT_DIR "${_hfc_prov_targetcache_install_prefix}")

    set(_hfc_prov_package_INCLUDE_DIRS "")
    set(_hfc_prov_package_LIBRARIES "")

    foreach(_hfc_prov_target IN LISTS _hfc_prov_imported_libraries)

      # include dirs
      get_target_property(_hfc_prov_tgt_include_dirs ${_hfc_prov_target} INTERFACE_INCLUDE_DIRECTORIES)
      if(_hfc_prov_tgt_include_dirs)
        list(APPEND _hfc_prov_package_INCLUDE_DIRS ${_hfc_prov_tgt_include_dirs})
      endif()

      # link libraries
      get_target_property(_hfc_prov_tgt_location ${_hfc_prov_target} LOCATION)
      if(_hfc_prov_tgt_location)
        list(APPEND _hfc_prov_package_LIBRARIES ${_hfc_prov_tgt_location})
      endif()
    endforeach()

    #
    # note for the below: it is technically required by some findPackage modules
    # to have both singular and plural variables availabe if exactly *one*
    # LIBRARY / INCLUDE_DIR is exported

    list(REMOVE_DUPLICATES _hfc_prov_package_INCLUDE_DIRS)
    list(LENGTH _hfc_prov_package_INCLUDE_DIRS _hfc_prov_include_dirs_len)

    if(_hfc_prov_include_dirs_len EQUAL 1)
      set(${_hfc_prov_package_name_uppercase}_INCLUDE_DIR "${_hfc_prov_package_INCLUDE_DIRS}")
    endif()

    if(_hfc_prov_include_dirs_len GREATER_EQUAL 1)
      set(${_hfc_prov_package_name_uppercase}_INCLUDE_DIRS "${_hfc_prov_package_INCLUDE_DIRS}")
    endif()

    list(REMOVE_DUPLICATES _hfc_prov_package_LIBRARIES)
    list(LENGTH _hfc_prov_package_LIBRARIES _hfc_prov_libraries_len)

    if(_hfc_prov_libraries_len EQUAL 1)
      set(${_hfc_prov_package_name_uppercase}_LIBRARY "${_hfc_prov_package_LIBRARIES}")
    endif()

    if(_hfc_prov_libraries_len GREATER_EQUAL 1)
      set(${_hfc_prov_package_name_uppercase}_LIBRARIES "${_hfc_prov_package_LIBRARIES}")
    endif()

    if(_hfc_prov_targetcache_version)
      set(${_hfc_prov_package_name}_VERSION "${_hfc_prov_targetcache_version}")
      set(${_hfc_prov_package_name_uppercase}_VERSION "${_hfc_prov_targetcache_version}")
    endif()

  else()
    cmake_policy(PUSH)
    cmake_policy(SET CMP0057 NEW) # make sure we have IN_LIST

    if(${_hfc_prov_package_name} IN_LIST HERMETIC_FETCHCONTENT_BYPASS_PROVIDER_FOR_PACKAGES OR FORCE_SYSTEM_${_hfc_prov_package_name})
      hfc_log_debug(" - forwarding request to native find_package()")
      # Save, override, call, restore — see _HFC_FIND_SCOPE_VARS above.
      # CMAKE_FIND_ROOT_PATH_MODE_* -->  BOTH: search both sysroot and normal paths
      #   so that system/FORCE_SYSTEM packages are reachable regardless of what
      #   the toolchain set (e.g. ONLY for cross-compilation).
      # Save, override, call, restore — see _HFC_FIND_SCOPE_VARS above.
      # CMAKE_FIND_ROOT_PATH_MODE_* --> BOTH so system packages are reachable.
      # CMAKE_FIND_USE_*            --> unset so system paths are searched.
      # _HFC_AUTHORIZED_BYPASS_DEPTH is incremented so that any find_package()
      # call made from within the system Find module (e.g. FindOpenSSL calling
      # find_package(Threads)) is recognised as nested inside an authorized
      # system search and allowed through by the "not in list" branch below.
      _hfc_find_scope_save()
      set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
      set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
      set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
      set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
      unset(CMAKE_FIND_USE_CMAKE_PATH)
      unset(CMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH)
      unset(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH)
      unset(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH)
      unset(CMAKE_FIND_USE_PACKAGE_REGISTRY)
      unset(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY)
      if(NOT DEFINED _HFC_AUTHORIZED_BYPASS_DEPTH)
        set(_HFC_AUTHORIZED_BYPASS_DEPTH 0)
      endif()
      math(EXPR _HFC_AUTHORIZED_BYPASS_DEPTH "${_HFC_AUTHORIZED_BYPASS_DEPTH} + 1")
      find_package(${_hfc_prov_package_name} ${ARGN} BYPASS_PROVIDER)
      math(EXPR _HFC_AUTHORIZED_BYPASS_DEPTH "${_HFC_AUTHORIZED_BYPASS_DEPTH} - 1")
      _hfc_find_scope_restore()
      # Reflect the actual result so the final set(${_hfc_prov_content_name}_FOUND ...) is correct
      # and cmake does not re-run its own search after the provider returns.
      set(_hfc_prov_package_found ${${_hfc_prov_package_name}_FOUND})
    else()
      hfc_log_debug(" - not in list of hermetic dependencies")
      # If we are nested inside an authorized bypass call (e.g. FindOpenSSL.cmake
      # calling find_package(Threads)), pass through to the system.
      # At top level (depth == 0) we block so the consumer cannot accidentally
      # pick up unregistered system packages.
      if(NOT (DEFINED _HFC_AUTHORIZED_BYPASS_DEPTH AND _HFC_AUTHORIZED_BYPASS_DEPTH GREATER 0))
        set(CMAKE_FIND_USE_CMAKE_PATH FALSE)
        set(CMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH FALSE)
        set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH FALSE)
        set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH FALSE)
        set(CMAKE_FIND_USE_PACKAGE_REGISTRY FALSE)
        set(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY FALSE)
      endif()
    endif()

    cmake_policy(POP)
  endif()

  set(${_hfc_prov_package_name_uppercase}_FOUND ${_hfc_prov_package_found})
  set(${_hfc_prov_content_name}_FOUND ${_hfc_prov_package_found})

  # evaluate the per package EXTRA_CODE in place
  if(_hfc_prov_package_found AND DEFINED HERMETIC_${_hfc_prov_content_name}_FIND_PACKAGE_EXTRA_CODE)
      # Expose the HFC install prefix to the user-provided EXTRA_CODE under the
      # documented, stable name `<content-name>_HERMETIC_INSTALL_PREFIX`.
      set(${_hfc_prov_content_name}_HERMETIC_INSTALL_PREFIX "${_hfc_prov_targetcache_install_prefix}")
      cmake_language(EVAL CODE "${HERMETIC_${_hfc_prov_content_name}_FIND_PACKAGE_EXTRA_CODE}")
      unset(${_hfc_prov_content_name}_HERMETIC_INSTALL_PREFIX) # macro!! unset to not pollute global scope
  endif()

  # Clean up internal variables to prevent leakage into the caller's scope.
  # This is critical because this is a macro (no scope isolation): any variable
  # we set here persists in the caller and could collide with variables in
  # calling functions (e.g. hfc_targets_cache_consume) or with subsequent
  # invocations of this macro that rely on hfc_targets_cache_get_registered_info
  # only setting output vars on success.
  unset(_hfc_prov_content_name)
  unset(_hfc_prov_package_name)
  unset(_hfc_prov_package_name_uppercase)
  unset(_hfc_prov_package_found)
  unset(_hfc_prov_targetcache_install_prefix)
  unset(_hfc_prov_targetscache_file)
  unset(_hfc_prov_targetcache_version)
  unset(_hfc_prov_package_forward_to_native)
  unset(_hfc_prov_imported_libraries)
  unset(_hfc_prov_library_byproducts)
  unset(_hfc_prov_package_INCLUDE_DIRS)
  unset(_hfc_prov_package_LIBRARIES)
  unset(_hfc_prov_include_dirs_len)
  unset(_hfc_prov_libraries_len)
  unset(_hfc_prov_target)
  unset(_hfc_prov_tgt_include_dirs)
  unset(_hfc_prov_tgt_location)

endmacro()
