#
# Base hfc targets cache functionality test 
# ensures that packages and properties can be read from package config exports 
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

include(FetchContent)
include(HermeticFetchContent)


if(HFC_TEST_RENAMETARGET) 
  set(mathlib_alias_fn
    [==[
    if("${TARGET_NAME}" STREQUAL "MathFunctionscbrt::MathFunctionscbrt")
      set(TARGET_ALIASES "RenamedMathFunctionscbrt::MathFunctionscbrt") # rename
    endif()
    ]==]
  )
else()
  set(mathlib_alias_fn
    [==[
    if("${TARGET_NAME}" STREQUAL "MathFunctionscbrt::MathFunctionscbrt")
      list(APPEND TARGET_ALIASES "MathFunctionscbrt::MathFunctionscbrt" "AliasedMathFunctionscbrt::MathFunctionscbrt") # alias
    endif()
    ]==]
  )
endif()

FetchContent_Declare(
  mathslib
  GIT_REPOSITORY "https://github.com/tipi-build/unit-test-cmake-template-2libs.git"
  GIT_TAG "ecc756a4c3f1811cdfd637bd6d8f4e3feb6aff92"
)

FetchContent_MakeHermetic(mathslib  
  HERMETIC_CREATE_TARGET_ALIASES "${mathlib_alias_fn}"
)


if(HFC_TEST_FORCE_SYSTEM_RENAME_TARGET)
  set(iconv_alias_fn
    [==[
    if("${TARGET_NAME}" STREQUAL "Iconv::Iconv")
      set(TARGET_ALIASES "RenamedIconv::Iconv") # rename
    endif()
    ]==]
  )
else()
  set(iconv_alias_fn
    [==[
    if("${TARGET_NAME}" STREQUAL "Iconv::Iconv")
      list(APPEND TARGET_ALIASES "Iconv::Iconv" "AliasedIconv::Iconv") # alias
    endif()
    ]==]
  )
endif()

set(FORCE_SYSTEM_Iconv ON)

FetchContent_Declare(
  Iconv
  GIT_REPOSITORY https://github.com/tipi-build/unittest-autotools-sample.git
  GIT_TAG ad80b024eeda8f4c0a96eedf669dc453ed33a094 
)

FetchContent_MakeHermetic(
  Iconv
  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION 
    [=[
      message(FATAL_ERROR "This bit of code should not have been executed because we FORCE_SYSTEM_Iconv")
    ]=]
  HERMETIC_CREATE_TARGET_ALIASES "${iconv_alias_fn}"
  HERMETIC_BUILD_SYSTEM autotools
)

if(HFCTEST_CONFIGURETIME_DEPENDENCY AND HFCTEST_BUILDTIME_DEPENDENCY)
  set(param_error ON)
else()
  if(HFCTEST_CONFIGURETIME_DEPENDENCY) 
    HermeticFetchContent_MakeAvailableAtConfigureTime(mathslib)
    HermeticFetchContent_MakeAvailableAtConfigureTime(Iconv)
  elseif(HFCTEST_BUILDTIME_DEPENDENCY)
    HermeticFetchContent_MakeAvailableAtBuildTime(mathslib)
    HermeticFetchContent_MakeAvailableAtBuildTime(Iconv)
  else()
    set(param_error ON)
  endif()
endif()

if(param_error)
  message(FATAL_ERROR "This project needs either HFCTEST_CONFIGURETIME_DEPENDENCY or HFCTEST_BUILDTIME_DEPENDENCY defined")
endif()


set(exampleTargetLinkLibs "MathFunctions::MathFunctions") # this one should always exist as we don't alias/rename it (we do that only for MathFunctionscbrt::MathFunctionscbrt)


# check for expected targets of our mathlib content
if(HFC_TEST_RENAMETARGET)
  list(APPEND exampleTargetLinkLibs "RenamedMathFunctionscbrt::MathFunctionscbrt")

  if(TARGET MathFunctionscbrt::MathFunctionscbrt)
    message(FATAL_ERROR "Found target MathFunctionscbrt::MathFunctionscbrt which should not exist")
  endif()
else()
  list(APPEND exampleTargetLinkLibs "AliasedMathFunctionscbrt::MathFunctionscbrt")

  if(NOT TARGET MathFunctionscbrt::MathFunctionscbrt)
    message(FATAL_ERROR "Didn't find target MathFunctionscbrt::MathFunctionscbrt which should still exist despite aliasing")
  endif()
endif()

# check for expected targets of our Iconv content
if(HFC_TEST_FORCE_SYSTEM_RENAME_TARGET)
  list(APPEND exampleTargetLinkLibs "RenamedIconv::Iconv")

  if(TARGET Iconv::Iconv)
    message(FATAL_ERROR "Found target Iconv::Iconv which should not exist")
  endif()

else()

  list(APPEND exampleTargetLinkLibs "AliasedIconv::Iconv")

  if(NOT TARGET Iconv::Iconv)
    message(FATAL_ERROR "Didn't find target Iconv::Iconv which should still exist despite aliasing")
  endif()

endif()


add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE ${exampleTargetLinkLibs})


