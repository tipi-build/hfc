cmake_minimum_required(VERSION 3.27.6)
project(hfc-openssl-example)

include(FetchContent)

set(hfc_REPOSITORY https://github.com/tipi-build/hfc)
set(hfc_REVISION main)

include(HermeticFetchContent OPTIONAL RESULT_VARIABLE hfc_included) 
if(NOT hfc_included)
  FetchContent_Populate(
    hermetic_fetchcontent
    GIT_REPOSITORY https://github.com/tipi-build/hfc.git
    GIT_TAG ${hfc_REVISION}
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/src"
    SUBBUILD_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/subbuild"
    BINARY_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/bin"
  )
  FetchContent_GetProperties(hermetic_fetchcontent)
  message(STATUS "Hermetic FetchContent ${hfc_REVISION} available at '${hermetic_fetchcontent_SOURCE_DIR}'")
  list(APPEND CMAKE_MODULE_PATH "${hermetic_fetchcontent_SOURCE_DIR}/cmake")
  include(HermeticFetchContent) 
endif()


set(HERMETIC_FETCHCONTENT_FORWARDED_CMAKE_VARIABLES
  "CMAKE_BUILD_TYPE"
  "CMAKE_DEBUG_POSTFIX"
  "CMAKE_EXE_LINKER_FLAGS"
  "CMAKE_C_COMPILER"
  "CMAKE_C_FLAGS"
  "CMAKE_C_COMPILE_FEATURES"
  "CMAKE_C_EXTENSIONS"
  "CMAKE_C_STANDARD"
  "CMAKE_C_STANDARD_REQUIRED"
  "CMAKE_CXX_COMPILER"
  "CMAKE_CXX_FLAGS"
  "CMAKE_CXX_COMPILE_FEATURES"
  "CMAKE_CXX_COMPILER_IMPORT_STD"
  "CMAKE_CXX_EXTENSIONS"
  "CMAKE_CXX_STANDARD"
  "CMAKE_CXX_STANDARD_REQUIRED"
  "BUILD_SHARED_LIBS"
)

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

add_executable(main main.cpp)
target_link_libraries(main OpenSSL::Crypto OpenSSL::SSL)