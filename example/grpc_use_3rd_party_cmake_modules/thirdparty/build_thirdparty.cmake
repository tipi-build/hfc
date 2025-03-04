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
  ZLIB
  GIT_REPOSITORY https://github.com/cpp-pm/zlib.git
  GIT_TAG        57af136e436c5596e4f1c63fd5bdd2ce988777d1
)

FetchContent_MakeHermetic(
  ZLIB
  
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
  absl
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG        4a2c63365eff8823a5221db86ef490e828306f9d
)

FetchContent_MakeHermetic(
  absl
  HERMETIC_TOOLCHAIN_EXTENSION [=[
    set(ABSL_ENABLE_INSTALL  ON CACHE BOOL "" FORCE)
    set(ABSL_PROPAGATE_CXX_STD  ON CACHE BOOL "" FORCE)
  ]=]
  SBOM_LICENSE "Apache License 2.0"
  SBOM_SUPPLIER "The abseil-cpp Project Authors"
)

HermeticFetchContent_MakeAvailableAtConfigureTime(absl)

FetchContent_Declare(
  re2
  GIT_REPOSITORY https://github.com/google/re2.git
  GIT_TAG        6dcd83d60f7944926bfd308cc13979fc53dd69ca
)

FetchContent_MakeHermetic(
  re2
  HERMETIC_FIND_PACKAGES "absl"
  SBOM_LICENSE "BSD 3-Clause"
  SBOM_SUPPLIER "The RE2 Authors"
)

HermeticFetchContent_MakeAvailableAtConfigureTime(re2)

FetchContent_Declare(
  c-ares
  GIT_REPOSITORY https://github.com/c-ares/c-ares.git
  GIT_TAG        b82840329a4081a1f1b125e6e6b760d4e1237b52
)

FetchContent_MakeHermetic(
  c-ares
  HERMETIC_TOOLCHAIN_EXTENSION [=[
    set(CARES_STATIC  ON CACHE BOOL "" FORCE)
    set(CARES_STATIC_PIC  ON CACHE BOOL "" FORCE)
    set(CARES_SHARED  OFF CACHE BOOL "" FORCE) 
    set(CARES_BUILD_TOOLS  OFF CACHE BOOL "" FORCE) 
  ]=]
  SBOM_LICENSE "MIT License"
  SBOM_SUPPLIER "Daniel Stenberg"

)

HermeticFetchContent_MakeAvailableAtConfigureTime(c-ares)

FetchContent_Declare(
  Protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG        2d4414f384dc499af113b5991ce3eaa9df6dd931
)

FetchContent_MakeHermetic(
  Protobuf
  HERMETIC_FIND_PACKAGES "ZLIB;absl"
  MAKE_EXECUTABLES_FINDABLE ON

  HERMETIC_TOOLCHAIN_EXTENSION [=[
    set(protobuf_BUILD_TESTS  OFF CACHE BOOL "" FORCE)
    set(protobuf_BUILD_SHARED_LIBS  OFF CACHE BOOL "" FORCE)
    set(protobuf_BUILD_SHARED_LIBS_DEFAULT  OFF CACHE BOOL "" FORCE)
    set(protobuf_FORCE_FETCH_DEPENDENCIES OFF CACHE BOOL "" FORCE)
    set(protobuf_LOCAL_DEPENDENCIES_ONLY ON CACHE BOOL "" FORCE)
  ]=]

  SBOM_LICENSE "BSD 3-Clause"
  SBOM_SUPPLIER "Google Inc"
)

HermeticFetchContent_MakeAvailableAtConfigureTime(Protobuf)

FetchContent_Declare(
  gRPC
  GIT_REPOSITORY https://github.com/grpc/grpc.git
  GIT_TAG 5e099002c1600c580ebe1e6741f8ff8b182ffea4
)

FetchContent_MakeHermetic(
  gRPC
  HERMETIC_FIND_PACKAGES "OpenSSL;ZLIB;re2;absl;c-ares;Protobuf"
  MAKE_EXECUTABLES_FINDABLE ON
  HERMETIC_TOOLCHAIN_EXTENSION [=[
    set(gRPC_INSTALL_default ON CACHE BOOL "" FORCE)
    set(gRPC_INSTALL ON CACHE BOOL "" FORCE)
    set(gRPC_BUILD_GRPC_CPP_PLUGIN ON CACHE BOOL "" FORCE)
    set(gRPC_BUILD_GRPC_CSHARP_PLUGIN OFF CACHE BOOL "" FORCE)
    set(gRPC_BUILD_GRPC_NODE_PLUGIN OFF CACHE BOOL "" FORCE)
    set(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN OFF CACHE BOOL "" FORCE)
    set(gRPC_BUILD_GRPC_PHP_PLUGIN OFF CACHE BOOL "" FORCE)
    set(gRPC_BUILD_GRPC_PYTHON_PLUGIN OFF CACHE BOOL "" FORCE)
    set(gRPC_BUILD_GRPC_RUBY_PLUGIN OFF CACHE BOOL "" FORCE)
    set(gRPC_DOWNLOAD_ARCHIVES OFF CACHE BOOL "" FORCE)
    set(gRPC_ZLIB_PROVIDER "package" CACHE STRING "" FORCE)
    set(gRPC_SSL_PROVIDER "package" CACHE STRING "" FORCE)
    set(gRPC_RE2_PROVIDER "package" CACHE STRING "" FORCE)
    set(gRPC_ABSL_PROVIDER "package" CACHE STRING "" FORCE)
    set(gRPC_CARES_PROVIDER "package" CACHE STRING "" FORCE)
    set(gRPC_PROTOBUF_PROVIDER "package" CACHE STRING "" FORCE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
  ]=]

  SBOM_LICENSE "Apache License 2.0"
  SBOM_SUPPLIER "Google Inc"
)

HermeticFetchContent_MakeAvailableAtConfigureTime(gRPC)

include(hfc_provide_dependency_FINDPACKAGE)

hfc_provide_dependency_FINDPACKAGE("FIND_PACKAGE" Protobuf)

set(Protobuf_CMAKE_MODULES_PATH "${PROTOBUF_ROOT_DIR}/lib/cmake/protobuf")
set(Protobuf_PROTOC_EXECUTABLE "$<TARGET_FILE:protobuf::protoc>")
list(APPEND CMAKE_MODULE_PATH "${Protobuf_CMAKE_MODULES_PATH}")
include(protobuf-generate)