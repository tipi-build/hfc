include(${CMAKE_CURRENT_LIST_DIR}/hfc_toolchain_fingerprint.cmake)

get_property(root_toolchain_fingerprint_computed GLOBAL PROPERTY HERMETIC_FETCHCONTENT_BOOTSTRAP_PROJECT_TOOLCHAIN_FINGERPRINT SET)

if(NOT root_toolchain_fingerprint_computed)

    compute_live_toolchain_fingerprint(boostrap_toolchain_hash)
    message(STATUS "HFC bootstrap toolchain fingerprint: ${boostrap_toolchain_hash}")
    set_property(GLOBAL PROPERTY HERMETIC_FETCHCONTENT_BOOTSTRAP_PROJECT_TOOLCHAIN_FINGERPRINT "${boostrap_toolchain_hash}")
    unset(boostrap_toolchain_hash)

endif()

unset(root_toolchain_fingerprint_computed)