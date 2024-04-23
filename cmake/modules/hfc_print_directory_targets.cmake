include(hfc_log)


# Print all targets of a (sub) directory to the console
# Use like : 
# hfc_print_directory_targets(DIR <project directory> RESULT_LIST <out_variable>)
function(hfc_print_directory_targets)

    # arguments parsing
  set(options "")
  set(oneValueArgs DIR RESULT_LIST)
  set(multiValueArgs "")
  cmake_parse_arguments(FN_ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FN_ARGS_DIR AND FN_ARGS_RESULT_LIST)
    Hermetic_FetchContent_SetFunctionOverride_Enabled(OFF)
    hfc_log(FATAL_ERROR "hfc_print_directory_targets needs DIR and RESULT_LIST parameters to be set")
  endif() 

  set(result "")

  get_property(dir_targets DIRECTORY "${FN_ARGS_DIR}" PROPERTY BUILDSYSTEM_TARGETS)
  foreach(tgt IN LISTS dir_targets)
    list(APPEND ${FN_ARGS_RESULT_LIST} "${tgt}")
  endforeach()

  get_property(subdirectories DIRECTORY "${FN_ARGS_DIR}" PROPERTY SUBDIRECTORIES)
  foreach(subdir IN LISTS subdirectories)
    hfc_print_directory_targets(DIR "${subdir}" RESULT_LIST ${FN_ARGS_RESULT_LIST})
  endforeach()

  set(${FN_ARGS_RESULT_LIST} ${${FN_ARGS_RESULT_LIST}} PARENT_SCOPE)

endfunction()