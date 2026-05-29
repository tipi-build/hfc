include_guard()

include(hfc_log)
include(hfc_genex_eval)

# Configure + generate a nested "flags-resolver" CMake subproject that uses
# file(GENERATE) + $<GENEX_EVAL> + $<TARGET_GENEX_EVAL> to evaluate any
# generator expressions in the toolchain's add_compile_options /
# add_link_options / add_compile_definitions, then read the resolved option
# lists back into PARENT_SCOPE variables.
#
# Needed because hunter_get_build_flags reads directory properties at configure
# time via get_directory_property(), which returns the raw, unresolved genex
# string form — and ./Configure cannot interpret `$<...>` tokens.
#
# Per-language resolution: the resolver creates C and CXX probe targets so
# that $<COMPILE_LANGUAGE:...> and $<LINK_LANGUAGE:...> evaluate in a
# language-tagged context. That means language-conditional toolchain options
# resolve to the correct C or CXX form.
function(hfc_resolve_forwarded_flags)
  set(one_value_params
    PROJECT_NAME
    RESOLVER_ROOT_DIR
    OUT_RESOLVED_COMPILE_OPTIONS_C
    OUT_RESOLVED_COMPILE_OPTIONS_CXX
    OUT_RESOLVED_COMPILE_DEFINITIONS_C
    OUT_RESOLVED_COMPILE_DEFINITIONS_CXX
    OUT_RESOLVED_INCLUDE_DIRECTORIES_C
    OUT_RESOLVED_INCLUDE_DIRECTORIES_CXX
    OUT_RESOLVED_LINK_OPTIONS_C
    OUT_RESOLVED_LINK_OPTIONS_CXX
    OUT_RESOLVED_LINK_DIRECTORIES_C
    OUT_RESOLVED_LINK_DIRECTORIES_CXX
  )
  cmake_parse_arguments(FN "" "${one_value_params}" "" ${ARGN})

  set(resolver_src "${FN_RESOLVER_ROOT_DIR}")
  set(resolver_bin "${FN_RESOLVER_ROOT_DIR}/build")
  file(MAKE_DIRECTORY "${resolver_src}")
  file(MAKE_DIRECTORY "${resolver_bin}")

  set(PROJECT_NAME "${FN_PROJECT_NAME}")
  configure_file(
    "${HERMETIC_FETCHCONTENT_ROOT_DIR}/templates/CMakeLists_flags_resolver.txt.in"
    "${resolver_src}/CMakeLists.txt"
    @ONLY
  )

  execute_process(
    COMMAND
      "${CMAKE_COMMAND}"
      "-G" "${CMAKE_GENERATOR}"
      "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
      "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
      "-S" "${resolver_src}"
      "-B" "${resolver_bin}"
    RESULT_VARIABLE resolver_rv
    OUTPUT_VARIABLE resolver_out
    ERROR_VARIABLE  resolver_err
  )
  if(NOT resolver_rv EQUAL 0)
    hfc_log(FATAL_ERROR
      "flags-resolver subproject failed (rv=${resolver_rv}).\n"
      "stdout:\n${resolver_out}\n"
      "stderr:\n${resolver_err}"
    )
  endif()

  set(_lang_outputs
    COMPILE_DEFINITIONS_C     compile_definitions_c
    COMPILE_DEFINITIONS_CXX   compile_definitions_cxx
    COMPILE_OPTIONS_C         compile_options_c
    COMPILE_OPTIONS_CXX       compile_options_cxx
    INCLUDE_DIRECTORIES_C     include_directories_c
    INCLUDE_DIRECTORIES_CXX   include_directories_cxx
    LINK_OPTIONS_C            link_options_c
    LINK_OPTIONS_CXX          link_options_cxx
    LINK_DIRECTORIES_C        link_directories_c
    LINK_DIRECTORIES_CXX      link_directories_cxx
  )
  while(_lang_outputs)
    list(POP_FRONT _lang_outputs _suffix _stem)
    set(_out_var "FN_OUT_RESOLVED_${_suffix}")
    if(NOT DEFINED ${_out_var})
      continue()
    endif()
    hfc_genex_eval_read_resolved("${resolver_bin}" "resolved_${_stem}" _resolved)
    set(${${_out_var}} "${_resolved}" PARENT_SCOPE)
  endwhile()
endfunction()
