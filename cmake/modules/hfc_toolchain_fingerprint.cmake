include_guard(GLOBAL)
include(${CMAKE_CURRENT_LIST_DIR}/hfc_log.cmake)

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

# computes the augmented toolchain fingerprint in an isolated cmake mini-project
# so that add_compile_definitions(), add_compile_options() etc. in the toolchain
# are captured correctly regardless of the calling cmake version's scoping behaviour.
#
# HERMETIC_FETCHCONTENT_FORWARDED_CMAKE_VARIABLES are injected into the mini-project
# as cmake set() calls (preserving cache type) placed before project(), mirroring
# the proxy toolchain behaviour, so the toolchain can read them during execution.
# When no TOOLCHAIN_FILE is provided the mini-project runs without one.
#
# within a single configure run results are deduped via a global property so
# multiple dependencies sharing the same toolchain only pay the cost once.
#
# - out_var              / stores the resulting SHA256 hash in the parent scope
# - TOOLCHAIN_FILE       / path to the project toolchain file (optional)
# - ADDITIONAL_VARIABLES / optional extra variable names to track (same semantics
#                          as compute_augmented_toolchain_fingerprint)
# - HFC_TOOLCHAIN_FINGERPRINT_LANGUAGES controls languages enabled in the mini-project:
#   "root_project" (default) mirrors ENABLED_LANGUAGES from the calling project,
#   "none" uses LANGUAGES NONE, any other value is forwarded as-is (e.g. "CXX")
function(compute_augmented_toolchain_fingerprint_isolated out_var)
    set(options "")
    set(oneValueArgs TOOLCHAIN_FILE)
    set(multiValueArgs ADDITIONAL_VARIABLES)
    cmake_parse_arguments(PARSE_ARGV 1 FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")

    # resolve the language list for the mini-project
    if(NOT HFC_TOOLCHAIN_FINGERPRINT_LANGUAGES OR HFC_TOOLCHAIN_FINGERPRINT_LANGUAGES STREQUAL "root_project")
        get_property(fp_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
        # ENABLED_LANGUAGES is empty when project() used LANGUAGES NONE; guard against
        # the literal string "NONE" appearing in the list to avoid invalid language names
        list(REMOVE_ITEM fp_languages "NONE")
        if(NOT fp_languages)
            set(fp_languages "NONE")
        endif()
    elseif(HFC_TOOLCHAIN_FINGERPRINT_LANGUAGES STREQUAL "none")
        set(fp_languages "NONE")
    else()
        set(fp_languages "${HFC_TOOLCHAIN_FINGERPRINT_LANGUAGES}")
    endif()

    list(SORT fp_languages)
    list(JOIN fp_languages ";" TEMPLATE_ENABLE_LANGUAGES)

    # deduplication key for within-run caching: toolchain content + build type + languages.
    # a global property is used deliberately (no disk persistence) so that out-of-band
    # state referenced by the toolchain (e.g. a file hash read inside it) is always
    # re-evaluated on the next configure run.
    if(FN_ARG_TOOLCHAIN_FILE)
        file(SHA256 "${FN_ARG_TOOLCHAIN_FILE}" toolchain_sha256)
    else()
        set(toolchain_sha256 "no-toolchain")
    endif()
    string(SHA256 dedup_key "${toolchain_sha256}:${CMAKE_BUILD_TYPE}:${TEMPLATE_ENABLE_LANGUAGES}")
    set(dedup_property "HERMETIC_FETCHCONTENT_TOOLCHAIN_FINGERPRINT_${dedup_key}")

    get_property(live_fingerprint_cached GLOBAL PROPERTY "${dedup_property}" SET)
    if(live_fingerprint_cached)
        get_property(live_fingerprint GLOBAL PROPERTY "${dedup_property}")
    else()
        # generate forwarded variable declarations to inject before project() in the
        # mini-project, mirroring how the proxy toolchain forwards them (preserving
        # cache type) so the toolchain can reference these values during its execution
        set(FORWARDED_CMAKE_VARIABLES_CONTENT "")
        foreach(cmake_variable IN LISTS HERMETIC_FETCHCONTENT_FORWARDED_CMAKE_VARIABLES)
            if(DEFINED ${cmake_variable})
                set(additional_set_args "")
                get_property(var_type_in_cache CACHE ${cmake_variable} PROPERTY TYPE)
                if(var_type_in_cache)
                    set(additional_set_args "CACHE ${var_type_in_cache} \"\" FORCE")
                endif()
                string(APPEND FORWARDED_CMAKE_VARIABLES_CONTENT
                    "set(${cmake_variable} \"${${cmake_variable}}\" ${additional_set_args})\n"
                )
            endif()
        endforeach()

        string(RANDOM LENGTH 10 rand_str)
        set(tmp_dir "${FETCHCONTENT_BASE_DIR}/.hfc_fingerprint_tmp_${rand_str}")
        file(MAKE_DIRECTORY "${tmp_dir}")

        set(FINGERPRINT_OUTPUT_FILE "${tmp_dir}/fingerprint.txt")

        configure_file(
            "${HERMETIC_FETCHCONTENT_ROOT_DIR}/templates/compute_toolchain_fingerprint.CMakeLists.txt.in"
            "${tmp_dir}/CMakeLists.txt"
            @ONLY
        )

        set(fp_cmake_args "." "-G" "${CMAKE_GENERATOR}" "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
        if(FN_ARG_TOOLCHAIN_FILE)
            list(APPEND fp_cmake_args "-DCMAKE_TOOLCHAIN_FILE=${FN_ARG_TOOLCHAIN_FILE}")
        endif()

        hfc_log(STATUS "Fingerprinting CMAKE_TOOLCHAIN_FILE: ${FN_ARG_TOOLCHAIN_FILE} [fingerprint key ${dedup_key}]")
        hfc_log_debug(" - command: ${CMAKE_COMMAND} ${fp_cmake_args}")

        execute_process(
            COMMAND ${CMAKE_COMMAND} ${fp_cmake_args}
            WORKING_DIRECTORY "${tmp_dir}"
            RESULT_VARIABLE fp_result
            OUTPUT_VARIABLE fp_output
            ERROR_VARIABLE  fp_error
        )

        if(NOT fp_result EQUAL 0)
            message(FATAL_ERROR "compute_augmented_toolchain_fingerprint_isolated: isolated cmake process failed\n--- stdout ---\n${fp_output}\n--- stderr ---\n${fp_error}")
        endif()

        if(NOT EXISTS "${FINGERPRINT_OUTPUT_FILE}")
            message(FATAL_ERROR "compute_augmented_toolchain_fingerprint_isolated: fingerprint output file not produced by isolated process")
        endif()

        file(READ "${FINGERPRINT_OUTPUT_FILE}" live_fingerprint)
        string(STRIP "${live_fingerprint}" live_fingerprint)

        set_property(GLOBAL PROPERTY "${dedup_property}" "${live_fingerprint}")

        file(REMOVE_RECURSE "${tmp_dir}")
    endif()

    hfc_log_debug(" - isolated toolchain fingerprint: ${live_fingerprint}")

    set(toolchain_state "${live_fingerprint}\n")

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
    hfc_log(STATUS " - toolchain fingerprint: ${fingerprint}")
    set(${out_var} "${fingerprint}" PARENT_SCOPE)
endfunction()
