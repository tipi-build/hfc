#
# Using dependency built executables in the project build
#

cmake_minimum_required(VERSION 3.27.6)
project(
  ModernCMakeExample
  VERSION 1.0
  LANGUAGES CXX)


set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  ${CMAKE_MODULE_PATH}
)

# negative test: should not be found
find_program(tool hello)

if(tool_FOUND)
  message(FATAL_ERROR "For some reason hello is already on the PATH but it shouldn't!")
endif() 


include(FetchContent)
include(HermeticFetchContent)


FetchContent_Declare(
  Iconv
  GIT_REPOSITORY https://github.com/tipi-build/unittest-autotools-sample.git
  GIT_TAG ad80b024eeda8f4c0a96eedf669dc453ed33a094 
)

# the MAKE_EXECUTABLES_FINDABLE parameter is not required to be present
# test if it defaults to the correct result (i.e. not findable)
if(DEFINED TEST_INPUT_MAKE_EXECUTABLE_FINDABLE)
    
  set(expecting_to_find_tool "${TEST_INPUT_MAKE_EXECUTABLE_FINDABLE}")
  set(mk_hermetic_param "MAKE_EXECUTABLES_FINDABLE")
  set(mk_hermetic_param_value "${TEST_INPUT_MAKE_EXECUTABLE_FINDABLE}")
  
else()

  set(expecting_to_find_tool FALSE)
  set(mk_hermetic_param "")
  set(mk_hermetic_param_value "")

endif()

FetchContent_MakeHermetic(
  Iconv
  HERMETIC_BUILD_SYSTEM autotools
  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION 
     [=[
       add_library(Iconv::Iconv STATIC IMPORTED)
       set_property(TARGET Iconv::Iconv PROPERTY IMPORTED_LOCATION "@HFC_PREFIX_PLACEHOLDER@/lib/libiconv.a")    
       set_property(TARGET Iconv::Iconv PROPERTY INTERFACE_INCLUDE_DIRECTORIES @HFC_PREFIX_PLACEHOLDER@/include)

       add_executable(Iconv::hello IMPORTED)
       set_property(TARGET Iconv::hello PROPERTY IMPORTED_LOCATION "@HFC_PREFIX_PLACEHOLDER@/bin/hello")
     ]=]
  HERMETIC_TOOLCHAIN_EXTENSION 
    [=[
        add_compile_definitions(
          TIPI_TEAM=1
          TIPI_TEAM_ZURICH
          # TIPI_TEAM_LOCATION=ZURICH
        )
    ]=]
  ${mk_hermetic_param} ${mk_hermetic_param_value}
)



HermeticFetchContent_MakeAvailableAtConfigureTime(Iconv)

if(expecting_to_find_tool)

  if(NOT CMAKE_PROGRAM_PATH) 
    message(FATAL_ERROR "Expected CMAKE_PROGRAM_PATH to not be empty!")
  endif()

  find_program(tool hello REQUIRED)

else()

  # negative test: still not be able to find it hopefully
  find_program(tool hello)

  if(tool_FOUND)
    message(FATAL_ERROR "For some reason dep-as-tool is already on the PATH but it shouldn't!")
  endif()

endif()

