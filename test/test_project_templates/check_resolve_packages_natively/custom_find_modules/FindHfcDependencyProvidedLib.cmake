# FindHfcDependencyProvidedLib.cmake
#
# Locates HfcDependencyProvidedLib using HfcDependencyProvidedLib_ROOT, which
# is set by HFC's HERMETIC_DEFER_NATIVE_ROOTED_FIND_PACKAGE_FOR mechanism
# before forwarding to native find_package().
#
# This is the cmake Find module that a real-world project (e.g. Arrow) would
# normally ship or that the integrating project provides.  It demonstrates
# that HERMETIC_DEFER_NATIVE_ROOTED_FIND_PACKAGE_FOR correctly sets _ROOT
# so that a standard cmake Find module can locate HFC-built dependencies.
#
# Defines:
#   HfcDependencyProvidedLib_FOUND
#   HfcDependencyProvidedLib::HfcDependencyProvidedLib  (imported target)

find_path(HfcDependencyProvidedLib_INCLUDE_DIR
    NAMES lib.h
    PATHS "${HfcDependencyProvidedLib_ROOT}/include"
    NO_DEFAULT_PATH
)

find_library(HfcDependencyProvidedLib_LIBRARY
    NAMES iconv
    PATHS "${HfcDependencyProvidedLib_ROOT}/lib"
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HfcDependencyProvidedLib
    REQUIRED_VARS
        HfcDependencyProvidedLib_LIBRARY
        HfcDependencyProvidedLib_INCLUDE_DIR
)

if(HfcDependencyProvidedLib_FOUND AND NOT TARGET HfcDependencyProvidedLib::HfcDependencyProvidedLib)
    add_library(HfcDependencyProvidedLib::HfcDependencyProvidedLib STATIC IMPORTED)
    set_target_properties(HfcDependencyProvidedLib::HfcDependencyProvidedLib PROPERTIES
        IMPORTED_LOCATION             "${HfcDependencyProvidedLib_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${HfcDependencyProvidedLib_INCLUDE_DIR}"
    )
endif()
