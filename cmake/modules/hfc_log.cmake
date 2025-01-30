
include_guard()

# Log debug information if HERMETIC_FETCHCONTENT_LOG_DEBUG is set
function(hfc_log_debug message)

  if(DEFINED HERMETIC_FETCHCONTENT_LOG_DEBUG AND "${HERMETIC_FETCHCONTENT_LOG_DEBUG}" )
    # We set and leverage the environment variable to support enabling the debug log in
    # subprocesses
    set(ENV{HERMETIC_FETCHCONTENT_LOG_DEBUG} "${HERMETIC_FETCHCONTENT_LOG_DEBUG}")
  endif()

  if(DEFINED ENV{HERMETIC_FETCHCONTENT_LOG_DEBUG} AND "$ENV{HERMETIC_FETCHCONTENT_LOG_DEBUG}" )
    string(TIMESTAMP timestamp)
    message(STATUS "[HERMETIC_FETCHCONTENT ${timestamp}] ${message} ${ARGN}")
  endif()

endfunction()

# log informations at supplied LEVEL
function(hfc_log level message)
  message(${level} "[HERMETIC_FETCHCONTENT] ${message} ${ARGN}")
endfunction()
