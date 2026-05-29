include_guard(GLOBAL)

# Read the generated output file(s) for a given base name from a resolver/
# fingerprint binary directory and return their content as a CMake list.
#
# Per-language files (base_c.txt, base_cxx.txt) are tried first -- these are
# produced when a probe target is available (see hfc_genex_eval_produce_flag_files
# and hfc_genex_eval_produce_evals_file).  If none exist, base.txt is used as a
# fallback (probe-less evaluation or language-suffixed stems from the flags
# resolver, e.g. "resolved_compile_definitions_c").
#
# This is the single shared reader used by both hfc_resolve_forwarded_flags()
# and compute_augmented_toolchain_fingerprint_isolated().
function(hfc_genex_eval_read_resolved dir base out_var)
    set(_result "")
    foreach(_lang c cxx)
        set(_f "${dir}/${base}_${_lang}.txt")
        if(EXISTS "${_f}")
            file(STRINGS "${_f}" _lines)
            list(APPEND _result ${_lines})
        endif()
    endforeach()
    # Fallback: exact file (flags-resolver stems already encode the language,
    # e.g. "resolved_compile_definitions_c"; also handles probe-less output).
    if(NOT _result)
        set(_f "${dir}/${base}.txt")
        if(EXISTS "${_f}")
            file(STRINGS "${_f}" _lines)
            set(_result ${_lines})
        endif()
    endif()
    set(${out_var} "${_result}" PARENT_SCOPE)
endfunction()


# Capture all directory-level flag properties for the current cmake project
# into parent-scope variables named _hfc_genex_content_<prop>.
#
# Edge cases handled here so callers don't have to repeat them:
#   - $<LINK_LANGUAGE:...> is substituted with $<COMPILE_LANGUAGE:...> because
#     file(GENERATE) cannot evaluate $<LINK_LANGUAGE:...> without a link target.
#   - Each property list is split on ";" -> "\n" so individual tokens survive
#     file(GENERATE) round-trips without being confused with list separators.
#   - COMPILE_DEFINITIONS_<CONFIG> is captured alongside COMPILE_DEFINITIONS.
function(hfc_genex_eval_capture_dir_properties config_name)
    string(TOUPPER "${config_name}" _config_upper)

    get_directory_property(_defs     DIRECTORY "${CMAKE_SOURCE_DIR}" COMPILE_DEFINITIONS)
    get_directory_property(_defs_cfg DIRECTORY "${CMAKE_SOURCE_DIR}" COMPILE_DEFINITIONS_${_config_upper})
    get_directory_property(_opts     DIRECTORY "${CMAKE_SOURCE_DIR}" COMPILE_OPTIONS)
    get_directory_property(_incdirs  DIRECTORY "${CMAKE_SOURCE_DIR}" INCLUDE_DIRECTORIES)
    get_directory_property(_linkopts DIRECTORY "${CMAKE_SOURCE_DIR}" LINK_OPTIONS)
    get_directory_property(_linkdirs DIRECTORY "${CMAKE_SOURCE_DIR}" LINK_DIRECTORIES)

    string(REPLACE "$<LINK_LANGUAGE" "$<COMPILE_LANGUAGE" _linkopts "${_linkopts}")
    string(REPLACE "$<LINK_LANGUAGE" "$<COMPILE_LANGUAGE" _linkdirs "${_linkdirs}")

    string(REPLACE ";" "\n" _defs     "${_defs}")
    string(REPLACE ";" "\n" _defs_cfg "${_defs_cfg}")
    string(REPLACE ";" "\n" _opts     "${_opts}")
    string(REPLACE ";" "\n" _incdirs  "${_incdirs}")
    string(REPLACE ";" "\n" _linkopts "${_linkopts}")
    string(REPLACE ";" "\n" _linkdirs "${_linkdirs}")

    set(_hfc_genex_content_compile_defs     "${_defs}"     PARENT_SCOPE)
    set(_hfc_genex_content_compile_defs_cfg "${_defs_cfg}" PARENT_SCOPE)
    set(_hfc_genex_content_compile_opts     "${_opts}"     PARENT_SCOPE)
    set(_hfc_genex_content_include_dirs     "${_incdirs}"  PARENT_SCOPE)
    set(_hfc_genex_content_link_opts        "${_linkopts}" PARENT_SCOPE)
    set(_hfc_genex_content_link_dirs        "${_linkdirs}" PARENT_SCOPE)
endfunction()


# Create the __hfc_probe static library target used to give file(GENERATE) a
# language context so that $<COMPILE_LANGUAGE:...> evaluates correctly.
# Only sources for currently-enabled languages (C, CXX) are included, making
# this safe to call from projects that enable a subset of those languages.
function(hfc_genex_eval_create_probe_target)
    get_property(_langs GLOBAL PROPERTY ENABLED_LANGUAGES)
    set(_sources "")
    if("C" IN_LIST _langs)
        file(WRITE "${CMAKE_BINARY_DIR}/__hfc_probe.c"   "void __hfc_probe_c(void){}\n")
        list(APPEND _sources "${CMAKE_BINARY_DIR}/__hfc_probe.c")
    endif()
    if("CXX" IN_LIST _langs)
        file(WRITE "${CMAKE_BINARY_DIR}/__hfc_probe.cpp" "void __hfc_probe_cxx(){}\n")
        list(APPEND _sources "${CMAKE_BINARY_DIR}/__hfc_probe.cpp")
    endif()
    if(_sources)
        add_library(__hfc_probe STATIC EXCLUDE_FROM_ALL ${_sources})
    endif()
endfunction()


# Produce per-language resolved_<prop>_<lang>.txt files under output_dir.
# Requires hfc_genex_eval_capture_dir_properties() and
# hfc_genex_eval_create_probe_target() to have been called first in the same
# cmake scope.  These are the files consumed by hfc_resolve_forwarded_flags().
function(hfc_genex_eval_produce_flag_files output_dir)
    file(GENERATE
        OUTPUT  "${output_dir}/resolved_compile_definitions_$<LOWER_CASE:$<COMPILE_LANGUAGE>>.txt"
        CONTENT "${_hfc_genex_content_compile_defs}\n${_hfc_genex_content_compile_defs_cfg}\n"
        TARGET  __hfc_probe
    )
    file(GENERATE
        OUTPUT  "${output_dir}/resolved_compile_options_$<LOWER_CASE:$<COMPILE_LANGUAGE>>.txt"
        CONTENT "${_hfc_genex_content_compile_opts}\n"
        TARGET  __hfc_probe
    )
    file(GENERATE
        OUTPUT  "${output_dir}/resolved_include_directories_$<LOWER_CASE:$<COMPILE_LANGUAGE>>.txt"
        CONTENT "${_hfc_genex_content_include_dirs}\n"
        TARGET  __hfc_probe
    )
    file(GENERATE
        OUTPUT  "${output_dir}/resolved_link_options_$<LOWER_CASE:$<COMPILE_LANGUAGE>>.txt"
        CONTENT "${_hfc_genex_content_link_opts}\n"
        TARGET  __hfc_probe
    )
    file(GENERATE
        OUTPUT  "${output_dir}/resolved_link_directories_$<LOWER_CASE:$<COMPILE_LANGUAGE>>.txt"
        CONTENT "${_hfc_genex_content_link_dirs}\n"
        TARGET  __hfc_probe
    )
endfunction()


# Produce generation-time evaluated output files for toolchain fingerprinting.
# Output paths: output_base_<lang>.txt when the __hfc_probe target exists
# (one file per enabled language), or output_base.txt otherwise.
#
# Requires hfc_genex_eval_capture_dir_properties() to have been called first.
# hfc_genex_eval_create_probe_target() should be called too when possible so
# that $<COMPILE_LANGUAGE:...> conditionals evaluate correctly per-language.
#
# ADDITIONAL_GENEX_EXPRESSIONS may contain raw GENEX strings to evaluate
# alongside the directory properties -- typically values of forwarded cache
# variables or HFC_ADDITIONAL_TOOLCHAIN_FINGERPRINT_VARIABLES that are
# themselves generator expressions.
function(hfc_genex_eval_produce_evals_file output_base)
    cmake_parse_arguments(FN "" "" "ADDITIONAL_GENEX_EXPRESSIONS" ${ARGN})

    set(_content
        "${_hfc_genex_content_compile_defs}\n${_hfc_genex_content_compile_defs_cfg}\n"
        "${_hfc_genex_content_compile_opts}\n"
        "${_hfc_genex_content_include_dirs}\n"
        "${_hfc_genex_content_link_opts}\n"
        "${_hfc_genex_content_link_dirs}\n"
    )
    list(JOIN _content "" _content)

    if(FN_ADDITIONAL_GENEX_EXPRESSIONS)
        list(REMOVE_DUPLICATES FN_ADDITIONAL_GENEX_EXPRESSIONS)
        foreach(_expr IN LISTS FN_ADDITIONAL_GENEX_EXPRESSIONS)
            string(REPLACE "$<LINK_LANGUAGE" "$<COMPILE_LANGUAGE" _expr "${_expr}")
            string(APPEND _content "${_expr}\n")
        endforeach()
    endif()

    if(TARGET __hfc_probe)
        file(GENERATE
            OUTPUT  "${output_base}_$<LOWER_CASE:$<COMPILE_LANGUAGE>>.txt"
            CONTENT "${_content}"
            TARGET  __hfc_probe
        )
    else()
        file(GENERATE
            OUTPUT  "${output_base}.txt"
            CONTENT "${_content}"
        )
    endif()
endfunction()
