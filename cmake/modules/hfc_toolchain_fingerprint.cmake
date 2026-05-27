include_guard(GLOBAL)

# this computes a live SHA256 fingerprint of the active CMake toolchain
# taking into account all influential CMAKE_* cache variables,
# as well as the following parameters and special rules:
# - HFC_ADDITIONAL_TOOLCHAIN_FINGERPRINT_VARIABLES  / global list appended to the additional variables
function(compute_live_toolchain_fingerprint out_var)

    get_cmake_property(all_cache_vars CACHE_VARIABLES)
    set(toolchain_state "")

    # Capture Top-Level Directory Properties that calls to add_compile_options(), include_directories(), etc.
    # as defined if evaluated at the root level by the toolchain.
    # we provide a way to disable this as users might be in a situation where their main CMakeLists.txt
    # has some of those that would polute the capture
    if(NOT HFC_TOOLCHAIN_FINGERPRINT_DISABLE_CAPTURE_TOP_LEVEL_DIR_PROPERTIES)
      get_directory_property(dir_compile_defs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS)
      get_directory_property(dir_compile_opts DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_OPTIONS)
      get_directory_property(dir_link_opts DIRECTORY ${CMAKE_SOURCE_DIR} LINK_OPTIONS)
      get_directory_property(dir_include_dirs DIRECTORY ${CMAKE_SOURCE_DIR} INCLUDE_DIRECTORIES)
      get_directory_property(dir_link_dirs DIRECTORY ${CMAKE_SOURCE_DIR} LINK_DIRECTORIES)

      string(APPEND toolchain_state "TOPLEVEL_DIR_COMPILE_DEFINITIONS=" "${dir_compile_defs}" "\n")
      string(APPEND toolchain_state "TOPLEVEL_DIR_COMPILE_OPTIONS=${dir_compile_opts}\n")
      string(APPEND toolchain_state "TOPLEVEL_DIR_LINK_OPTIONS=${dir_link_opts}\n")
      string(APPEND toolchain_state "TOPLEVEL_DIR_INCLUDE_DIRECTORIES=${dir_include_dirs}\n")
      string(APPEND toolchain_state "TOPLEVEL_DIR_LINK_DIRECTORIES=${dir_link_dirs}\n")
    endif()

    # Sort the list to guarantee deterministic hashing regardless of cache order
    list(SORT all_cache_vars)

    foreach(var_name IN LISTS all_cache_vars)
      if(var_name STREQUAL "CMAKE_MAKE_PROGRAM" OR
         var_name STREQUAL "CMAKE_SIZEOF_VOID_P" OR
         var_name STREQUAL "CMAKE_FRAMEWORK_PATH" OR
         var_name STREQUAL "CMAKE_POSITION_INDEPENDENT_CODE" OR
         var_name MATCHES "^CMAKE_.+_POSTFIX" OR
         var_name MATCHES "^CMAKE_EXECUTABLE_SUFFIX" OR
         var_name MATCHES "^CMAKE_GENERATOR" OR
         var_name MATCHES "^CMAKE_IMPORT_LIBRARY_PREFIX" OR
         var_name MATCHES "^CMAKE_USER_MAKE_RULES_OVERRIDE" OR
         var_name MATCHES "^CMAKE_INTERPROCEDURAL_OPTIMIZATION" OR
         var_name MATCHES "^CMAKE_SYSROOT" OR
         var_name MATCHES "^CMAKE_CROSSCOMPILING_" OR           
         var_name MATCHES "^CMAKE_LINK_" OR
         var_name MATCHES "^CMAKE_RUNTIME_" OR
         var_name MATCHES "^CMAKE_MODULE_" OR
         var_name MATCHES "^CMAKE_INSTALL_" OR
         var_name MATCHES "^CMAKE_(C|CXX|ASM|OBJC|OBJCXX|Fortran|Swift|VS|XCODE|OSX|ANDROID|MSVC|CUDA|HIP|ISPC)_" OR
         var_name MATCHES "^CMAKE_(EXE|SHARED|MODULE|STATIC)_" OR
         var_name MATCHES "^CMAKE_SYSTEM_" OR
         var_name MATCHES "^CMAKE_HOST_" OR
         var_name MATCHES "^CMAKE_BUILD_"
        )            
          get_property(var_value CACHE ${var_name} PROPERTY VALUE)
          string(APPEND toolchain_state "${var_name}=${var_value}\n")
      endif()
    endforeach()

    set(extra_vars ${HFC_ADDITIONAL_TOOLCHAIN_FINGERPRINT_VARIABLES})
    
    if(extra_vars)
        # keep this as deterministic as possible
        list(REMOVE_DUPLICATES extra_vars)
        list(SORT extra_vars)

        foreach(extra_var IN LISTS extra_vars)
            # Check for the literal string format "ENV{VAR_NAME}"
            if(extra_var MATCHES "^ENV\\{(.+)\\}$")
                set(env_name "${CMAKE_MATCH_1}")
                set(env_value "$ENV{${env_name}}")
                string(APPEND toolchain_state "ENV_${env_name}=${env_value}\n")
            else()
                if(DEFINED "${extra_var}")
                    string(APPEND toolchain_state "${extra_var}=${${extra_var}}\n")
                else()
                    # Explicitly track undefined variables to catch if they get defined later (semantically defined != empty)
                    string(APPEND toolchain_state "${extra_var}//NOTFOUND\n")
                endif()
            endif()
        endforeach()
    endif()

    string(SHA256 fingerprint "${toolchain_state}")
    set(${out_var} "${fingerprint}" PARENT_SCOPE)
endfunction()

# this computes a live SHA256 fingerprint of the active CMake toolchain
# taking into account all influential CMAKE_* cache variables,
# as well as the following parameters and special rules:
#
# - out_var  / stores the resulting SHA256 hash in the parent scope
# - ADDITIONAL_VARIABLES  / optional list of extra variables to track
# - HFC_ADDITIONAL_TOOLCHAIN_FINGERPRINT_VARIABLES  / global list appended to the additional variables
# - ENV{<name>}  / special string format to explicitly track environment variables
# - NOTFOUND  / recorded if an additional variable is requested but currently undefined
# - by default relevant Top-Level directory properties are captured as well, this can be disabled by setting HFC_TOOLCHAIN_FINGERPRINT_DISABLE_CAPTURE_TOP_LEVEL_DIR_PROPERTIES=ON
function(compute_augmented_toolchain_fingerprint out_var)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs ADDITIONAL_VARIABLES)
    cmake_parse_arguments(PARSE_ARGV 1 FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")

    get_property(root_toolchain_fingerprint_computed GLOBAL PROPERTY HERMETIC_FETCHCONTENT_BOOTSTRAP_PROJECT_TOOLCHAIN_FINGERPRINT SET)

    if(NOT root_toolchain_fingerprint_computed)
        # just run that code... it will set the global property
        include(hfc_toolchain_fingerprint_bootstrap)
    endif()

    get_property(root_toolchain_fingerprint GLOBAL PROPERTY HERMETIC_FETCHCONTENT_BOOTSTRAP_PROJECT_TOOLCHAIN_FINGERPRINT)
    set(toolchain_state "${root_toolchain_fingerprint}\n")

    set(extra_vars ${FN_ARG_ADDITIONAL_VARIABLES} ${HFC_ADDITIONAL_TOOLCHAIN_FINGERPRINT_VARIABLES} ${HERMETIC_FETCHCONTENT_FORWARDED_CMAKE_VARIABLES})
    
    if(extra_vars)
        # keep this as deterministic as possible
        list(REMOVE_DUPLICATES extra_vars)
        list(SORT extra_vars)

        foreach(extra_var IN LISTS extra_vars)
            # Check for the literal string format "ENV{VAR_NAME}"
            if(extra_var MATCHES "^ENV\\{(.+)\\}$")
                set(env_name "${CMAKE_MATCH_1}")
                set(env_value "$ENV{${env_name}}")
                string(APPEND toolchain_state "ENV_${env_name}=${env_value}\n")
            else()
                if(DEFINED "${extra_var}")
                    string(APPEND toolchain_state "${extra_var}=${${extra_var}}\n")
                else()
                    # Explicitly track undefined variables to catch if they get defined later (semantically defined != empty)
                    string(APPEND toolchain_state "${extra_var}//NOTFOUND\n")
                endif()
            endif()
        endforeach()
    endif()

    string(SHA256 fingerprint "${toolchain_state}")
    set(${out_var} "${fingerprint}" PARENT_SCOPE)
endfunction()
