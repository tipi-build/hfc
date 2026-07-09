# Findiconv_lib.cmake
#
# Used only by the TEST_DATA_DEFER_VIA_ALIAS=ON variant of this template.
#
# When HERMETIC_DEFER_NATIVE_ROOTED_FIND_PACKAGE_FOR is requested via the alias
# name (HfcDependencyProvidedLib), HFC resolves it to the canonical name
# (iconv_lib), sets iconv_lib_ROOT to the HFC install prefix and forwards
# find_package(iconv_lib BYPASS_PROVIDER) to cmake's native implementation.
#
# mathlib links against HfcDependencyProvidedLib::HfcDependencyProvidedLib, so
# this module locates the library via iconv_lib_ROOT and defines that target.

find_path(iconv_lib_INCLUDE_DIR
    NAMES lib.h
    PATHS "${iconv_lib_ROOT}/include"
    NO_DEFAULT_PATH
)

find_library(iconv_lib_LIBRARY
    NAMES iconv
    PATHS "${iconv_lib_ROOT}/lib"
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(iconv_lib
    REQUIRED_VARS
        iconv_lib_LIBRARY
        iconv_lib_INCLUDE_DIR
)

if(iconv_lib_FOUND AND NOT TARGET HfcDependencyProvidedLib::HfcDependencyProvidedLib)
    add_library(HfcDependencyProvidedLib::HfcDependencyProvidedLib STATIC IMPORTED)
    set_target_properties(HfcDependencyProvidedLib::HfcDependencyProvidedLib PROPERTIES
        IMPORTED_LOCATION             "${iconv_lib_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${iconv_lib_INCLUDE_DIR}"
    )
endif()
