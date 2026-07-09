include_guard(GLOBAL)
include(hfc_log)

#
# Adds content aliases for <content_name>; those are forwarded to dependencies recursively.
# Alias state is stored in global properties (non-persistent across cmake runs) so that
# stale values from a previous configure never bleed into the current run.
function(HermeticFetchContent_AddContentAliases content_name)

  get_property(all_aliases GLOBAL PROPERTY "HERMETIC_FETCHCONTENT_${content_name}_ALIASES")

  foreach(alias_name IN ITEMS ${ARGN})
    # reverse-lookup: HERMETIC_FETCHCONTENT_${alias_name}_IS_ALIAS_OF = ${content_name}
    set_property(GLOBAL PROPERTY "HERMETIC_FETCHCONTENT_${alias_name}_IS_ALIAS_OF" "${content_name}")
    list(APPEND all_aliases "${alias_name}")
  endforeach()

  list(REMOVE_DUPLICATES all_aliases)
  set_property(GLOBAL PROPERTY "HERMETIC_FETCHCONTENT_${content_name}_ALIASES" "${all_aliases}")

  get_property(aliased_contents_all GLOBAL PROPERTY HERMETIC_FETCHCONTENT_ALIASED_CONTENTS)
  list(APPEND aliased_contents_all "${content_name}")
  list(REMOVE_DUPLICATES aliased_contents_all)
  set_property(GLOBAL PROPERTY HERMETIC_FETCHCONTENT_ALIASED_CONTENTS "${aliased_contents_all}")

endfunction()

#
# Resolve the unaliased content name for a given <content_name>
function(HermeticFetchContent_ResolveContentNameAlias content_name OUT_result)

  get_property(canonical GLOBAL PROPERTY "HERMETIC_FETCHCONTENT_${content_name}_IS_ALIAS_OF")

  if(NOT "${canonical}" STREQUAL "")
    set(${OUT_result} "${canonical}" PARENT_SCOPE)
  else()
    set(${OUT_result} "${content_name}" PARENT_SCOPE)
  endif()

endfunction()