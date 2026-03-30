# Copyright (c) 2017 Ruslan Baratov, Alexandre Pretyman
# All rights reserved.
#
# This function dump build flags.
# Only OUT_* paramaters which are passed will be written too (i.e. optional)
#
# Usage example:
# hunter_get_build_flags(
#   PACKAGE_CONFIGURATION_TYPES
#     "Release"                 # Mandatory ONE build config type
#   INSTALL_DIR
#     "${HUNTER_INSTALL_DIR}"   # Mandatory <root-id> of hunter
#   OUT_CPPFLAGS
#     cppflags                  # set cppflags with preprocessor flags
#   OUT_CFLAGS
#     cflags                    # set cflags with c compilation flags
#   OUT_CXXFLAGS
#     cxxflags                  # set cxxflags with c++ compilation flags
#   OUT_LDFLAGS
#     ldflags                   # set ldflags with linking flags
# )
include_guard()

include(hunter_dump_cmake_flags)
include(hunter_fatal_error)
include(hunter_internal_error)
include(hunter_status_debug)

function(hunter_get_build_flags)
  set(optional_params)
  set(one_value_params
      INSTALL_DIR
      OUT_CPPFLAGS
      OUT_CFLAGS
      OUT_CXXFLAGS
      OUT_LDFLAGS
  )
  set(multi_value_params
      PACKAGE_CONFIGURATION_TYPES
  )
  cmake_parse_arguments(
      PARAM
      "${optional_params}"
      "${one_value_params}"
      "${multi_value_params}"
      ${ARGN}
  )

  if(PARAM_UNPARSED_ARGUMENTS)
    hunter_internal_error(
        "Invalid arguments passed to hunter_get_build_flags"
        " ${PARAM_UNPARSED_ARGUMENTS}"
    )
  endif()

  string(COMPARE NOTEQUAL "${PARAM_INSTALL_DIR}" "" has_install_dir)
  if (NOT has_install_dir)
    hunter_internal_error(
        "hunter_get_build_flags expects INSTALL_DIR, it must be provided"
    )
  endif()

  list(LENGTH PARAM_PACKAGE_CONFIGURATION_TYPES len)
  if(NOT "${len}" EQUAL "1")
    hunter_fatal_error(
        "hunter_get_build_flags expects PACKAGE_CONFIGURATION_TYPES to have exactly 1 value, but has ${len} with elements: ${PARAM_PACKAGE_CONFIGURATION_TYPES}"
    )
  endif()
  string(TOUPPER ${PARAM_PACKAGE_CONFIGURATION_TYPES} config_type)

  hunter_status_debug(
      "Build flags config ${config_type} on dir ${PARAM_GLOBAL_INSTALLDIR}"
  )
  string(COMPARE NOTEQUAL "${PARAM_OUT_CPPFLAGS}" "" has_out_cppflags)
  string(COMPARE NOTEQUAL "${PARAM_OUT_CFLAGS}"   "" has_out_cflags)
  string(COMPARE NOTEQUAL "${PARAM_OUT_CXXFLAGS}" "" has_out_cxxflags)
  string(COMPARE NOTEQUAL "${PARAM_OUT_LDFLAGS}"  "" has_out_ldflags)

  if(has_out_cppflags)
    # CPPFLAGS=${PARAM_CPPFLAGS} [-D${COMPILE_DEFINITIONS}]
    #          -I${INCLUDE_DIRECTORIES}
    #
    # C Preprocessor flags

    hunter_dump_cmake_flags(CPPFLAGS cppflags)
    set(cppflags "${cppflags} -I${PARAM_INSTALL_DIR}/include")
    string(STRIP "${cppflags}" cppflags)
    # build config type definitions
    get_directory_property(defs
        COMPILE_DEFINITIONS_${config_type}
    )
    foreach(def ${defs})
      set(cppflags "${cppflags} -D${def}")
    endforeach()
    # non-build config specific definitions
    get_directory_property(defs COMPILE_DEFINITIONS)
    foreach(def ${defs})
      set(cppflags "${cppflags} -D${def}")
    endforeach()

    get_directory_property(include_dirs INCLUDE_DIRECTORIES)
    foreach(include_dir ${include_dirs})
      set(cppflags
          "${cppflags} ${CMAKE_INCLUDE_SYSTEM_FLAG_CXX} ${include_dir}"
      )
    endforeach()

    hunter_status_debug("  CPPFLAGS=${cppflags}")
    set(${PARAM_OUT_CPPFLAGS} ${cppflags} PARENT_SCOPE)
  endif()

  if(has_out_cflags)
    # CFLAGS=${cflags} ${CMAKE_C_FLAGS}
    #
    # C Compiler Flags (defines or include directories should not be needed here)
    set(cflags "${CMAKE_C_FLAGS_${config_type}} ${CMAKE_C_FLAGS}")

    # Gather COMPILE_OPTIONS directory property (from add_compile_options)
    # Get from root directory to include toolchain and HERMETIC_TOOLCHAIN_EXTENSION flags
    get_directory_property(compile_options DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_OPTIONS)
    foreach(opt ${compile_options})
      # Handle SHELL: prefix - CMake uses this to indicate the string should be split by spaces
      # Remove the prefix and pass the flag as-is to the shell
      if(opt MATCHES "^SHELL:(.+)$")
        set(opt "${CMAKE_MATCH_1}")
      endif()
      set(cflags "${cflags} ${opt}")
    endforeach()

    # Gather COMPILE_DEFINITIONS directory property (from add_compile_definitions)
    # Get from root directory to include toolchain and HERMETIC_TOOLCHAIN_EXTENSION flags
    get_directory_property(compile_definitions DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS)
    foreach(def ${compile_definitions})
      set(cflags "${cflags} -D${def}")
    endforeach()

    string(STRIP "${cflags}" cflags)
    hunter_status_debug("  CFLAGS=${cflags}")
    set(${PARAM_OUT_CFLAGS} ${cflags} PARENT_SCOPE)
  endif()

  if(has_out_cxxflags)
    # CXXFLAGS=${cxxflags} ${CMAKE_CXX_FLAGS}
    #
    # C++ Compiler flags (defines or include directories should not be needed here)
    set(cxxflags
        "${CMAKE_CXX_FLAGS_${config_type}} ${CMAKE_CXX_FLAGS} ${PARAM_CXXFLAGS}"
    )

    # Gather COMPILE_OPTIONS directory property (from add_compile_options)
    # Get from root directory to include toolchain and HERMETIC_TOOLCHAIN_EXTENSION flags
    get_directory_property(compile_options DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_OPTIONS)
    foreach(opt ${compile_options})
      # Handle SHELL: prefix - CMake uses this to indicate the string should be split by spaces
      # Remove the prefix and pass the flag as-is to the shell
      if(opt MATCHES "^SHELL:(.+)$")
        set(opt "${CMAKE_MATCH_1}")
      endif()
      set(cxxflags "${cxxflags} ${opt}")
    endforeach()

    # Gather COMPILE_DEFINITIONS directory property (from add_compile_definitions)
    # Get from root directory to include toolchain and HERMETIC_TOOLCHAIN_EXTENSION flags
    get_directory_property(compile_definitions DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS)
    foreach(def ${compile_definitions})
      set(cxxflags "${cxxflags} -D${def}")
    endforeach()

    string(STRIP "${cxxflags}" cxxflags)
    hunter_status_debug("  CXXFLAGS=${cxxflags}")
    set(${PARAM_OUT_CXXFLAGS} ${cxxflags} PARENT_SCOPE)
  endif()

  if(has_out_ldflags)
    # LDFLAGS=${ldflags}
    #
    # Linker flags
    set(ldflags "-L${PARAM_INSTALL_DIR}/lib")
    set(ldflags "${ldflags} ${CMAKE_EXE_LINKER_FLAGS_${config_type}}")
    string(STRIP "${ldflags}" ldflags)
    set(ldflags "${ldflags} ${CMAKE_EXE_LINKER_FLAGS}")
    string(STRIP "${ldflags}" ldflags)

    # Gather LINK_OPTIONS directory property (from add_link_options)
    # Get from root directory to include toolchain and HERMETIC_TOOLCHAIN_EXTENSION flags
    get_directory_property(link_options DIRECTORY ${CMAKE_SOURCE_DIR} LINK_OPTIONS)

    # Set up linker wrapper variables with defaults for Unix-like systems
    # Use CMAKE_C_LINKER_WRAPPER_FLAG and CMAKE_C_LINKER_WRAPPER_FLAG_SEP for portability
    # While in theory there could be a difference between the C and CXX flags, we don't
    # have any information about which language the code is compiled in and can't be precise.
    # In practice, the same compiler will probably be used and the options will be identical
    # for either language.
    if(NOT DEFINED CMAKE_C_LINKER_WRAPPER_FLAG)
      set(CMAKE_C_LINKER_WRAPPER_FLAG "-Wl,")
      set(CMAKE_C_LINKER_WRAPPER_FLAG_SEP ",")
    endif()

    # Process CMAKE_C_LINKER_WRAPPER_FLAG according to CMake's convention:
    # For Clang-style, it may be a list like ["-Xlinker", " "] where the trailing " " means
    # arguments should be space-separated. For GCC-style, it's typically "-Wl," with separator ",".
    set(linker_wrapper_flag ${CMAKE_C_LINKER_WRAPPER_FLAG})
    set(linker_wrapper_space "")
    if(linker_wrapper_flag)
      list(GET linker_wrapper_flag -1 last_element)
      if(last_element STREQUAL " ")
        list(REMOVE_AT linker_wrapper_flag -1)
        set(linker_wrapper_space " ")
      endif()
    endif()
    list(JOIN linker_wrapper_flag " " linker_wrapper_prefix)

    foreach(opt ${link_options})
      # Handle LINKER: prefix - convert using CMake's linker wrapper variables
      # CMake documentation: LINKER: is replaced by the wrapper flag, and commas separate arguments
      # Example: "LINKER:-z,defs" becomes "-Wl,-z,defs" for GCC or "-Xlinker -z -Xlinker defs" for Clang
      set(linker_args_list "")
      if(opt MATCHES "^LINKER:SHELL:(.+)$")
        # LINKER:SHELL: uses space-separated arguments
        separate_arguments(linker_args_list UNIX_COMMAND "${CMAKE_MATCH_1}")
      elseif(opt MATCHES "^LINKER:(.+)$")
        # LINKER: uses comma-separated arguments
        string(REPLACE "," ";" linker_args_list "${CMAKE_MATCH_1}")
      endif()

      if(linker_args_list)
        # Convert list of arguments using the appropriate linker wrapper style
        if(CMAKE_C_LINKER_WRAPPER_FLAG_SEP)
          # GCC-style: single prefix with separator-joined arguments (-Wl,-z,relro)
          list(JOIN linker_args_list "${CMAKE_C_LINKER_WRAPPER_FLAG_SEP}" joined_args)
          set(opt "${linker_wrapper_prefix}${linker_wrapper_space}${joined_args}")
        else()
          # Clang-style: repeat prefix for each argument (-Xlinker -z -Xlinker relro)
          set(opt "")
          foreach(arg ${linker_args_list})
            if(opt)
              string(APPEND opt " ")
            endif()
            string(APPEND opt "${linker_wrapper_prefix}${linker_wrapper_space}${arg}")
          endforeach()
        endif()
      endif()
      set(ldflags "${ldflags} ${opt}")
    endforeach()

    string(STRIP "${ldflags}" ldflags)
    string(COMPARE NOTEQUAL "${ANDROID}" "" is_android)
    if(is_android)
        set(ldflags "${ldflags} ${__libstl}")
    endif()
    hunter_status_debug("  LDFLAGS=${ldflags}")
    set(${PARAM_OUT_LDFLAGS} ${ldflags} PARENT_SCOPE)
  endif()
endfunction()

