include(hfc_log)

# Internal use, projects must not call this directly. It is
# intended for use by HFC's FetchContent_Declare() override.
#
# Records the raw declaration arguments of a content in HFC-owned storage,
# mirroring FetchContent's first-declaration-wins semantics. This is what
# frees HFC from reading CMake's private saved-details storage
# (__FetchContent_getSavedDetails).
function(hfc_declared_details_store contentName)
  string(TOLOWER ${contentName} contentNameLower)
  set(propertyName "_HermeticFetchContent_${contentNameLower}_declaredDetails")
  get_property(alreadyDefined GLOBAL PROPERTY ${propertyName} DEFINED)
  if(alreadyDefined)
    return()
  endif()

  set(quotedArgs "")
  foreach(__cmake_item IN LISTS ARGN)
    string(APPEND quotedArgs " [==[${__cmake_item}]==]")
  endforeach()

  # mirror FetchContent_Declare()'s documented FETCHCONTENT_BASE_DIR layout:
  # SOURCE_DIR/BINARY_DIR default to <base>/<name>-(src|build) when the
  # declaration does not provide them (HFC's consumers rely on BINARY_DIR
  # being part of the recorded details)
  cmake_parse_arguments(ARG "" "SOURCE_DIR;BINARY_DIR" "" ${ARGN})
  if(NOT ARG_SOURCE_DIR)
    string(APPEND quotedArgs " [==[SOURCE_DIR]==] [==[${FETCHCONTENT_BASE_DIR}/${contentNameLower}-src]==]")
  endif()
  if(NOT ARG_BINARY_DIR)
    string(APPEND quotedArgs " [==[BINARY_DIR]==] [==[${FETCHCONTENT_BASE_DIR}/${contentNameLower}-build]==]")
  endif()

  define_property(GLOBAL PROPERTY ${propertyName})
  cmake_language(EVAL CODE
    "set_property(GLOBAL PROPERTY ${propertyName} ${quotedArgs})"
  )
endfunction()

# Internal use, projects must not call this directly.
#
# Retrieves the declaration arguments recorded by hfc_declared_details_store().
# Falls back to FetchContent's private saved details for declarations that
# happened before HermeticFetchContent was included (i.e. before HFC's
# FetchContent_Declare() override was installed).
function(hfc_declared_details_get contentName outVar)
  string(TOLOWER ${contentName} contentNameLower)
  set(propertyName "_HermeticFetchContent_${contentNameLower}_declaredDetails")
  get_property(alreadyDefined GLOBAL PROPERTY ${propertyName} DEFINED)
  if(alreadyDefined)
    get_property(propertyValue GLOBAL PROPERTY ${propertyName})
    set(${outVar} "${propertyValue}" PARENT_SCOPE)
    return()
  endif()

  if(COMMAND __FetchContent_getSavedDetails)
    hfc_log_debug("No HFC-recorded declaration for ${contentName}; falling back to FetchContent's saved details (was it declared before including HermeticFetchContent?)")
    __FetchContent_getSavedDetails(${contentName} propertyValue)
    set(${outVar} "${propertyValue}" PARENT_SCOPE)
    return()
  endif()

  message(FATAL_ERROR
    "No declaration details recorded for ${contentName}. "
    "Call FetchContent_Declare(${contentName} ...) after including HermeticFetchContent.")
endfunction()

# Internal use, projects must not call this directly. It is
# intended for use by the FetchContent_Declare() function.
#
# Retrieves details saved for the specified content in an
# earlier call to __FetchContent_declareDetails().
function(hfc_saved_details_get contentName outVar)

  string(TOLOWER ${contentName} contentNameLower)
  set(propertyName "_HermeticFetchContent_${contentNameLower}_savedDetails")
  get_property(alreadyDefined GLOBAL PROPERTY ${propertyName} DEFINED)
  if(NOT alreadyDefined)
    message(FATAL_ERROR "No content details recorded for ${contentName}")
  endif()
  get_property(propertyValue GLOBAL PROPERTY ${propertyName})
  set(${outVar} "${propertyValue}" PARENT_SCOPE)

endfunction()

# Internal use, projects must not call this directly. It is
# intended for use by the HFC function.
#
# Retrieves details saved for the specified content in an
# earlier call to __FetchContent_declareDetails() and
# return NOTFOUND if not found
function(hfc_details_declared contentName outVar)
  string(TOLOWER ${contentName} contentNameLower)
  set(propertyName "_HermeticFetchContent_${contentNameLower}_savedDetails")
  get_property(alreadyDefined GLOBAL PROPERTY ${propertyName} DEFINED)
  set(${outVar} "${alreadyDefined}" PARENT_SCOPE)
endfunction()

#[=======================================================================[.rst:
hfc_saved_details_persist
------------------------------------------------------------------------------------------
This persists all arguments passed to FetchContent to keep track of any reconfiguration
or rebuild from the cached installed tree.

  ``contentName``
  The FetchContent_Declare identifier to persist details for.

  ``saveToFilename```
  The file in which the content details need to be stored.

  **Returns** ``${contentName}_DETAILS_HASH`
  Is an output variable defined by the function, containing the hash of all persisted
  details to determine if rebuilding the dependency is necessary on any change.

#]=======================================================================]
function(hfc_saved_details_persist contentName saveToFilename)
  block(SCOPE_FOR VARIABLES PROPAGATE contentName saveToFilename)
  # TODO: Persist sorted
  hfc_saved_details_get(${contentName} __fetchcontent_arguments)

  set(persistedSavedDetails_content "")
  foreach(__cmake_item IN LISTS __fetchcontent_arguments)
    string(APPEND persistedSavedDetails_content " [==[${__cmake_item}]==]")
  endforeach()

  file(WRITE "${saveToFilename}" "${persistedSavedDetails_content}")
  file(SHA1 "${saveToFilename}" persistedSavedDetails_content_hash)
  set(${contentName}_DETAILS_HASH ${persistedSavedDetails_content_hash})
  return(PROPAGATE ${contentName}_DETAILS_HASH)
  endblock()
endfunction()
