include_guard()

# An option() replacement with a default value tracked for changes so
# that project options can be overriden by the toolchain or by the
# command line, in such a way that either the command line -Doptions
# always takes precedence, or if applied on options marked NON_OVERIDABLE
# errors out.
#
# The tracking of the default option allows user to get a warning
# whenever the previous default of an user-overriden option changes.
#
# This module exists to permit :
#   * overridable toolchain options
#   * configurable toolchains options
#   * reliable project options
#
# Rationale
# =========
# Before CMake 3.13 if set() was used without CACHE the value would not
# be taken by option(), leading to a reconfiguration changing the toolchain
# prescribed value on the first build.
#
# Also this leads to using CACHE FORCE because otherwise any modification
# of the value in the toolchain file would not be taken in an already
# configured build tree, requiring full rebuilds to test a simple
# toolchain value change.
#
# Finally the new behaviour of option() since CMP0077 is for option() to
# respect scope variables in priority. This makes essentially plain
# option() set in a toolchain file non-overridable on the command line,
# as the cmake -D command line sets CACHE variables.
#
# Usage:
#
#   toolchain_overridable_option(
#       <variable_name>
#       "<cache comment string>"
#       <default value>
#       TYPE [BOOL/PATH/FILEPATH/STRING]
#       [NON_OVERRIDABLE]
#       [TOOLCHAIN_DEFAULT]
#   )
function(toolchain_overridable_option variable_name comment default_value)

  set(options_params "NON_OVERRIDABLE;TOOLCHAIN_DEFAULT")
  set(one_value_params "TYPE")
  set(multi_value_params)
  cmake_parse_arguments(FN_ARG "${options_params}" "${one_value_params}" "${multi_value_params}" ${ARGN})

  if(NOT FN_ARG_TYPE)
      set(set_param_TYPE "BOOL")
  else()
      set(set_param_TYPE "${FN_ARG_TYPE}")
  endif()

  # get last default value
  set(current_variable_value "${${variable_name}}") # this will come from CACHE or previous set() call
  set(non_overridable_variable_marker "_toolchain_non_overridable_option_OVERRIDABLE_${variable_name}")
  set(project_default_variable "_toolchain_overridable_option_project_DEFAULT_VALUE_${variable_name}")
  set(toolchain_default_variable "_toolchain_overridable_option_toolchain_DEFAULT_VALUE_${variable_name}")
  set(toolchain_prev_stored_default_value $CACHE{${toolchain_default_variable}}) # this was the DEFAULT_VALUE saved by TOOLCHAIN_DEFAULT
  set(project_prev_stored_default_value $CACHE{${project_default_variable}}) # this was the DEFAULT_VALUE saved at the last run
  set(set_value FALSE)
  set(update_initial_default FALSE)

  if(NOT DEFINED CACHE{${toolchain_default_variable}} AND NOT DEFINED CACHE{${project_default_variable}})
    # First time we are configuring a build tree for this variable, creating the initial CMakeCache.txt
    set(initial_cmakecache_creation TRUE)
    set(update_initial_default TRUE)
  else()
    set(initial_cmakecache_creation FALSE)
  endif()

  # The user insn't trying to override via -D${variable_name}=... or via a setting in a parent project
  if (NOT DEFINED ${variable_name})
    set(set_value TRUE)
  else()

    # If the option was set by a prior call marked NON_OVERRIDABLE just do nothing and return early.
    if (NOT FN_ARG_NON_OVERRIDABLE AND DEFINED CACHE{${non_overridable_variable_marker}} AND "$CACHE{${non_overridable_variable_marker}}")
      return()
    endif()

    # If the user is trying to override a value marked as non-overridable we should return an error...
    if(FN_ARG_NON_OVERRIDABLE OR (DEFINED CACHE{${non_overridable_variable_marker}} AND "$CACHE{${non_overridable_variable_marker}}"))

      # ...unless the user is using the same default_value
      if(current_variable_value STREQUAL default_value)
        set(set_value TRUE)
      else()
        message(FATAL_ERROR "🔴 ${variable_name} is a non-overridable default. Please reconfigure without setting this variable or undefining the CMakeCache entry with -U${variable_name}, it will be set to '${default_value}'.")
      endif()
    endif()

  endif()

  # Toolchain default changed and it was not overriden previously, update.
  if(DEFINED CACHE{${toolchain_default_variable}} AND FN_ARG_TOOLCHAIN_DEFAULT AND (NOT default_value STREQUAL toolchain_prev_stored_default_value))
    # and it was not overriden previously, update.
    if(current_variable_value STREQUAL toolchain_prev_stored_default_value OR NOT DEFINED ${variable_name})
      set(update_initial_default TRUE)
      set(set_value TRUE)
    elseif(DEFINED ${variable_name} AND (NOT ${variable_name} STREQUAL toolchain_prev_stored_default_value)) # User did override it in the past
      message(WARNING "️❗️${variable_name} toolchain default changed, since the first time this option was introduced to this build tree. Currently configured with -D${variable_name}=${current_variable_value}. Consider reverting to default with -U${variable_name}")
    endif()
  endif()

  # Project default change, and there is no toolchain default and it was not overriden previously, update.
  if(DEFINED CACHE{${project_default_variable}} AND (NOT DEFINED CACHE{${toolchain_default_variable}}) AND (NOT default_value STREQUAL project_prev_stored_default_value))
    #  and it was not overriden previously, update.
    if(current_variable_value STREQUAL project_prev_stored_default_value OR NOT DEFINED ${variable_name})
      set(update_initial_default TRUE)
      set(set_value TRUE)
    elseif(DEFINED ${variable_name} AND (NOT ${variable_name} STREQUAL project_prev_stored_default_value)) # User did override it in the past
      message(WARNING "️❗️${variable_name} project default value changed, since the first time this option was introduced to this build tree.  It is configured with -D${variable_name}=${current_variable_value}. Consider reverting to default with -U${variable_name}")
    endif()
  endif()


  # Write all decisions

  if(update_initial_default)
    if (FN_ARG_TOOLCHAIN_DEFAULT)
      set(${toolchain_default_variable} "${default_value}" CACHE INTERNAL "Keep track of the default value for ${variable_name}" FORCE)
    else()
      set(${project_default_variable} "${default_value}" CACHE INTERNAL "Keep track of the default value for ${variable_name}" FORCE)
    endif()

    set(${non_overridable_variable_marker} ${FN_ARG_NON_OVERRIDABLE} CACHE INTERNAL "Keep track of the default value for ${variable_name}" FORCE)
  endif()

  if(set_value)
    set(${variable_name} "${default_value}" CACHE ${set_param_TYPE} "${comment}" FORCE)
    set(${variable_name} "${default_value}" PARENT_SCOPE)
  endif()

endfunction()
