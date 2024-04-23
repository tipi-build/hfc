# HermeticFetchContent / targets cache common
# 

include(hfc_log)
include(hfc_custom_echo_command)


#
# Consumes a cache file and creates libraries
# 
# Usage:
# hfc_targets_cache_consume(
#   TARGETS_CACHE_FILE <library cache file>       # library cache file
#   TARGET_INSTALL_PREFIX <target install prefix> # path to the installed/ tree
#   TARGET_SOURCE_DIR 
#   MAKE_EXECUTABLES_FINDABLE <ON/OFF> set to truth-y value to add the found executables to the find_package search set
#   HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING              # flag parameter to specificy if the consumed target should be added to the data used to fill the hfc_list_dependencies_install_dirs custom target
# )
function(hfc_targets_cache_consume content_name)

  # arguments parsing
  set(options)
  set(oneValueArgs TARGETS_CACHE_FILE TARGET_INSTALL_PREFIX TARGET_SOURCE_DIR TARGET_BINARY_DIR MAKE_EXECUTABLES_FINDABLE OUT_IMPORTED_LIBRARIES OUT_LIBRARY_BYPRODUCTS HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING)
  set(multiValueArgs )
  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  
  if(NOT FN_ARG_TARGETS_CACHE_FILE)
    Hermetic_FetchContent_SetFunctionOverride_Enabled(OFF)
    hfc_log(FATAL_ERROR "hfc_targets_cache_consume needs a value for TARGETS_CACHE_FILE parameter to be defined")
  endif()

  if(NOT FN_ARG_TARGET_INSTALL_PREFIX)
    Hermetic_FetchContent_SetFunctionOverride_Enabled(OFF)
    hfc_log(FATAL_ERROR "hfc_targets_cache_consume needs a value for TARGET_INSTALL_PREFIX parameter to be defined")
  endif()

  #
  # read the cache
  hfc_log_debug("Reading targets cache information from ${FN_ARG_TARGETS_CACHE_FILE}")
  include("${FN_ARG_TARGETS_CACHE_FILE}")

  set(out_byproducts "")
  set(processed_targets "")

  if ((NOT "${FN_ARG_HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING}") AND (NOT "${FORCE_SYSTEM_${content_name}}"))
  
    #
    # custom commands for "list locations for this content"
    hfc_custom_echo_command_create(hfc_list_${content_name}_STATIC_LIBRARY_locations_cmd  "Dependency: ${content_name}\n===STATIC_LIBRARIES_locations===")
    hfc_custom_echo_command_create(hfc_list_${content_name}_SHARED_LIBRARY_locations_cmd  "Dependency: ${content_name}\n===SHARED_LIBRARIES_locations===")
    hfc_custom_echo_command_create(hfc_list_${content_name}_MODULE_LIBRARY_locations_cmd  "Dependency: ${content_name}\n===MODULE_LIBRARIES_locations===")
    hfc_custom_echo_command_create(hfc_list_${content_name}_UNKNOWN_LIBRARY_locations_cmd "Dependency: ${content_name}\n===UNKNOWN_LIBRARIES_locations===")
    hfc_custom_echo_command_create(hfc_list_${content_name}_EXECUTABLE_locations_cmd      "Dependency: ${content_name}\n===EXECUTABLE_LIBRARIES_locations===")
  endif()

  foreach(target_name IN LISTS ${HERMETIC_FETCHCONTENT_TARGET_LIST_NAME})

    Hermetic_FetchContent_TargetsCache_get_found_property_variable_name("${target_name}" found_properties_var_name)

    hfc_log_debug("Processing target '${target_name}'")

    set(visited_target_properties "NAME;IMPORTED_GLOBAL") # ignoring bc basically read-only    
    list(APPEND visited_target_properties "TYPE" "IMPORTED") 

    #
    # get all properties required for lib decl
    Hermetic_FetchContent_TargetsCache_getExportVariable(
      TARGET_NAME "${target_name}" 
      PROPERTY_NAME "TYPE" 
      EXPORT_VARIABLE_PREFIX "${HERMETIC_FETCHCONTENT_TARGET_VARIABLE_PREFIX}"
      RESULT target_TYPE
    )

    Hermetic_FetchContent_TargetsCache_getExportVariable(
      TARGET_NAME "${target_name}" 
      PROPERTY_NAME "IMPORTED" 
      EXPORT_VARIABLE_PREFIX "${HERMETIC_FETCHCONTENT_TARGET_VARIABLE_PREFIX}"
      RESULT target_IMPORTED
    )

    if(${target_IMPORTED})
      set(add_library_ARG_IMPORTED "IMPORTED")
    else()
      set(add_library_ARG_IMPORTED "")
    endif()

    hfc_log_debug(" x - TYPE = '${target_TYPE}'")

    if(TARGET "${target_name}")
      hfc_log_debug("Target '${target_name}' already declared proceeding anyway")

    elseif("${target_TYPE}" STREQUAL "STATIC_LIBRARY")
      add_library("${target_name}" ${add_library_ARG_IMPORTED} STATIC GLOBAL)
    
    elseif("${target_TYPE}" STREQUAL "SHARED_LIBRARY")
      add_library("${target_name}" ${add_library_ARG_IMPORTED} SHARED GLOBAL)
    
    elseif("${target_TYPE}" STREQUAL "MODULE_LIBRARY")
      add_library("${target_name}" ${add_library_ARG_IMPORTED} MODULE GLOBAL)

    elseif("${target_TYPE}" STREQUAL "UNKNOWN_LIBRARY")
      add_library("${target_name}" ${add_library_ARG_IMPORTED} UNKNOWN GLOBAL)

    elseif("${target_TYPE}" STREQUAL "INTERFACE_LIBRARY")
      add_library("${target_name}" ${add_library_ARG_IMPORTED} INTERFACE GLOBAL)

    elseif("${target_TYPE}" STREQUAL "EXECUTABLE")
      add_executable("${target_name}" ${add_library_ARG_IMPORTED} GLOBAL)

    else()
      Hermetic_FetchContent_SetFunctionOverride_Enabled(OFF)
      hfc_log(FATAL_ERROR "hfc_targets_cache_consume does not support library imports of type ${target_TYPE}")
    endif()


    list(APPEND processed_targets ${target_name})

    # only set IMPORTED_LOCATION for the object/archive/so/DLL and EXECUTABLE, skip the other *_LOCATIONS properties
    if("${target_TYPE}" MATCHES "((STATIC|SHARED|MODULE|UNKNOWN)_LIBRARY)|(EXECUTABLE)")
      list(APPEND visited_target_properties "LOCATION" "LOCATION_<CONFIG>" "IMPORTED_LOCATION")

      Hermetic_FetchContent_TargetsCache_getExportVariable(
        TARGET_NAME "${target_name}" 
        PROPERTY_NAME "LOCATION" 
        EXPORT_VARIABLE_PREFIX "${HERMETIC_FETCHCONTENT_TARGET_VARIABLE_PREFIX}"
        RESULT target_LOCATION
      )

      string(
        REPLACE 
        "${HERMETIC_FETCHCONTENT_CONST_PREFIX_PLACEHOLDER}" 
        "${FN_ARG_TARGET_INSTALL_PREFIX}"
        target_LOCATION_real
        "${target_LOCATION}"
      )

      hfc_log_debug(" * - IMPORTED_LOCATION raw = ${target_LOCATION}")
      hfc_log_debug(" * - IMPORTED_LOCATION = ${target_LOCATION_real}")
      set_property(TARGET "${target_name}" PROPERTY IMPORTED_LOCATION ${target_LOCATION_real})
      list(APPEND out_byproducts "${target_LOCATION_real}")

      if ((NOT "${FN_ARG_HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING}") AND (NOT "${FORCE_SYSTEM_${content_name}}"))
        hfc_custom_echo_command_append(hfc_list_${content_name}_${target_TYPE}_locations_cmd "${target_LOCATION_real}")
      endif()


      # take care of find_packaging transitive depedencies if required
      Hermetic_FetchContent_TargetsCache_getExportVariable(
        TARGET_NAME "${target_name}" 
        PROPERTY_NAME "INTERFACE_LINK_LIBRARIES" 
        EXPORT_VARIABLE_PREFIX "${HERMETIC_FETCHCONTENT_TARGET_VARIABLE_PREFIX}"
        RESULT target_INTERFACE_LINK_LIBRARIES
      )

      foreach(link_lib IN LISTS target_INTERFACE_LINK_LIBRARIES)

        if(link_lib IN_LIST ${HERMETIC_FETCHCONTENT_TARGET_LIST_NAME})
          hfc_log_debug("   - skipping transitive target search for '${link_lib}' as it is a locally declared target")
          continue()
        endif()

        if(link_lib MATCHES "^-.+")
          hfc_log_debug("   - skipping transitive target search for '${link_lib}' as it a linker flag")
          continue()
        endif()

        if(link_lib MATCHES "^\\\$<.+>$")
          hfc_log_debug("   - skipping transitive target search for generator expression '${link_lib}'")
          continue()
        endif()


        if(NOT TARGET ${link_lib})

          # 
          # extract information from generator expressions like '$<LINK_ONLY:OpenSSL::Crypto>'
          #                                                                  ^^^^^^^^^^^^^^^ 
          string(REGEX MATCH "^\\$<.+:(.+::.+)>$" match_generator ${link_lib})

          if(match_generator AND CMAKE_MATCH_1) # CMAKE_MATCH_n magic - the group matching the marked library above is in group 1 (group 0 == whole match)
              set(link_lib "${CMAKE_MATCH_1}")
          endif()

          if(link_lib MATCHES "::")
            string(REPLACE "::" ";" link_lib_parts "${link_lib}")
            list(GET link_lib_parts 0 link_lib_package_name)

            if(link_lib_package_name) 
              # try our luck with find_package()
              hfc_log_debug("   - trying to find_package() '${link_lib_package_name}'")
              find_package(${link_lib_package_name} QUIET)
            endif()
          endif()

        endif()


        if(TARGET ${link_lib})
          hfc_log_debug("   - success found transitive target '${link_lib}'")
        else()
          hfc_log_debug("   - failed to find transitive target '${link_lib}' - proceeding anyway")
        endif()
        
      endforeach()

    endif()

    # set the remaining target properties
    foreach(property_name IN LISTS ${found_properties_var_name})

      if("${property_name}" IN_LIST visited_target_properties)
        continue()
      else()
        Hermetic_FetchContent_TargetsCache_getExportVariable(
          TARGET_NAME "${target_name}" 
          PROPERTY_NAME "${property_name}" 
          EXPORT_VARIABLE_PREFIX "${HERMETIC_FETCHCONTENT_TARGET_VARIABLE_PREFIX}"
          RESULT property_value
        )

        hfc_log_debug(" P - ${property_name} raw = ${property_value}")

        string(
          REPLACE 
          "${HERMETIC_FETCHCONTENT_CONST_PREFIX_PLACEHOLDER}" 
          "${FN_ARG_TARGET_INSTALL_PREFIX}"
          property_value
          "${property_value}"
        )
        hfc_log_debug(" P - ${property_name} = ${property_value}")
        set_property(TARGET "${target_name}" PROPERTY "${property_name}" "${property_value}")
      endif()

    endforeach()

    # if it's an executable we might have to add it the the cmake find_program search path
    if(FN_ARG_MAKE_EXECUTABLES_FINDABLE AND ("${target_TYPE}" STREQUAL "EXECUTABLE"))

      get_property(executable_location TARGET "${target_name}" PROPERTY IMPORTED_LOCATION)
      hfc_log_debug(" E - adding executable ${executable_location} to CMAKE_PROGRAM_PATH")

      cmake_path(GET executable_location PARENT_PATH executable_location_dir)
      set(new_CMAKE_PROGRAM_PATH "${CMAKE_PROGRAM_PATH}")
      LIST(APPEND new_CMAKE_PROGRAM_PATH "${executable_location_dir}")
      list(REMOVE_DUPLICATES new_CMAKE_PROGRAM_PATH)

      # set this globally
      set(CMAKE_PROGRAM_PATH "${new_CMAKE_PROGRAM_PATH}" CACHE INTERNAL "CMAKE_PROGRAM_PATH")

    endif()
    
  endforeach()


  if(FN_ARG_OUT_IMPORTED_LIBRARIES)
    set(${FN_ARG_OUT_IMPORTED_LIBRARIES} "${processed_targets}" PARENT_SCOPE)
  endif()

  if(FN_ARG_OUT_LIBRARY_BYPRODUCTS)
    list(REMOVE_DUPLICATES out_byproducts) # required because of our alias definitions, so multiple targets might reference the same byproduct
    set(${FN_ARG_OUT_LIBRARY_BYPRODUCTS} "${out_byproducts}" PARENT_SCOPE)
  endif()

  # store global state info 
  set(all_consumed_target_contents ${HERMETIC_FETCHCONTENT_TARGETS_CACHE_CONSUMED_CONTENTS} ${content_name})
  list(REMOVE_DUPLICATES all_consumed_target_contents)
  set(HERMETIC_FETCHCONTENT_TARGETS_CACHE_CONSUMED_CONTENTS ${all_consumed_target_contents} CACHE INTERNAL "Cache target files consumed by Hermetic_FetchContent")
  hfc_targets_cache_register_dependency_for_provider(
    ${content_name} 
    TARGETS_INSTALL_PREFIX "${FN_ARG_TARGET_INSTALL_PREFIX}"
    TARGETS_CACHE_FILE "${FN_ARG_TARGETS_CACHE_FILE}"
  )

  # don't output a path if we are using the system provided $content
  if ((NOT "${FN_ARG_HERMETIC_SKIP_REGISTER_TARGET_FOR_LISTING}") AND (NOT "${FORCE_SYSTEM_${content_name}}"))

    set(all_public_consumed_target_contents ${HERMETIC_FETCHCONTENT_TARGETS_CACHE_PUBLIC_CONTENTS} ${content_name})
    list(REMOVE_DUPLICATES all_public_consumed_target_contents)
    set(HERMETIC_FETCHCONTENT_TARGETS_CACHE_PUBLIC_CONTENTS ${all_public_consumed_target_contents} CACHE INTERNAL "PUBLIC marked Cache target files consumed by Hermetic_FetchContent")
    
    hfc_custom_echo_command_append("hfc_list_dependencies_install_dirs_echo_cmd" "${FN_ARG_TARGET_INSTALL_PREFIX}")
    hfc_custom_echo_command_append("hfc_list_dependencies_target_cache_files_echo_cmd" "${FN_ARG_TARGETS_CACHE_FILE}")

    ##
    add_custom_target(hfc_list_${content_name}_STATIC_LIBRARIES_locations   DEPENDS hfc_list_${content_name}_STATIC_LIBRARY_locations_cmd)
    add_custom_target(hfc_list_${content_name}_SHARED_LIBRARIES_locations   DEPENDS hfc_list_${content_name}_SHARED_LIBRARY_locations_cmd)
    add_custom_target(hfc_list_${content_name}_MODULE_LIBRARIES_locations   DEPENDS hfc_list_${content_name}_MODULE_LIBRARY_locations_cmd)
    add_custom_target(hfc_list_${content_name}_UNKNOWN_LIBRARIES_locations  DEPENDS hfc_list_${content_name}_UNKNOWN_LIBRARY_locations_cmd)
    add_custom_target(hfc_list_${content_name}_EXECUTABLES_locations        DEPENDS hfc_list_${content_name}_EXECUTABLE_locations_cmd)
  endif()

endfunction()