# Toolchain for the automatic file-flag detection test.
#
# Unlike linux-toolchain-file-hash.cmake (which demonstrates the *manual*
# workaround of hashing a file and injecting the digest as a compile
# definition), this toolchain simply passes a file-bearing compiler flag and
# relies on HFC to detect the file argument and fold its content hash into the
# toolchain fingerprint automatically.
#
# We use "-include <header>" because it is a genuine file-bearing flag accepted
# by both GCC and Clang and is harmless when the header is trivial, so the test
# stays decoupled from compiler choice and sanitizer availability while still
# exercising the real top-level COMPILE_OPTIONS capture + scan path end-to-end.

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

# File-bearing flag, passed verbatim with NO manual hashing. HFC detects the
# "-include <file>" pair, hashes forced_include.h, and folds it into the
# fingerprint. Editing forced_include.h must therefore trigger a rebuild.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/forced_include.h")
  add_compile_options(-include "${CMAKE_CURRENT_LIST_DIR}/forced_include.h")
endif()
