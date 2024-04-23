# HermeticFetchContent / project hooks
# 
include_guard()
include(hfc_log)
include(hfc_required_args)


# Log debug information if HERMETIC_FETCHCONTENT_LOG_DEBUG is set
function(hfc_project_hooks_load content_name)
    string(TOLOWER ${content_name} lower_case_content_name)

    set(hooks_file "${HERMETIC_FETCHCONTENT_ROOT_DIR}/projects/${lower_case_content_name}/${lower_case_content_name}-hooks.cmake")

    if(EXISTS "${hooks_file}")
      hfc_log_debug("Loading project hooks: ${hooks_file}")
      include(${hooks_file})
    endif()

endfunction()

set(HERMETIC_FETCHCONTENT_KNOWN_HOOKS "")
list(APPEND HERMETIC_FETCHCONTENT_KNOWN_HOOKS "AFTER_SOURCE_POPULATE")
list(APPEND HERMETIC_FETCHCONTENT_KNOWN_HOOKS "AFTER_RESOLVE_FINDPACKAGE")
#list(APPEND HERMETIC_FETCHCONTENT_KNOWN_HOOKS "BEFORE_" "AFTER_")

# log informations at supplied LEVEL
function(hfc_project_hooks_invoke hook content_name)
  string(TOLOWER ${content_name} lower_case_content_name)
  
  if(NOT ${hook} IN_LIST HERMETIC_FETCHCONTENT_KNOWN_HOOKS)
    hfc_log(FATAL_ERROR "Unkown hook name '${hook}'")
  endif()
  
  set(command_name "${lower_case_content_name}_hfc_hook_${hook}")
  if(COMMAND ${command_name})
    # invoke the hook and just forward everything after the named params above
    hfc_log_debug("Invoking ${command_name}")
    cmake_language(CALL ${command_name} ${ARGN})
  else()
  hfc_log_debug("No ${command_name}() available")
  endif()

endfunction()
