set(FETCHCONTENT_QUIET OFF)
include(FetchContent)

set(hfc_REVISION main)
if(DEFINED ENV{COMMIT_ID} AND NOT "$ENV{COMMIT_ID}" STREQUAL "")
    set(hfc_REVISION "$ENV{COMMIT_ID}")
endif()

list(APPEND CMAKE_MODULE_PATH "/workspaces/hfc/cmake")

include(HermeticFetchContent OPTIONAL RESULT_VARIABLE hfc_included)
if(NOT hfc_included)
  FetchContent_Populate(
    hfc
    GIT_REPOSITORY https://github.com/tipi-build/hfc.git
    GIT_TAG ${hfc_REVISION}
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/src"
    SUBBUILD_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/subbuild"
    BINARY_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/bin"
  )
  FetchContent_GetProperties(hfc)
  message(STATUS "Hermetic FetchContent ${hfc_REVISION} available at '${hfc_SOURCE_DIR}'")
  list(APPEND CMAKE_MODULE_PATH "${hfc_SOURCE_DIR}/cmake")
  include(HermeticFetchContent)
endif()

set(HERMETIC_FETCHCONTENT_BYPASS_PROVIDER_FOR_PACKAGES "Threads;Python;dl;m;Git")

FetchContent_Declare(
  OpenSSL
  URL https://github.com/openssl/openssl/releases/download/openssl-3.2.1/openssl-3.2.1.tar.gz
  URL_HASH SHA1=9668723d65d21a9d13e985203ce8c27ac5ecf3ae
)

FetchContent_MakeHermetic(
  OpenSSL
  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION [=[
    function(_OpenSSL_target_add_dependencies target)
      #set_property( TARGET ${target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ZLIB::ZLIB )
      set_property( TARGET ${target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES Threads::Threads)
      set_property( TARGET ${target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${CMAKE_DL_LIBS} )

      if(WIN32 AND OPENSSL_USE_STATIC_LIBS)
        if(WINCE)
          set_property( TARGET ${target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ws2 )
        else()
          set_property( TARGET ${target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ws2_32 )
        endif()
        set_property( TARGET ${target} APPEND PROPERTY INTERFACE_LINK_LIBRARIES crypt32 )
      endif()
    endfunction()
    set(OPENSSL_INCLUDE_DIR "@HFC_PREFIX_PLACEHOLDER@/include")
    set(OPENSSL_CRYPTO_LIBRARY "@HFC_PREFIX_PLACEHOLDER@/lib/libcrypto.a")
    set(OPENSSL_SSL_LIBRARY "@HFC_PREFIX_PLACEHOLDER@/lib/libssl.a")

    add_library(OpenSSL::Crypto UNKNOWN IMPORTED)
    set_target_properties(OpenSSL::Crypto PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
    set_target_properties(OpenSSL::Crypto PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}")
    _OpenSSL_target_add_dependencies(OpenSSL::Crypto)

    add_library(OpenSSL::SSL UNKNOWN IMPORTED)
    set_target_properties(OpenSSL::SSL PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
    set_target_properties(OpenSSL::SSL PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}")
      set_target_properties(OpenSSL::SSL PROPERTIES
        INTERFACE_LINK_LIBRARIES OpenSSL::Crypto)
    _OpenSSL_target_add_dependencies(OpenSSL::SSL)
  ]=]

  HERMETIC_BUILD_SYSTEM openssl
  SBOM_LICENSE "Apache License 2.0"
  SBOM_SUPPLIER "The OpenSSL Project Authors"
)

HermeticFetchContent_MakeAvailableAtConfigureTime(OpenSSL)

FetchContent_Declare(
  ZLIB
  GIT_REPOSITORY https://github.com/tipi-build/zlib.git
  GIT_TAG        44e8fad11a603327b255ae1f1756c0eb56bfdd07
)

FetchContent_MakeHermetic(
  ZLIB

  HERMETIC_TOOLCHAIN_EXTENSION [=[
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
  ]=]

  HERMETIC_CREATE_TARGET_ALIASES [=[
    if("${TARGET_NAME}" STREQUAL "ZLIB::zlib")
      list(APPEND TARGET_ALIASES "ZLIB::zlib" "ZLIB::ZLIB")
    endif()
  ]=]

  SBOM_LICENSE "zlib License"
  SBOM_SUPPLIER "Jean-loup Gailly and Mark Adler"
)

HermeticFetchContent_MakeAvailableAtConfigureTime(ZLIB)


FetchContent_Declare(
  CURL
  GIT_REPOSITORY https://github.com/curl/curl.git
  GIT_TAG        cd95ee9f771361acf241629d2fe5507e308082a2 # curl 7.86.0
)

FetchContent_MakeHermetic(
  CURL
  HERMETIC_BUILD_SYSTEM cmake
  HERMETIC_FIND_PACKAGES "OpenSSL;ZLIB" # this means "CURL can find_package() OpenSSL and ZLIB"
  HERMETIC_TOOLCHAIN_EXTENSION [=[
    set(CURL_USE_LIBSSH2 OFF)
    set(CURL_DISABLE_LDAP ON)
    set(USE_LIBIDN2 OFF)
    set(BUILD_CURL_EXE OFF)
    set(CURL_USE_OPENSSL ON)
    set(CURL_USE_ZLIB ON)
    set(CURL_CA_PATH none)
    set(BUILD_SHARED_LIBS OFF)
  ]=]

  SBOM_LICENSE "curl license"
  SBOM_SUPPLIER " Daniel Stenberg, <daniel@haxx.se>, and many contributors"
)

HermeticFetchContent_MakeAvailableAtConfigureTime(CURL)

FetchContent_Declare(
  lz4
  GIT_REPOSITORY https://github.com/lz4/lz4.git
  GIT_TAG ebb370ca83af193212df4dcbadcc5d87bc0de2f0 # v1.10.0 (2024-07-21)
  SOURCE_SUBDIR build/cmake
)

FetchContent_MakeHermetic(
  lz4
  HERMETIC_TOOLCHAIN_EXTENSION [=[
    set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  ]=]

  SBOM_LICENSE "BSD 2-Clause license"
  SBOM_SUPPLIER "Yann Collet"
)

HermeticFetchContent_MakeAvailableAtConfigureTime(lz4)

FetchContent_Declare(
  zstd
  GIT_REPOSITORY https://github.com/facebook/zstd.git
  GIT_TAG f8745da6ff1ad1e7bab384bd1f9d742439278e99 # v1.3.1.2 (2025-12-08)
  SOURCE_SUBDIR build/cmake
)

FetchContent_MakeHermetic(
  zstd

  HERMETIC_TOOLCHAIN_EXTENSION [=[
    set(ZSTD_BUILD_STATIC ON)
    set(ZSTD_BUILD_SHARED OFF)
  ]=]

  SBOM_LICENSE "BSD License"
  SBOM_SUPPLIER "Facebook, Inc."
)

HermeticFetchContent_MakeAvailableAtConfigureTime(zstd)

FetchContent_Declare(
  BZip2
  GIT_REPOSITORY https://github.com/cpp-pm/bzip2.git
  GIT_TAG        6790b7198e2b7646b9d613284d181fe9f85a5cb5 # v1.0.6-p4
)

FetchContent_MakeHermetic(
  BZip2

  HERMETIC_CREATE_TARGET_ALIASES [=[
    if("${TARGET_NAME}" STREQUAL "BZip2::bz2")
      list(APPEND TARGET_ALIASES "BZip2::bz2" "BZip2::BZip2")
    endif()
  ]=]

  SBOM_LICENSE "bzip2/libbzip2 license version 1.0.6"
  SBOM_SUPPLIER "Julian Seward"
)
HermeticFetchContent_MakeAvailableAtConfigureTime(BZip2)

FetchContent_Declare(
  Boost
  GIT_REPOSITORY https://github.com/boostorg/boost
  GIT_TAG        "ab7968a0bbcf574a7859240d1d8443f58ed6f6cf" # 1.85
)

FetchContent_MakeHermetic(
  Boost
  HERMETIC_TOOLCHAIN_EXTENSION
  [=[
    set(BOOST_BUILD_TEST OFF CACHE BOOL "" FORCE)
    set(BOOST_ENABLE_PYTHON OFF CACHE BOOL "" FORCE)
  ]=]

  HERMETIC_CMAKE_ADDITIONAL_EXPORTS [=[
    # Boost::dynamic_linking is created by BoostConfig.cmake logic (not exported)
    # but referenced in INTERFACE_LINK_LIBRARIES of Boost targets
    if(NOT TARGET Boost::dynamic_linking)
      add_library(Boost::dynamic_linking INTERFACE IMPORTED)
      set_property(TARGET Boost::dynamic_linking PROPERTY INTERFACE_COMPILE_DEFINITIONS "BOOST_ALL_NO_LIB")
    endif()

    if(NOT TARGET Boost::disable_autolinking)
      add_library(Boost::disable_autolinking INTERFACE IMPORTED)
      set_property(TARGET Boost::disable_autolinking PROPERTY INTERFACE_COMPILE_DEFINITIONS "BOOST_ALL_NO_LIB")
    endif()
  ]=]

  SBOM_LICENSE "Boost Software License - Version 1.0"
  SBOM_SUPPLIER "Boost.org"
)

HermeticFetchContent_MakeAvailableAtConfigureTime(Boost)

FetchContent_Declare(
  Snappy
  GIT_REPOSITORY https://github.com/google/snappy
  GIT_TAG 2c94e11145f0b7b184b831577c93e5a41c4c0346 #Snappy 1.2.1
)

FetchContent_MakeHermetic(
  Snappy
  HERMETIC_TOOLCHAIN_EXTENSION [=[
    set(SNAPPY_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(SNAPPY_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
  ]=]


  SBOM_LICENSE "BSD License 2.0"
  SBOM_SUPPLIER "Google Inc."
)

HermeticFetchContent_MakeAvailableAtConfigureTime(Snappy)

FetchContent_Declare(
  Thrift
  GIT_REPOSITORY https://github.com/tipi-deps/thrift
  GIT_TAG 398f874499b2799f4a4ef2c4ff39874600c97c75
)

FetchContent_MakeHermetic(
  Thrift
  HERMETIC_FIND_PACKAGE_VERSION_OVERRIDE "0.23.0"
  HERMETIC_FIND_PACKAGES "OpenSSL;ZLIB;Boost"
  HERMETIC_TOOLCHAIN_EXTENSION [=[
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_TUTORIALS OFF CACHE BOOL "" FORCE)
    set(BUILD_COMPILER OFF CACHE BOOL "" FORCE)
    set(BUILD_CPP ON CACHE BOOL "" FORCE)
    set(BUILD_C_GLIB OFF CACHE BOOL "" FORCE)
    set(BUILD_JAVASCRIPT OFF CACHE BOOL "" FORCE)
    set(BUILD_JAVA OFF CACHE BOOL "" FORCE)
    set(BUILD_KOTLIN OFF CACHE BOOL "" FORCE)
    set(BUILD_NODEJS OFF CACHE BOOL "" FORCE)
    set(BUILD_PYTHON OFF CACHE BOOL "" FORCE)
    set(Boost_with_cmake ON CACHE BOOL "" FORCE)
  ]=]

  SBOM_LICENSE "Apache License Version 2.0"
  SBOM_SUPPLIER "Apache Software Foundation"
)

HermeticFetchContent_MakeAvailableAtConfigureTime(Thrift)

FetchContent_Declare(
  xsimd
  GIT_REPOSITORY https://github.com/xtensor-stack/xsimd
  GIT_TAG 5ac7edf30d0f519e0b7344b933382e4fc02fdee7 # xsimd 13.0.0
)

FetchContent_MakeHermetic(
  xsimd
  HERMETIC_TOOLCHAIN_EXTENSION [=[
  set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  ]=]


  SBOM_LICENSE "BSD-3-Clause license"
  SBOM_SUPPLIER "Johan Mabille and Sylvain Corlay"
)

HermeticFetchContent_MakeAvailableAtConfigureTime(xsimd)

FetchContent_Declare(
  jemalloc
  URL https://github.com/jemalloc/jemalloc/releases/download/5.3.0/jemalloc-5.3.0.tar.bz2
  URL_HASH SHA1=1c8f2d0dfbf39fa8cd86363bf3314351ab21f8d4
)

FetchContent_MakeHermetic(
  jemalloc
  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION
    [=[
      add_library(jemalloc::jemalloc STATIC IMPORTED)
      set_property(TARGET jemalloc::jemalloc PROPERTY IMPORTED_LOCATION "@HFC_PREFIX_PLACEHOLDER@/lib/libjemalloc.a")
      set_property(TARGET jemalloc::jemalloc PROPERTY INTERFACE_INCLUDE_DIRECTORIES @HFC_PREFIX_PLACEHOLDER@/include)
    ]=]
  HERMETIC_BUILD_SYSTEM autotools
  SBOM_LICENSE "Modified BSD 2-Clause"
  SBOM_SUPPLIER "Jason Evans, Mozilla Foundation., Facebook, Inc."
)

HermeticFetchContent_AddContentAliases(jemalloc "jemallocAlt")

HermeticFetchContent_MakeAvailableAtConfigureTime(jemalloc)

FetchContent_Declare(
  arrow
  GIT_REPOSITORY https://github.com/apache/arrow.git
  GIT_TAG        6a2e19a852b367c72d7b12da4d104456491ed8b7 # apache-arrow-17.0.0
  SOURCE_SUBDIR cpp/
)

set(ARROW_HERMETIC_SETTINGS [=[
  # arrow's build system doesn't use CMAKE_DEBUG_POSTFIX as it is supposed to
  # but it does use CMAKE_STATIC_LIBRARY_SUFFIX
  if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_DEBUG_POSTFIX)
    set(CMAKE_STATIC_LIBRARY_SUFFIX "${CMAKE_DEBUG_POSTFIX}.${CMAKE_STATIC_LIBRARY_SUFFIX}")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
  set(BUILD_WARNING_LEVEL "PRODUCTION" CACHE STRING "" FORCE)
  set(ARROW_PARQUET ON CACHE BOOL "" FORCE)
  set(ARROW_WITH_SNAPPY ON CACHE BOOL "" FORCE)
  set(ARROW_WITH_ZLIB ON CACHE BOOL "" FORCE)
  set(ARROW_WITH_LZ4 ON CACHE BOOL "" FORCE)
  set(ARROW_WITH_ZSTD ON CACHE BOOL "" FORCE)
  set(ARROW_ZSTD_USE_SHARED OFF CACHE BOOL "" FORCE)
  set(ARROW_ZSTD_USE_STATIC ON CACHE BOOL "" FORCE)
  set(ARROW_BUILD_SHARED OFF CACHE BOOL "" FORCE)
  set(ARROW_BUILD_STATIC ON CACHE BOOL "" FORCE)
  set(ARROW_DEPENDENCY_SOURCE "SYSTEM" CACHE STRING "" FORCE)
  set(PARQUET_BUILD_EXECUTABLES ON CACHE BOOL "" FORCE)
  set(ARROW_USE_CCACHE OFF CACHE BOOL "" FORCE)
  set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
  set(ARROW_BUNDLED_STATIC_LIBS "" CACHE STRING "" FORCE)

  # force jemalloc lookup in system:
  set(jemalloc_SOURCE "SYSTEM" CACHE STRING "" FORCE)
]=])

FetchContent_MakeHermetic(
  arrow
  HERMETIC_FIND_PACKAGES "OpenSSL;ZLIB;CURL;lz4;zstd;Bzip2;Boost;Snappy;Thrift;xsimd;jemalloc;jemallocAlt"

  HERMETIC_TOOLCHAIN_EXTENSION "${ARROW_HERMETIC_SETTINGS}"
  HERMETIC_CMAKE_ADDITIONAL_EXPORTS [=[
    # Arrow always references this target from arrow_static even when no deps are bundled
    if(NOT TARGET Arrow::arrow_bundled_dependencies)
      add_library(Arrow::arrow_bundled_dependencies STATIC IMPORTED)
      set_property(TARGET Arrow::arrow_bundled_dependencies PROPERTY IMPORTED_LOCATION "@HFC_PREFIX_PLACEHOLDER@/lib/libarrow_bundled_dependencies.a")
    endif()

    # Arrow uses jemalloc internally but doesn't export it in INTERFACE_LINK_LIBRARIES
    set_property(TARGET Arrow::arrow_static APPEND PROPERTY INTERFACE_LINK_LIBRARIES jemalloc::jemalloc)
  ]=]

  SBOM_LICENSE "Apache License Version 2.0"
  SBOM_SUPPLIER "The Apache Foundation"
)

HermeticFetchContent_MakeAvailableAtConfigureTime(arrow)