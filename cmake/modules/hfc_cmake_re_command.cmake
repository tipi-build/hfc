include(hfc_log)
include(hfc_required_args)

# 
# Create a cmake-re command (as list)
# 
# Usage:
# hfc_get_cmake_re_command(
#   <OUT_result>
#   INSTALL_PREFIX <value> 
#   CMAKE_BUILD_TYPE <value>
#   PROJECT_SOURCE_DIR <value>
#   PROJECT_BINARY_DIR <value>
#   TOOLCHAIN_FILE <value> 
#   GENERATOR <value>
#   ORIGIN <value>
#   [ 
#     BUILD <value>
#     [ BUILD_TARGET <target-name> ]
#     [ BUILD_CLEAN_FIRST ]
#   ]
#   [ VERBOSE | VERY_VERBOSE | TRACE ] set verbosity
#   [
#     CACHE_OP_RESTORE | CACHE_OP_POPULATE
#     REVISION <value>
#   ]
#   [ INITIAL_CACHE <value> ]
#   [ CPU_JOBS <value> ]
# )
function(hfc_get_cmake_re_command OUT_result)

  if(NOT CMAKE_RE_PATH)
    hfc_log(FATAL_ERROR "No value for CMAKE_RE_PATH")
  endif()

  # arguments parsing
  set(options VERBOSE VERY_VERBOSE TRACE CACHE_OP_RESTORE CACHE_OP_POPULATE BUILD_CLEAN_FIRST)
  set(oneValueArgs_required ORIGIN)
  set(oneValueArgs ${oneValueArgs_required} BUILD BUILD_TARGET PROJECT_BINARY_DIR INITIAL_CACHE REVISION CPU_JOBS INSTALL_PREFIX CMAKE_BUILD_TYPE PROJECT_SOURCE_DIR  TOOLCHAIN_FILE GENERATOR)
  set(multiValueArgs )
  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  hfc_required_args(FN_ARG ${oneValueArgs_required})

  if(FN_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
        "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
        " ${FN_ARG_UNPARSED_ARGUMENTS}"
    )
  endif()

  set(result_cmd "")

  list(APPEND result_cmd "${CMAKE_RE_PATH}")

  # verbosity
  if(FN_ARG_TRACE)
    list(APPEND result_cmd "-vvv")
  elseif(FN_ARG_VERY_VERBOSE)
    list(APPEND result_cmd "-vv")
  elseif(FN_ARG_VERBOSE)
    list(APPEND result_cmd "-v")
  endif()

  if(FN_ARG_CPU_JOBS)
    list(APPEND result_cmd "-j${FN_ARG_CPU_JOBS}")
  endif()

  if(FN_ARG_BUILD) 
    list(APPEND result_cmd "--build" "${FN_ARG_BUILD}")

    if(FN_ARG_BUILD_TARGET)
      list(APPEND result_cmd "--target" "${FN_ARG_BUILD_TARGET}")
    endif()

    if(FN_ARG_BUILD_CLEAN_FIRST)
      list(APPEND result_cmd "--clean-first")
    endif()
  else()
    if(FN_ARG_BUILD_TARGET OR FN_ARG_BUILD_CLEAN_FIRST)
      hfc_log(FATAL_ERROR "Cannot specify  BUILD_TARGET or BUILD_CLEAN_FIRST without also setting BUILD")
    endif()
  endif()

  # --origin
  list(APPEND result_cmd "--origin" "${FN_ARG_ORIGIN}")

  if(FN_ARG_INSTALL_PREFIX)
    # --install-prefix
    list(APPEND result_cmd "--install-prefix" "${FN_ARG_INSTALL_PREFIX}")
  endif()

  if(FN_ARG_CMAKE_BUILD_TYPE)
    # -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} 
    list(APPEND result_cmd "-DCMAKE_BUILD_TYPE=${FN_ARG_CMAKE_BUILD_TYPE}")
  endif()

  if(FN_ARG_PROJECT_SOURCE_DIR)
    # -S ${project_sources_dir}
    list(APPEND result_cmd "-S" "${FN_ARG_PROJECT_SOURCE_DIR}")
  endif()

  if(FN_ARG_PROJECT_BINARY_DIR)
    # -B ${project_build_dir} 
    list(APPEND result_cmd "-B" "${FN_ARG_PROJECT_BINARY_DIR}")
  endif()

  if(FN_ARG_TOOLCHAIN_FILE)
    # -DCMAKE_TOOLCHAIN_FILE=....
    list(APPEND result_cmd "-DCMAKE_TOOLCHAIN_FILE=${FN_ARG_TOOLCHAIN_FILE}")
  endif()

  # -C / initial cache
  if(FN_ARG_INITIAL_CACHE) 
    list(APPEND result_cmd "-C" "${FN_ARG_INITIAL_CACHE}")
  endif()


  if(FN_ARG_CACHE_OP_RESTORE AND FN_ARG_CACHE_OP_POPULATE) 
    hfc_log(FATAL_ERROR "Cannot specify cache restore and populate operations at the same time")
  endif()
  

  if(FN_ARG_CACHE_OP_RESTORE OR FN_ARG_CACHE_OP_POPULATE) 

    if(NOT FN_ARG_REVISION)
      hfc_log(FATAL_ERROR "CMake RE cache restore and populate operations require a value for the REVISION to be set")
    endif()
    
    list(APPEND result_cmd "--cache")

    if(FN_ARG_CACHE_OP_RESTORE)
      list(APPEND result_cmd "restore")
    else()
      list(APPEND result_cmd "populate")
    endif()

    list(APPEND result_cmd "--revision" "${FN_ARG_REVISION}")

  endif()
  
  set(${OUT_result} "${result_cmd}")
  return(PROPAGATE ${OUT_result})
endfunction()


# 
# Create a cmake-re command (as string)
# 
# Usage:
# hfc_get_cmake_re_command(
#   <OUT_result>
#   INSTALL_PREFIX <value> 
#   CMAKE_BUILD_TYPE <value>
#   PROJECT_SOURCE_DIR <value>
#   PROJECT_BINARY_DIR <value>
#   TOOLCHAIN_FILE <value> 
#   GENERATOR <value>
#   ORIGIN <value>
#   [ 
#     BUILD <value>
#     [ BUILD_TARGET <target-name> ]
#     [ BUILD_CLEAN_FIRST ]
#   ]
#   [ VERBOSE | VERY_VERBOSE | TRACE ] set verbosity
#   [
#     CACHE_OP_RESTORE | CACHE_OP_POPULATE
#     REVISION <value>
#   ]
#   [ INITIAL_CACHE <value> ]
# )
function(hfc_get_cmake_re_command_string OUT_result)
  hfc_get_cmake_re_command(result_cmd ${ARGN})
  list(JOIN result_cmd " " ${OUT_result})
  return(PROPAGATE ${OUT_result})
endfunction()