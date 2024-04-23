# Copyright (c) 2017 Ruslan Baratov, Alexandre Pretyman
# All rights reserved.
#
# This function generates a ./configure command line for autotools
#
# Usage example:
# hunter_openssl_configure_command(out_command_line # saves the result in this var
#   PACKAGE_CONFIGURATION_TYPES
#     "Release"                             # Mandatory ONE build config type
#   CONFIGURE_HOST
#     "armv7"                               # passed to --configure-host=
#   PACKAGE_INSTALL_DIR
#     "${HUNTER_PACKAGE_INSTALL_DIR}"       # passed to --prefix=
#   CPPFLAGS
#     "-DEXTRA_CPP_FLAGS"                   # extra preprocessor flags
#   CFLAGS
#     "-O2"                                 # extra c compilation flags
#   CXXFLAGS
#     "-Wall"                               # extra c++ compilation flags
#   LDFLAGS
#     "-lmycrazylib"                        # extra linking flags
#   EXTRA_FLAGS
#     --any-other                           # extra flags to be appended
#     --flags
#     --needed
# )
include_guard()

include(hunter_dump_cmake_flags)
include(hunter_fatal_error)
include(hunter_user_error)
include(hunter_get_build_flags)
include(hunter_get_toolchain_binaries)
include(hunter_internal_error)
include(hunter_status_debug)

function(hunter_openssl_configure_command out_command_line)
  set(optional_params)
  set(one_value_params
      CONFIGURE_HOST
      PACKAGE_INSTALL_DIR
      CPPFLAGS
      CFLAGS
      CXXFLAGS
      LDFLAGS
  )
  set(multi_value_params
      PACKAGE_CONFIGURATION_TYPES
      EXTRA_FLAGS
  )
  cmake_parse_arguments(
      PARAM
      "${optional_params}"
      "${one_value_params}"
      "${multi_value_params}"
      ${ARGN}
  )

  if(PARAM_UNPARSED_ARGUMENTS)
    hunter_internal_error(
        "Invalid arguments passed to hunter_openssl_configure_command"
        " ${PARAM_UNPARSED_ARGUMENTS}"
    )
  endif()


  # Build the configure command line options
  set(configure_command)

  list(APPEND configure_command "./Configure")

  if(MINGW)
    if(HUNTER_OPENSSL_MINGW64)
      set(configure_opts mingw64)
    else()
      set(configure_opts mingw)
    endif()
  
  elseif(ANDROID)
    string(COMPARE EQUAL "${CMAKE_ANDROID_ARCH}" "" _has_old_cmake)
    string(COMPARE EQUAL "${ANDROID_ARCH_NAME}" "" _has_unexpected_toolchain)
    if (NOT _has_old_cmake)
      set(ANDROID_SSL_ARCH "${CMAKE_ANDROID_ARCH}")
    elseif(NOT _has_unexpected_toolchain)
      set(ANDROID_SSL_ARCH "${ANDROID_ARCH_NAME}")
    else()
      hunter_user_error("Could not find android architecture. Please set ANDROID_ARCH_NAME in your toolchain or use CMake 3.7+")
    endif()
  
    # Building OpenSSL with Android:
    # * https://wiki.openssl.org/index.php/Android#Build_the_OpenSSL_Library
    # Set environment variables similar to 'setenv-android.sh' script:
    # * https://wiki.openssl.org/index.php/Android#Adjust_the_Cross-Compile_Script


    # Using android-* targets is currently broken for Clang on ALL versions
    # * https://github.com/openssl/openssl/pull/2229
    # The ./config script only detects Android x86 and armv7 targets anyway.
    # * https://github.com/openssl/openssl/issues/2490
    if (ANDROID_SSL_ARCH MATCHES "mips64|arm64|x86_64")
      set(configure_opts "linux-generic64")
    else()
      set(configure_opts "linux-generic32")
    endif()

    # On Android '--sysroot' differs for compile and linker.
    # With the '-l*' trick we can pass needed '--sysroot' to linker but not to
    # compiler.
    #
    # Tested:
    # * android-ndk-r16b-api-21-armeabi-v7a-neon-clang-libcxx
    # * android-ndk-r17-api-24-arm64-v8a-clang-libcxx14
    # * android-ndk-r18-api-24-arm64-v8a-clang-libcxx14
    #list(APPEND configure_opts "-latomic ${CMAKE_EXE_LINKER_FLAGS}")
  elseif(RASPBERRY_PI)
    set(configure_opts "linux-generic32")
  elseif(OPENWRT)
    set(configure_opts "linux-generic32" "no-async")
  elseif(EMSCRIPTEN)
    # disable features that will not work with emscripten
    set(configure_opts "no-engine" "no-dso" "no-asm" "no-shared" "no-sse2" "no-srtp")
  elseif(APPLE)
    set(configure_opts "darwin64-${CMAKE_SYSTEM_PROCESSOR}-cc")
  endif()
  
  list(APPEND configure_command ${configure_opts})
  list(APPEND configure_command MACHINE=${CMAKE_SYSTEM_PROCESSOR})

  hunter_get_toolchain_binaries(
      OUT_AR
        ar
      OUT_AS
        as
      OUT_LD
        ld
      OUT_NM
        nm
      OUT_OBJCOPY
        objcopy
      OUT_OBJDUMP
        objdump
      OUT_RANLIB
        ranlib
      OUT_STRIP
        strip
      OUT_CPP
        cpp
      OUT_CC
        cc
      OUT_CXX
        cxx
  )

  set(toolchain_binaries)
  if(ar)
    list(APPEND toolchain_binaries "AR=${ar}")
  endif()
  if(as)
    list(APPEND toolchain_binaries "AS=${as}")
  endif()

  if (NOT UNIX)
    if(ld)
      list(APPEND toolchain_binaries "LD=${ld}") #OPENSSL will ignore it on UNIX : https://github.com/openssl/openssl/blob/master/INSTALL.md#environment-variables
    endif()
  endif()

  if(nm)
    list(APPEND toolchain_binaries "NM=${nm}")
  endif()
  if(objcopy)
    list(APPEND toolchain_binaries "OBJCOPY=${objcopy}")
  endif()
  if(objdump)
    list(APPEND toolchain_binaries "OBJDUMP=${objdump}")
  endif()
  if(ranlib)
    list(APPEND toolchain_binaries "RANLIB=${ranlib}")
  endif()
  if(strip)
    list(APPEND toolchain_binaries "STRIP=${strip}")
  endif()
  if(cpp)
    list(APPEND toolchain_binaries "CPP=${cpp}")
  endif()
  if(cc)
    list(APPEND toolchain_binaries "CC=${cc}")
  endif()
  if(cxx)
    list(APPEND toolchain_binaries "CXX=${cxx}")
  endif()

  if(toolchain_binaries)
    list(APPEND configure_command ${toolchain_binaries})
  endif()

  list(LENGTH PARAM_PACKAGE_CONFIGURATION_TYPES len)
  if(NOT "${len}" EQUAL "1")
    hunter_fatal_error(
        "Autotools PACKAGE_CONFIGURATION_TYPES has ${len} elements: ${PARAM_PACKAGE_CONFIGURATION_TYPES}. Only 1 is allowed"
        ERROR_PAGE "autools.package.configuration.types"
    )
  endif()
  string(TOUPPER ${PARAM_PACKAGE_CONFIGURATION_TYPES} config_type)

  hunter_get_build_flags(
      INSTALL_DIR
        ${PARAM_PACKAGE_INSTALL_DIR}
      PACKAGE_CONFIGURATION_TYPES
        ${PARAM_PACKAGE_CONFIGURATION_TYPES}
      OUT_CPPFLAGS
        cppflags
      OUT_CFLAGS
        cflags
      OUT_CXXFLAGS
        cxxflags
      OUT_LDFLAGS
        ldflags
  )
  # -> CMAKE_C_FLAGS
  # -> CMAKE_CXX_FLAGS
  hunter_status_debug("Autotools complation/linking flags:")
  set(cppflags "${cppflags} ${PARAM_CPPFLAGS}")
  string(STRIP "${cppflags}" cppflags)
  hunter_status_debug("  CPPFLAGS=${cppflags}")
  list(APPEND configure_command CPPFLAGS=${cppflags})

  set(cflags "${cflags} ${PARAM_CFLAGS}")
  string(STRIP "${cflags}" cflags)
  hunter_status_debug("  CFLAGS=${cflags}")
  list(APPEND configure_command CFLAGS=${cflags})

  set(cxxflags "${cxxflags} ${PARAM_CXXFLAGS}")
  string(STRIP "${cxxflags}" cxxflags)
  hunter_status_debug("  CXXFLAGS=${cxxflags}")
  list(APPEND configure_command CXXFLAGS=${cxxflags})

  hunter_status_debug("  PARAM_LDFLAGS=${PARAM_LDFLAGS}")
  set(ldflags "${ldflags} ${PARAM_LDFLAGS}")
  string(STRIP "${ldflags}" ldflags)
  hunter_status_debug("  LDFLAGS=${ldflags}")
  list(APPEND configure_command LDFLAGS=${ldflags})


  if(PARAM_EXTRA_FLAGS)
    list(APPEND configure_command ${PARAM_EXTRA_FLAGS})
  endif()

  # Hunter builds static libraries by default
  if (BUILD_SHARED_LIBS)
    set(shared_flag shared)
  else()
    set(shared_flag no-shared)
  endif()
  
  list(APPEND configure_command threads)
  list(APPEND configure_command ${shared_flag})
  list(APPEND configure_command "--prefix=${PARAM_PACKAGE_INSTALL_DIR}")
  list(APPEND configure_command "--libdir=lib")

  if(HUNTER_STATUS_DEBUG)
    string(REPLACE ";" " " final_configure_command "${configure_command}")
    hunter_status_debug("Final configure command:")
    hunter_status_debug("  ${final_configure_command}")
  endif()

  set(${out_command_line} ${configure_command} PARENT_SCOPE)
endfunction()

