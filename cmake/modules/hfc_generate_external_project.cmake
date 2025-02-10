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

  # clear old files / if we're here we will be rebuilding that stuff anyway
  if(EXISTS "${TEMPLATE_EP_TARGETS_DIR}")
    hfc_log_debug("Clearing out old external project at: ${TEMPLATE_EP_TARGETS_DIR}")
    file(REMOVE_RECURSE "${TEMPLATE_EP_TARGETS_DIR}")
  endif()

  file(MAKE_DIRECTORY "${TEMPLATE_BINARY_DIR}")
  file(MAKE_DIRECTORY "${TEMPLATE_EP_TARGETS_DIR}")

  # generate the actual external project
  configure_file("${HERMETIC_FETCHCONTENT_ROOT_DIR}/templates/externalProject_Add_install.CMakeLists.txt.in" "${TEMPLATE_EP_TARGETS_DIR}/CMakeLists.txt" @ONLY)

endfunction()