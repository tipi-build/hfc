
include_guard()

set(HERMETIC_FETCHCONTENT_LOG_DEBUG ON)

# Log debug information if HERMETIC_FETCHCONTENT_LOG_DEBUG is set
function(hfc_log_debug message)

  if(HERMETIC_FETCHCONTENT_LOG_DEBUG)
    string(TIMESTAMP timestamp)
    message(STATUS "[HERMETIC_FETCHCONTENT ${timestamp}] ${message} ${ARGN}")
  endif()

endfunction()

# log informations at supplied LEVEL
function(hfc_log level message)
  message(${level} "[HERMETIC_FETCHCONTENT] ${message} ${ARGN}")
endfunction()
