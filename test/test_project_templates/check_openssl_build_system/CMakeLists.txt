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

include(HermeticFetchContent)
FetchContent_Declare(
  OpenSSL
#  URL https://www.openssl.org/source/old/1.1.1/openssl-1.1.1n.tar.gz
#  URL_HASH SHA1=4b0936dd798f60c97c68fc62b73033ecba6dfb0c
  URL https://github.com/openssl/openssl/releases/download/openssl-3.2.1/openssl-3.2.1.tar.gz
  URL_HASH SHA1=9668723d65d21a9d13e985203ce8c27ac5ecf3ae
)

find_package(Threads REQUIRED)
FetchContent_MakeHermetic(
  OpenSSL
  FIND_PACKAGE_ARGS
  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION
  [=[

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
  set(_OPENSSL_applink_interface_srcs "@HFC_PREFIX_PLACEHOLDER@/openssl/applink.c")

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

  add_library(OpenSSL::applink INTERFACE IMPORTED)
  set_property(TARGET OpenSSL::applink APPEND
    PROPERTY INTERFACE_SOURCES
      ${_OPENSSL_applink_interface_srcs})

  ]=]

  HERMETIC_TOOLCHAIN_EXTENSION
  [=[
  ]=]

  HERMETIC_BUILD_SYSTEM openssl
)

HermeticFetchContent_MakeAvailableAtConfigureTime(OpenSSL)

add_executable(MyExample simple_example.cpp)
target_link_libraries(MyExample PRIVATE OpenSSL::SSL OpenSSL::Crypto)