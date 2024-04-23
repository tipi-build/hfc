include(hfc_saved_details)

#[=======================================================================[.rst:
hfc_determine_cache_id
------------------------------------------------------------------------------------------
For a given FetchContent delared it returns the CMake RE cache id based on sources
origin information.

  ``contentName``
  The FetchContent_Declare identifier to retrieve the cacheId for.

  **Returns** ``${contentName}_origin`` and  ``${contentName}_revision``
  The returned information can be used to address CMake-RE cache.

#]=======================================================================]
function(hfc_determine_cache_id contentName)
  block(SCOPE_FOR VARIABLES PROPAGATE contentName)
    
    __FetchContent_getSavedDetails(${content_name} __fetchcontent_arguments)  
    cmake_parse_arguments(fetchContentArg "" "URL" "" ${__fetchcontent_arguments})
    cmake_parse_arguments(fetchContentArg "" "URL_HASH" "" ${__fetchcontent_arguments})
    cmake_parse_arguments(fetchContentArg "" "GIT_REPOSITORY" "" ${__fetchcontent_arguments})
    cmake_parse_arguments(fetchContentArg "" "GIT_TAG" "" ${__fetchcontent_arguments})

    set (${contentName}_origin "")
    set (${contentName}_revision "")
    if (NOT fetchContentArg_URL)
      set (${contentName}_origin "${fetchContentArg_GIT_REPOSITORY}")
      set (${contentName}_revision "${fetchContentArg_GIT_TAG}")
    else()
      set (${contentName}_origin "${fetchContentArg_URL}")
      string(FIND "${fetchContentArg_URL_HASH}" "=" EQUAL_SIGN_POSITION)
      MATH(EXPR EQUAL_SIGN_POSITION "${EQUAL_SIGN_POSITION}+1")
      string(SUBSTRING "${fetchContentArg_URL_HASH}" ${EQUAL_SIGN_POSITION} -1 HASH)
      set (${contentName}_revision "${HASH}")
    endif()

    return(PROPAGATE ${contentName}_origin ${contentName}_revision)
  endblock()
endfunction()