# Just a very simple
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# lines of code that test may activate selectively
#<toolchain_activate_CMAKE_C_COMPILER>set(CMAKE_C_COMPILER "{toolchain_placeholder_CMAKE_C_COMPILER}" {toolchain_placeholder_CMAKE_C_COMPILER_additional_params})

# this is a trick used to configure HFC in the tests to find the bootstrapped goldilock
# if the HFC_TEST_SHARED_TOOLS_DIR is provied by ctest / test runner via environment us it
if(DEFINED ENV{HFC_TEST_SHARED_TOOLS_DIR}) 
  message(STATUS "Found environment value for HFC_TEST_SHARED_TOOLS_DIR=$ENV{HFC_TEST_SHARED_TOOLS_DIR} configuring hermeticFetchContent to use this information")
  set(HERMETIC_FETCHCONTENT_TOOLS_DIR "$ENV{HFC_TEST_SHARED_TOOLS_DIR}")
endif()

# for CMake-RE builds, we are shipping the goldilock as part of the toolchain / environment zip and we detect if that exists
# this too comprises a small hack which is due to the executables loosing their +x flag in the process of the toolchain 
# generalization
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.hfc_tools_dir")
  message(STATUS "Found .hfc_tools_dir in toolchains folder - configuring hermeticFetchContent to use this information")
  set(HERMETIC_FETCHCONTENT_TOOLS_DIR "${CMAKE_CURRENT_LIST_DIR}/.hfc_tools_dir")

  # make sure that goldilock is executable
  file(GLOB_RECURSE goldilock_executables "${HERMETIC_FETCHCONTENT_TOOLS_DIR}/*/goldilock")
  message(STATUS "Making the following files executable: ${goldilock_executables}")
  file(CHMOD ${goldilock_executables} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)  
endif()