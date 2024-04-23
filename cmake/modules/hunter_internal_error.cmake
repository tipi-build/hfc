# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.
include_guard()


function(hunter_internal_error)
  message("")
  foreach(print_message ${ARGV})
    message("[hunter ** INTERNAL **] ${print_message}")
  endforeach()
  message("[hunter ** INTERNAL **] [Directory:${CMAKE_CURRENT_LIST_DIR}]")
  message("")
endfunction()
