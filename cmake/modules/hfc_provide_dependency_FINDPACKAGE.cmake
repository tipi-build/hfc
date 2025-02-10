include(hfc_log)
include(hfc_targets_cache_common)
include(hfc_targets_cache_consume)
include(hfc_targets_cache_alias)


macro(hfc_provide_dependency_FINDPACKAGE method content_name)
  hfc_log_debug("Received find_package() request for package name ${content_name}")
  HermeticFetchContent_ResolveContentNameAlias(${content_name} package_name) # resolve alias is available...

  if(NOT package_name STREQUAL content_name)
    hfc_log_debug(" - resolved to alias ${package_name}")
  endif()

  string(TOUPPER "${package_name}" package_name_uppercase)

  hfc_targets_cache_get_registered_info(
    ${package_name}
    OUT_FOUND package_found
    OUT_TARGETS_INSTALL_PREFIX targetcache_install_prefix
    OUT_TARGETS_CACHE_FILE targetscache_file
  )

  if("${package_found}" AND (NOT "${FORCE_SYSTEM_${package_name}}"))
    hfc_log_debug(" - loading target cache from ${targetscache_file}")

    hfc_targets_cache_consume(
      ${package_name}
      TARGETS_CACHE_FILE "${targetscache_file}"
      TARGET_INSTALL_PREFIX "${targetcache_install_prefix}"
      HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING ON
      OUT_IMPORTED_LIBRARIES imported_libraries
      OUT_LIBRARY_BYPRODUCTS library_byproducts
    )

    hfc_log_debug(" - emulating found package: ${package_name}")
    set(${package_name_uppername}_ROOT_DIR "${targetcache_install_prefix}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${targetcache_install_prefix}")

    hfc_log_debug(" - found targets: ${imported_libraries}")

    #
    # emulating findpackage
    set(${package_name_uppercase}_ROOT_DIR "${targetcache_install_prefix}")

    set(package_INCLUDE_DIRS "")
    set(package_LIBRARIES "")

    foreach(target IN LISTS imported_libraries)

      # include dirs
      get_target_property(tgt_include_dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
      if(tgt_include_dirs)
        list(APPEND package_INCLUDE_DIRS ${tgt_include_dirs})
      endif()

      # link libraries
      get_target_property(tgt_location ${target} LOCATION)
      if(tgt_location)
        list(APPEND package_LIBRARIES ${tgt_location})
      endif()
    endforeach()

    #
    # note for the below: it is technically required by some findPackage modules
    # to have both singular and plural variables availabe if exactly *one*
    # LIBRARY / INCLUDE_DIR is exported

    list(REMOVE_DUPLICATES package_INCLUDE_DIRS)
    list(LENGTH package_INCLUDE_DIRS include_dirs_len)

    if(include_dirs_len EQUAL 1)
      set(${package_name_uppercase}_INCLUDE_DIR "${package_INCLUDE_DIRS}")
    endif()

    if(include_dirs_len GREATER_EQUAL 1)
      set(${package_name_uppercase}_INCLUDE_DIRS "${package_INCLUDE_DIRS}")
    endif()

    list(REMOVE_DUPLICATES package_LIBRARIES)
    list(LENGTH package_LIBRARIES libraries_len)

    if(libraries_len EQUAL 1)
      set(${package_name_uppercase}_LIBRARY "${package_LIBRARIES}")
    endif()

    if(libraries_len GREATER_EQUAL 1)
      set(${package_name_uppercase}_LIBRARIES "${package_LIBRARIES}")
    endif()

  else()
    cmake_policy(PUSH)
    cmake_policy(SET CMP0057 NEW) # make sure we have IN_LIST

    if(${package_name} IN_LIST HERMETIC_FETCHCONTENT_BYPASS_PROVIDER_FOR_PACKAGES OR FORCE_SYSTEM_${package_name})
      hfc_log_debug(" - forwarding request to native find_package()")
      set (BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ${CMAKE_FIND_ROOT_PATH_MODE_PROGRAM})
      set (BACKUP_CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ${CMAKE_FIND_ROOT_PATH_MODE_LIBRARY})
      set (BACKUP_CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ${CMAKE_FIND_ROOT_PATH_MODE_INCLUDE})
      set (BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ${CMAKE_FIND_ROOT_PATH_MODE_PACKAGE})
      set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
      set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
      set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
      set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
      find_package(${package_name} ${ARGN} BYPASS_PROVIDER)
      # Get back to isolated sysroot
      set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ${BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PROGRAM})
      set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ${BACKUP_CMAKE_FIND_ROOT_PATH_MODE_LIBRARY})
      set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ${BACKUP_CMAKE_FIND_ROOT_PATH_MODE_INCLUDE})
      set (CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ${BACKUP_CMAKE_FIND_ROOT_PATH_MODE_PACKAGE})
    else()
      hfc_log_debug(" - not in list of hermetic dependencies")
    endif()

    cmake_policy(POP)
  endif()

  set(${package_name_uppercase}_FOUND ${package_found})

endmacro()
