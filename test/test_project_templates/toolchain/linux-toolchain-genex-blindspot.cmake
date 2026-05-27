
# Toolchain for the GENEX blind-spot test.
#
# Contains two generator expressions to exercise both paths in
# compute_live_toolchain_fingerprint's GENEX evaluation:
#
#   1. $<$<BOOL:$CACHE{HFC_TEST_GENEX_FLAG}>:HFC_GENEX_FLAG_ACTIVE>
#      References a non-CMAKE_ cache variable forwarded via
#      HERMETIC_FETCHCONTENT_FORWARDED_CMAKE_VARIABLES.  The GENEX string is
#      constant; only generation-time evaluation reveals the flag value.
#
#   2. $<$<CONFIG:Release>:HFC_TEST_RELEASE_MODE>
#      A pure generator expression requiring no forwarded variable.  Always
#      evaluates to HFC_TEST_RELEASE_MODE for Release builds, demonstrating
#      that toolchain-local GENEXs are also captured and evaluated.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#<toolchain_activate_CMAKE_C_COMPILER>set(CMAKE_C_COMPILER "{toolchain_placeholder_CMAKE_C_COMPILER}" {toolchain_placeholder_CMAKE_C_COMPILER_additional_params})

if(DEFINED ENV{HFC_TEST_SHARED_TOOLS_DIR})
  message(STATUS "Found environment value for HFC_TEST_SHARED_TOOLS_DIR=$ENV{HFC_TEST_SHARED_TOOLS_DIR} configuring hermeticFetchContent to use this information")
  set(HERMETIC_FETCHCONTENT_TOOLS_DIR "$ENV{HFC_TEST_SHARED_TOOLS_DIR}")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.hfc_tools_dir")
  message(STATUS "Found .hfc_tools_dir in toolchains folder - configuring hermeticFetchContent to use this information")
  set(HERMETIC_FETCHCONTENT_TOOLS_DIR "${CMAKE_CURRENT_LIST_DIR}/.hfc_tools_dir")

  file(GLOB_RECURSE goldilock_executables "${HERMETIC_FETCHCONTENT_TOOLS_DIR}/*/goldilock")
  message(STATUS "Making the following files executable: ${goldilock_executables}")
  file(CHMOD ${goldilock_executables} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

# GENEX 1: depends on a forwarded non-CMAKE_ cache variable.
add_compile_definitions($<$<BOOL:$CACHE{HFC_TEST_GENEX_FLAG}>:HFC_GENEX_FLAG_ACTIVE>)

# GENEX 2: pure config-based expression, no forwarding needed.
add_compile_definitions($<$<CONFIG:Release>:HFC_TEST_RELEASE_MODE>)
