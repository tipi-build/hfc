# Extends linux-toolchain.cmake with a hidden file input read at toolchain time.
# The SHA256 of hidden_input.txt is injected as a compile definition so that
# any change to that file is reflected in the toolchain fingerprint and
# triggers a dependency rebuild.

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

# Hidden file input: compute a hash of an external file and expose it as a
# compile definition.  When the file changes the hash changes, which changes
# the toolchain fingerprint and triggers a full dependency rebuild - exactly
# the same pattern as reading a sanitizer ignorelist or any other out-of-code
# build input.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/hidden_input.txt")
  file(SHA256 "${CMAKE_CURRENT_LIST_DIR}/hidden_input.txt" HFC_TEST_HIDDEN_INPUT_HASH)
  message(STATUS "HFC_TEST_HIDDEN_INPUT_HASH: ${HFC_TEST_HIDDEN_INPUT_HASH}")
  add_compile_definitions(HFC_TEST_HIDDEN_INPUT_HASH=${HFC_TEST_HIDDEN_INPUT_HASH})
endif()
