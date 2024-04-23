cmake_minimum_required(VERSION 3.27.0)
project(test_hfc_cmake_targets_discover_overrides LANGUAGES NONE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

set(HERMETIC_FETCHCONTENT_RUN_INTERNAL_DISCOVER_TARGETS_TEST ON)
include(hfc_cmake_targets_discover)
