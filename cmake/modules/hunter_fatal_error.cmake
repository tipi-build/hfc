# Copyright (c) 2013, Ruslan Baratov
# All rights reserved.

include(CMakeParseArguments) # cmake_parse_arguments

include(hunter_internal_error)

function(hunter_fatal_error)
  cmake_parse_arguments(x "" "ERROR_PAGE" "" "${ARGV}")
  if(NOT x_ERROR_PAGE)
    hunter_internal_error("Expected argument ERROR_PAGE")
  endif()
  message("")
  foreach(x ${x_UNPARSED_ARGUMENTS})
    message("[hunter ** FATAL ERROR **] ${x}")
  endforeach()
  message("[hunter ** FATAL ERROR **] [Directory:${CMAKE_CURRENT_LIST_DIR}]")
  message("")
endfunction()
