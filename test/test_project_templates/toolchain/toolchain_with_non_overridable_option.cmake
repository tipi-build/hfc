message("<START OF TOOLCHAIN FILE>")
include(${CMAKE_CURRENT_LIST_DIR}/environment/cmake/toolchain_overridable_option.cmake)

toolchain_overridable_option(AN_OVERRIDABLE_OPTION "Documentation string" "value-in-toolchain" TYPE STRING NON_OVERRIDABLE TOOLCHAIN_DEFAULT)
toolchain_overridable_option(CMAKE_CXX_STANDARD "C++ Standard (toolchain)" 20 TYPE STRING NON_OVERRIDABLE TOOLCHAIN_DEFAULT)
message("</END OF TOOLCHAIN FILE>")