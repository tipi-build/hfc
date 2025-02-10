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
