include_guard(GLOBAL)
include(hfc_log)

function(__HermeticFetchContent_GetAliasesForContentVariableName content_name OUT_result)
  set(${OUT_result} "HERMETIC_FETCHCONTENT_${content_name}_ALIASES" PARENT_SCOPE)
endfunction()

function(__HermeticFetchContent_GetAliasVariableName content_name OUT_result)
  set(${OUT_result} "HERMETIC_FETCHCONTENT_${content_name}_IS_ALIAS_OF" PARENT_SCOPE)
endfunction()

#
# Adds content aliases for <content_name> those are forwarded to dependencies recursively
function(HermeticFetchContent_AddContentAliases content_name)

  __HermeticFetchContent_GetAliasesForContentVariableName("${content_name}" state_all_aliases_variable_name)

  set(all_aliases "${${state_all_aliases_variable_name}}")

  foreach(alias_name IN ITEMS ${ARGN})
    __HermeticFetchContent_GetAliasVariableName("${alias_name}" alias_variable_name)

    # $HERMETIC_FETCHCONTENT_${alias_name}_IS_ALIAS_OF=${content_name}
    set(${alias_variable_name} "${content_name}" CACHE INTERNAL "HFC alias for ${content_name} = ${alias_name} for reverse lookup")
    list(APPEND all_aliases "${alias_name}")

  endforeach()

  list(REMOVE_DUPLICATES all_aliases)
  set(${state_all_aliases_variable_name} "${all_aliases}" CACHE INTERNAL "HFC all aliases of ${content_name}")

  #
  set(aliased_contents_all ${HERMETIC_FETCHCONTENT_ALIASED_CONTENTS} ${content_name})
  list(REMOVE_DUPLICATES aliased_contents_all)
  set(HERMETIC_FETCHCONTENT_ALIASED_CONTENTS "${aliased_contents_all}" CACHE INTERNAL "HFC all aliased content names")

endfunction()

#
# Resolve the unaliased content name for a given <content_name>
function(HermeticFetchContent_ResolveContentNameAlias content_name OUT_result)

  __HermeticFetchContent_GetAliasVariableName("${content_name}" alias_variable_name)

  if(NOT "${${alias_variable_name}}" STREQUAL "")
    set(${OUT_result} "${${alias_variable_name}}" PARENT_SCOPE)
  else()
    set(${OUT_result} "${content_name}" PARENT_SCOPE)
  endif()

endfunction()