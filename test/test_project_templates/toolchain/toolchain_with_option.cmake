message("<START OF TOOLCHAIN FILE>")
include(${CMAKE_CURRENT_LIST_DIR}/environment/cmake/toolchain_overridable_option.cmake)

toolchain_overridable_option(AN_OVERRIDABLE_OPTION "Documentation string" "value-in-toolchain" TYPE STRING TOOLCHAIN_DEFAULT)
message("</END OF TOOLCHAIN FILE>")