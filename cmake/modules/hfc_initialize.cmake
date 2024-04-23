# HermeticFetchContent / Initialize

include(hfc_log)
include(hfc_goldilock_helpers)
include(hfc_custom_echo_command)
include(FetchContent)
include(hfc_ensure_executable_version_greater_equal)

# enables the usage of CMake-RE if requested
function(hfc_initialize_enable_cmake_re_if_requested)

  if ("$ENV{CMAKE_RE_ENABLE}" OR CMAKE_RE_ENABLE)
    if (NOT CMAKE_RE_PATH)
      if (DEFINED ENV{CURRENT_CMAKE_RE_BINARY})
        set(CMAKE_RE_ENABLE ON CACHE BOOL "CMake RE mode" FORCE)
        set(CMAKE_RE_PATH "$ENV{CURRENT_CMAKE_RE_BINARY}" CACHE INTERNAL "CMake RE binary in use" FORCE)
        hfc_log_debug("Using CMake-RE from CURRENT_CMAKE_RE_BINARY: ${CMAKE_RE_PATH}")
      endif()
    endif()

    set(OVERRIDEN_CMAKE_COMMAND "${CMAKE_RE_PATH};-v" CACHE INTERNAL "CMake command to use in HermeticFetchContent" FORCE)
  else()
    set(OVERRIDEN_CMAKE_COMMAND ${CMAKE_COMMAND} CACHE INTERNAL "CMake command to use in HermeticFetchContent" FORCE)
    hfc_log_debug("Using CMake found at location: ${CMAKE_COMMAND}")
  endif()

endfunction()

function(hfc_run_goldilock_version_or_fail)
  hfc_log(TRACE "Using goldilock found at ${HERMETIC_FETCHCONTENT_goldilock_BIN}")
    __get_env_shell_command(shell)
    # test that it works  
    execute_process(
      COMMAND ${shell} "-c" "${HERMETIC_FETCHCONTENT_goldilock_BIN} --version"
      RESULT_VARIABLE ret_code
      OUTPUT_VARIABLE command_stdoutput
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(ret_code EQUAL 0)
      hfc_log(TRACE " - goldilock version info: ${command_stdoutput}")
    else()
      hfc_log(FATAL_ERROR "Failed start goldilock: command returned '${ret_code}'\n${command_stdoutput}")
    endif()
endfunction()


# make sure goldilock is available
#
# hfc_ensure_goldilock_available(
#  GOLDILOCK_REVISION <git-commit-hash>
#  GOLDILOCK_MINIMUM_VERSION <X.X.X> 
#
#  GOLDILOCK_URL_PREBUILT_Darwin_arm64 <URL>
#  GOLDILOCK_SHA_PREBUILT_Darwin_arm64 <SHA1>
#
#  GOLDILOCK_URL_PREBUILT_Darwin_x86_64 <URL>
#  GOLDILOCK_SHA_PREBUILT_Darwin_x86_64 <SHA1>
# 
#  GOLDILOCK_URL_PREBUILT_Linux_x86_64 <URL>
#  GOLDILOCK_SHA_PREBUILT_Linux_x86_64 <SHA1>
#)
# 
# This provisions goldilock as follow :
#  1. Check if there is a compatible goldilock on PATH
#  2. If there is none, try to download it
#  3. Check if there is a compatible downloaded goldilock
#  4. If downloaded goldilock doesn't work fetch and build it from sources
#  5. Fail if none can be used
#
function(hfc_ensure_goldilock_available) 

  # arguments parsing
  set(options "")
  set(oneValueArgs 
    GOLDILOCK_REVISION
    GOLDILOCK_MINIMUM_VERSION

    GOLDILOCK_URL_PREBUILT_Darwin_arm64 
    GOLDILOCK_SHA_PREBUILT_Darwin_arm64 
    
    GOLDILOCK_URL_PREBUILT_Darwin_x86_64
    GOLDILOCK_SHA_PREBUILT_Darwin_x86_64

    GOLDILOCK_URL_PREBUILT_Linux_x86_64 
    GOLDILOCK_SHA_PREBUILT_Linux_x86_64 
  )
  set(multiValueArgs
  )
  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  hfc_required_args(FN_ARG ${oneValueArgs})

  if(FN_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
        "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
        " ${FN_ARG_UNPARSED_ARGUMENTS}"
    )
  endif()

  file(REAL_PATH ${HERMETIC_FETCHCONTENT_TOOLS_DIR} tools_dir_realpath BASE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

  # TODO: Fix INSTALL_DIR, revision doesn't equal binary prebuilt
  set(HFC_GOLDILOCK_INSTALL_DIR "${tools_dir_realpath}/goldilock/${FN_ARG_GOLDILOCK_REVISION}")

  if(HERMETIC_FETCHCONTENT_TOOLCHAIN_IS_PROXY_TOOLCHAIN)
    hfc_log(STATUS "Using goldilock set by generated proxy toolchain, not checking as we are in a working dependency build: ${HERMETIC_FETCHCONTENT_goldilock_BIN}")
    return() # We should never do anything in these case, only use what was provided.
  endif()

  # make sure we are clean
  unset(HERMETIC_FETCHCONTENT_goldilock_BIN)
  unset(HERMETIC_FETCHCONTENT_goldilock_BIN CACHE)

  hfc_log_debug("Provisioning goldilock")

  # try to find the goldilock at the path that we will build it or download it 

  if (NOT DEFINED CMAKE_HOST_EXECUTABLE_SUFFIX)
    # Before CMake 3.31 CMAKE_HOST_EXECUTABLE_SUFFIX doesn't exists 
    set (CMAKE_HOST_EXECUTABLE_SUFFIX "${CMAKE_EXECUTABLE_SUFFIX}")
  endif() 
  set(HERMETIC_FETCHCONTENT_goldilock_BIN "${HFC_GOLDILOCK_INSTALL_DIR}/bin/goldilock${CMAKE_HOST_EXECUTABLE_SUFFIX}")

  hfc_log_debug("Checking if ${HERMETIC_FETCHCONTENT_goldilock_BIN} is available and compatible")
  set(goldilock_has_correct_version_and_is_executable FALSE)
  if(EXISTS "${HERMETIC_FETCHCONTENT_goldilock_BIN}")
    hfc_log_debug(" - found at ${HERMETIC_FETCHCONTENT_goldilock_BIN} - trying to run")
    hfc_ensure_executable_version_greater_equal(goldilock_has_correct_version_and_is_executable
      EXECUTABLE_PATH "${HERMETIC_FETCHCONTENT_goldilock_BIN}"
      MINIMUM_VERSION "${FN_ARG_GOLDILOCK_MINIMUM_VERSION}"
    )

    hfc_log_debug(" -> ran & matched version expectation: ${goldilock_has_correct_version_and_is_executable}")
    if (goldilock_has_correct_version_and_is_executable)
      hfc_log(STATUS "Using goldilock : ${HERMETIC_FETCHCONTENT_goldilock_BIN}")
      hfc_run_goldilock_version_or_fail()
      return(PROPAGATE HERMETIC_FETCHCONTENT_goldilock_BIN)
    else()
      unset(HERMETIC_FETCHCONTENT_goldilock_BIN)
      unset(HERMETIC_FETCHCONTENT_goldilock_BIN CACHE)
    endif()
  endif()

  if(NOT HERMETIC_FETCHCONTENT_goldilock_BIN OR NOT goldilock_has_correct_version_and_is_executable)
    # try to find a goldilock somewhere on all known search PATHs
    find_program(goldilock_found_on_some_paths "goldilock")
    if(goldilock_found_on_some_paths)
      hfc_log_debug(" - found at ${goldilock_found_on_some_paths} - trying to run")
      hfc_ensure_executable_version_greater_equal(goldilock_has_correct_version_and_is_executable
        EXECUTABLE_PATH "${goldilock_found_on_some_paths}"
        MINIMUM_VERSION "${FN_ARG_GOLDILOCK_MINIMUM_VERSION}"
      )
      hfc_log_debug(" -> ran & matched version expectation: ${goldilock_has_correct_version_and_is_executable}")

      if(goldilock_has_correct_version_and_is_executable)
        hfc_log(STATUS "golidlock has been found on the system and will be used. It's path is: ${HERMETIC_FETCHCONTENT_goldilock_BIN}")
        set(HERMETIC_FETCHCONTENT_goldilock_BIN "${goldilock_found_on_some_paths}")
        hfc_run_goldilock_version_or_fail()
        return(PROPAGATE HERMETIC_FETCHCONTENT_goldilock_BIN)
      endif()
    endif()
  endif()

  # The goldilock in the project is incompatible, the one on PATH as well or doesn't exists
  # Download prebuilt goldilock
  if(NOT HERMETIC_FETCHCONTENT_goldilock_BIN OR NOT goldilock_has_correct_version_and_is_executable)
    set(goldilock_url ${FN_ARG_GOLDILOCK_URL_PREBUILT_${CMAKE_HOST_SYSTEM_NAME}_${CMAKE_HOST_SYSTEM_PROCESSOR}})
    set(goldilock_sha ${FN_ARG_GOLDILOCK_SHA_PREBUILT_${CMAKE_HOST_SYSTEM_NAME}_${CMAKE_HOST_SYSTEM_PROCESSOR}})
    # There is a prebuilt version
    if(NOT goldilock_url STREQUAL "")

      hfc_log_debug(" - Downloading prebuilt goldilock from ${goldilock_url}")
      set(download_test "${tools_dir_realpath}/goldilock.zip")
      if(EXISTS "${download_test}")
        file(REMOVE ${download_test})
      endif()

      # Delete existing goldilock otherwise mv install of the bin directory won't work
      if(EXISTS "${HFC_GOLDILOCK_INSTALL_DIR}/bin/goldilock")
        file(REMOVE_RECURSE "${HFC_GOLDILOCK_INSTALL_DIR}/bin")
      endif()

      file(DOWNLOAD
        "${goldilock_url}"
        "${download_test}"
        SHOW_PROGRESS
        STATUS download_status
        LOG error)
      list(GET download_status 0 download_godilock_status)
      if(download_godilock_status EQUAL 0)
        file(SHA1 "${download_test}" computed_sha1)
        if(computed_sha1 STREQUAL goldilock_sha)
          file(ARCHIVE_EXTRACT
            INPUT "${download_test}"
            DESTINATION "${HFC_GOLDILOCK_INSTALL_DIR}"
          ) 
          file(RENAME "${HFC_GOLDILOCK_INSTALL_DIR}/goldilock/bin" "${HFC_GOLDILOCK_INSTALL_DIR}/bin")
          file(REMOVE ${download_test})
          file(REMOVE_RECURSE "${HFC_GOLDILOCK_INSTALL_DIR}/goldilock")
          set(goldilock_just_downloaded "${HFC_GOLDILOCK_INSTALL_DIR}/bin/goldilock${CMAKE_HOST_EXECUTABLE_SUFFIX}" )

          hfc_log_debug(" -> installed prebuilt to: ${goldilock_just_downloaded}")
          hfc_ensure_executable_version_greater_equal(goldilock_has_correct_version_and_is_executable
            EXECUTABLE_PATH "${goldilock_just_downloaded}"
            MINIMUM_VERSION "${FN_ARG_GOLDILOCK_MINIMUM_VERSION}"
          )
          hfc_log_debug(" -> ran & matched version expectation: ${goldilock_has_correct_version_and_is_executable}")
    
          if(goldilock_has_correct_version_and_is_executable)
            hfc_log(STATUS "golidlock has been found on the system and will be used. It's path is: ${HERMETIC_FETCHCONTENT_goldilock_BIN}")
            set(HERMETIC_FETCHCONTENT_goldilock_BIN "${goldilock_just_downloaded}")
            hfc_run_goldilock_version_or_fail()
            return(PROPAGATE HERMETIC_FETCHCONTENT_goldilock_BIN)
          endif()

        endif()
      endif()
    endif()
  endif()

  # Ok we have no luck, we need to build it for the current host
  if(NOT HERMETIC_FETCHCONTENT_goldilock_BIN OR NOT goldilock_has_correct_version_and_is_executable)
    hfc_log_debug(" - Building goldilock from source (${FN_ARG_GOLDILOCK_REVISION})")
    set(HFC_GOLDILOCK_BUILD_ARGS "")

    include(ExternalProject)
    include(FetchContent)

    hfc_log_debug(" - Fetching goldilock")

    FetchContent_Populate(
      hfc_goldilock
      GIT_REPOSITORY https://github.com/tipi-build/goldilock.git
      GIT_TAG        "${FN_ARG_GOLDILOCK_REVISION}"
      SUBBUILD_DIR  "${CMAKE_CURRENT_BINARY_DIR}/.hfc_tools/hfc_goldilock-subbuild"
      SOURCE_DIR    "${CMAKE_CURRENT_BINARY_DIR}/.hfc_tools/hfc_goldilock-src"
      BINARY_DIR    "${CMAKE_CURRENT_BINARY_DIR}/.hfc_tools/hfc_goldilock-build"
    )
    
    hfc_log_debug(" - Configuring goldilock")
    
    # cmake configure
    execute_process(
      COMMAND ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR} -S ${hfc_goldilock_SOURCE_DIR} -B ${hfc_goldilock_BINARY_DIR} ${HFC_GOLDILOCK_BUILD_ARGS}
      WORKING_DIRECTORY "${hfc_goldilock_BINARY_DIR}"
      COMMAND_ECHO STDOUT
      COMMAND_ERROR_IS_FATAL ANY
    ) 
    
    hfc_log_debug(" - Building goldilock")
    
    # cmake --build
    execute_process(
      COMMAND ${CMAKE_COMMAND} --build ${hfc_goldilock_BINARY_DIR} --config Release
      WORKING_DIRECTORY "${hfc_goldilock_BINARY_DIR}"
      COMMAND_ECHO STDOUT
      COMMAND_ERROR_IS_FATAL ANY
    )

    hfc_log_debug(" - Installing goldilock")
    
    # cmake --install
    execute_process(
      COMMAND ${CMAKE_COMMAND} --install ${hfc_goldilock_BINARY_DIR} --prefix ${HFC_GOLDILOCK_INSTALL_DIR}
      WORKING_DIRECTORY "${hfc_goldilock_BINARY_DIR}"
      COMMAND_ECHO STDOUT
      COMMAND_ERROR_IS_FATAL ANY
    )

    # test just built goldilock
    set(goldilock_just_built "${HFC_GOLDILOCK_INSTALL_DIR}/bin/goldilock${CMAKE_HOST_EXECUTABLE_SUFFIX}" )
    hfc_ensure_executable_version_greater_equal(goldilock_has_correct_version_and_is_executable
      EXECUTABLE_PATH "${goldilock_just_built}"
      MINIMUM_VERSION "${FN_ARG_GOLDILOCK_MINIMUM_VERSION}"
    )
    hfc_log_debug(" -> ran & matched version expectation: ${goldilock_has_correct_version_and_is_executable}")

    if(goldilock_has_correct_version_and_is_executable)
      set(HERMETIC_FETCHCONTENT_goldilock_BIN "${goldilock_just_built}")
      hfc_run_goldilock_version_or_fail()
      return(PROPAGATE HERMETIC_FETCHCONTENT_goldilock_BIN)
    else()
      message(FATAL_ERROR "The goldilock that was just built ${goldilock_just_built} is incompatible with HFC version requirements. This is an HFC Maintainer error, contact team tipi.build.")
      return()
    endif()
  endif()
    
  hfc_run_goldilock_version_or_fail()

endfunction()



# initializes important base settings of Hermetic FetchContent
function(hfc_initialize HFC_ROOT_DIR)
  
  # enable modern CMake IN_LIST support
  cmake_policy(SET CMP0057 NEW)

  get_property(hfc_initialized GLOBAL PROPERTY HERMETIC_FETCHCONTENT_INITIALIZED SET)

  if(NOT hfc_initialized)
    
    if (NOT DEFINED CACHE{HERMETIC_FETCHCONTENT_REMOVE_BUILD_DIR_AFTER_INSTALL})
      set(HERMETIC_FETCHCONTENT_REMOVE_BUILD_DIR_AFTER_INSTALL OFF CACHE BOOL "By default save space by removing build" FORCE)
    endif()
    
    if (NOT DEFINED CACHE{HERMETIC_FETCHCONTENT_REMOVE_BUILD_DIR_AFTER_INSTALL})
      set(HERMETIC_FETCHCONTENT_REMOVE_SOURCE_DIR_AFTER_INSTALL OFF CACHE BOOL "By default save space by removing sources after install" FORCE)
    endif()

    set_property(GLOBAL PROPERTY HERMETIC_FETCHCONTENT_INITIALIZED ON)
    set(HERMETIC_FETCHCONTENT_ROOT_DIR "${HFC_ROOT_DIR}" CACHE INTERNAL "Root directory of Hermetic_FetchContent")
    set(HERMETIC_FETCHCONTENT_CONSUMED_CACHETARGETFILES "" CACHE INTERNAL "Cache target files consumed by Hermetic_FetchContent")

  endif()

  # If the HERMETIC_FETCHCONTENT_INSTALL_DIR is not set we use CMake Default FETCHCONTENT_BASE_DIR ( i.e. build/_deps/ )
  if (NOT DEFINED HERMETIC_FETCHCONTENT_INSTALL_DIR)
    set(HERMETIC_FETCHCONTENT_INSTALL_DIR "${FETCHCONTENT_BASE_DIR}" CACHE INTERNAL "HFC's dependencies installation dir")
  endif()

  if(NOT HERMETIC_FETCHCONTENT_TOOLS_DIR)
    # note: this is necessary when operating without set  hfc basedir or explicitely provided HERMETIC_FETCHCONTENT_TOOLS_DIR
    # as otherwise each and every dependency will build it's own goldilock when running the external project subbuild project
    # as these will include and call hfc_initialize() too and the generated project needs HERMETIC_FETCHCONTENT_TOOLS_DIR
    # to be set in order to share the info
    set(HERMETIC_FETCHCONTENT_TOOLS_DIR "${CMAKE_SOURCE_DIR}/thirdparty/cache/.hfc_tools" CACHE INTERNAL "Default Hermetic-FetchContent tools dir")
    hfc_log_debug("HERMETIC_FETCHCONTENT_TOOLS_DIR = ${HERMETIC_FETCHCONTENT_TOOLS_DIR}")
  endif()

  if(NOT HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR)
    set(HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/cache" CACHE INTERNAL "Default value for Hermetic-FetchContent source cache dir")
    hfc_log_debug("HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR = ${HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR}")
  endif()

  # write a simple gitignore all rule to the HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR folder. If the file is present we don't touch
  # it in case someone wanted to customize it
  set(hfc_wd_gitignore_file "${HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR}/.gitignore")
  if(NOT EXISTS "${hfc_wd_gitignore_file}")
    file(WRITE 
      "${hfc_wd_gitignore_file}" 
      "**\n" 
    )
  endif()

  set(hfc_wd_tipiignore_file "${HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR}/.tipiignore")
  if(NOT EXISTS "${hfc_wd_tipiignore_file}")
    file(WRITE 
      "${hfc_wd_tipiignore_file}" 
      "**\n" 
    )
  endif()

  set(GOLDILOCK_MINIMUM_VERSION 1.2.0)
  hfc_ensure_goldilock_available(
    GOLDILOCK_REVISION 5b34d2fed2d7da1280a2c5a8134f3d6fe10587df
    GOLDILOCK_MINIMUM_VERSION ${GOLDILOCK_MINIMUM_VERSION}

    GOLDILOCK_URL_PREBUILT_Darwin_arm64 https://github.com/tipi-build/goldilock/releases/download/v${GOLDILOCK_MINIMUM_VERSION}/goldilock-macos.zip
    GOLDILOCK_SHA_PREBUILT_Darwin_arm64 642c533ef1257187ab023dc33b95c66ec84b38af

    GOLDILOCK_URL_PREBUILT_Darwin_x86_64 https://github.com/tipi-build/goldilock/releases/download/v${GOLDILOCK_MINIMUM_VERSION}/goldilock-macos-intel.zip
    GOLDILOCK_SHA_PREBUILT_Darwin_x86_64 8ae6b38766ed485e0b8d6d0b7460bb7e8a71a015
   
    GOLDILOCK_URL_PREBUILT_Linux_x86_64 https://github.com/tipi-build/goldilock/releases/download/v${GOLDILOCK_MINIMUM_VERSION}/goldilock-linux.zip
    GOLDILOCK_SHA_PREBUILT_Linux_x86_64 da1c7b9e9d79fa019ca0e41a002b6fed4fd854ee
  )
  set(HERMETIC_FETCHCONTENT_goldilock_BIN ${HERMETIC_FETCHCONTENT_goldilock_BIN} PARENT_SCOPE)

  hfc_initialize_enable_cmake_re_if_requested()  

  if(NOT TARGET hfc_list_dependencies_build_dirs)

    hfc_custom_echo_command_create("hfc_list_dependencies_build_dirs_echo_cmd" "===HFC_DEPENDECY_BUILD_DIRS===")
    add_custom_target(hfc_list_dependencies_build_dirs
      COMMENT "Listing dependency build dirs"
      DEPENDS hfc_list_dependencies_build_dirs_echo_cmd
    )
  endif()

  if(NOT TARGET hfc_list_dependencies_install_dirs)
    hfc_custom_echo_command_create("hfc_list_dependencies_install_dirs_echo_cmd" "===HFC_DEPENDECY_INSTALL_DIRS===")
    add_custom_target(hfc_list_dependencies_install_dirs
      COMMENT "Listing dependency install dirs"
      DEPENDS hfc_list_dependencies_install_dirs_echo_cmd
    )
  endif()

  if(NOT TARGET hfc_list_dependencies_target_cache_files)
    hfc_custom_echo_command_create("hfc_list_dependencies_target_cache_files_echo_cmd" "===HFC_DEPENDENCY_TARGET_CACHE_FILES===")
    add_custom_target(hfc_list_dependencies_target_cache_files
      COMMENT "Listing dependency target cache files"
      DEPENDS hfc_list_dependencies_target_cache_files_echo_cmd
    )
  endif()

endfunction()