
include_guard()

# Function that crashes on fatal error if any arguments passed to it is missing
# Use like : 
# cmake_parse_arguments(PARAM "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
#  hfc_required_args(PARAM ${oneValueArgs})
#
function(hfc_required_args prefix)
  list(POP_FRONT ARGV prefix)
  foreach(valueArg IN LISTS ARGV)
    if (NOT DEFINED ${prefix}_${valueArg})
      message(FATAL_ERROR "Missing argument ${valueArg} calling ${CMAKE_CURRENT_FUNCTION}")
    endif()
    if ("${prefix}_${valueArg}" STREQUAL "")
      message(FATAL_ERROR "Argument ${valueArg} is empty calling ${CMAKE_CURRENT_FUNCTION}")
    endif()
  endforeach()
endfunction()