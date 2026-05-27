include_guard()

# Generates the template containing the ExternalProject_Add call used to build the dependency registered.
# The template is used by both BUILD_AT_CONFIGURE_TIME ON or OFF.
#
# Usage:
# hfc_generate_external_project(
#
#   EP_TARGETS_DIR
#   TARGET_NAME
#   SOURCE_DIR
#   BINARY_DIR
#   INSTALL_DIR
#   INSTALL_BYPRODUCTS                              # Which artifacts are getting produced by this dependency
#   BUILD_CMD
#   INSTALL_CMD
#   DEPENDENCIES
#   [INSTALL_BYPRODUCTS...]     <byproduct paths>   # for linkable libraries, specify which are the produced binaries/build byproducts
# )
function(hfc_generate_external_project content_name)

  # arguments parsing
  set(options "")
  set(oneValueArgs_required
  )

  set(oneValueArgs_required
    EP_TARGETS_DIR
    TARGET_NAME
    SOURCE_DIR
    BINARY_DIR
    INSTALL_DIR
    BUILD_CMD
    INSTALL_CMD
  )
  set(oneValueArgs
    ${oneValueArgs_required}
    BUILD_IN_SOURCE_TREE
    DEPENDENCIES
  )

  set(multiValueArgs
    INSTALL_BYPRODUCTS
  )

  cmake_parse_arguments(TEMPLATE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  hfc_required_args(TEMPLATE ${oneValueArgs_required})

  file(MAKE_DIRECTORY "${TEMPLATE_BINARY_DIR}")

  # generate the new CMakeLists.txt to a temp file first so we can compare
  set(_ep_cmakelists "${TEMPLATE_EP_TARGETS_DIR}/CMakeLists.txt")
  set(_ep_cmakelists_tmp "${TEMPLATE_EP_TARGETS_DIR}.cmake_tmp")
  configure_file("${HERMETIC_FETCHCONTENT_ROOT_DIR}/templates/externalProject_Add_install.CMakeLists.txt.in" "${_ep_cmakelists_tmp}" @ONLY)

  # only clean and recreate the EP dir when the content actually changed
  # (or when it doesn't exist yet). keeping the dir intact when content is
  # unchanged preserves CMakeLists.txt mtime, which prevents cmake from
  # detecting a "change" and cascading into spurious further re-runs.
  set(_ep_needs_update FALSE)
  if(NOT EXISTS "${_ep_cmakelists}")
    set(_ep_needs_update TRUE)
  else()
    file(SHA256 "${_ep_cmakelists_tmp}" _ep_new_hash)
    file(SHA256 "${_ep_cmakelists}"     _ep_old_hash)
    if(NOT "${_ep_new_hash}" STREQUAL "${_ep_old_hash}")
      set(_ep_needs_update TRUE)
    endif()
  endif()

  if(_ep_needs_update)
    # clean old EP cmake state so it starts fresh with the new configuration
    if(EXISTS "${TEMPLATE_EP_TARGETS_DIR}")
      hfc_log_debug("Clearing out old external project at: ${TEMPLATE_EP_TARGETS_DIR}")
      file(REMOVE_RECURSE "${TEMPLATE_EP_TARGETS_DIR}")
    endif()
    file(MAKE_DIRECTORY "${TEMPLATE_EP_TARGETS_DIR}")
    file(RENAME "${_ep_cmakelists_tmp}" "${_ep_cmakelists}")
  else()
    hfc_log_debug("External project at ${TEMPLATE_EP_TARGETS_DIR} unchanged, preserving directory")
    file(REMOVE "${_ep_cmakelists_tmp}")
  endif()

endfunction()