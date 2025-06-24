cmake_minimum_required(VERSION 3.27.6)
#cmake_minimum_required(VERSION 3.10.0)
#cmake_policy(SET CMP0126 OLD)  # option
#cmake_policy(SET CMP0077 OLD)  # option

cmake_policy(SET CMP0054 OLD)  # if() interprets variables inside quotes
cmake_policy(SET CMP0074 NEW)  # find_package() ought to search _ROOT variables

message("AN_OVERRIDABLE_OPTION BEFORE-TOOLCHAIN-LOADED SCOPE:'${AN_OVERRIDABLE_OPTION}' CACHE:'$CACHE{AN_OVERRIDABLE_OPTION}'")
project(check_options_with_tracked_default)
message("AN_OVERRIDABLE_OPTION AFTER-TOOLCHAIN-LOADED SCOPE:'${AN_OVERRIDABLE_OPTION}' CACHE:'$CACHE{AN_OVERRIDABLE_OPTION}'")

set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  ${CMAKE_MODULE_PATH}
)

toolchain_overridable_option(AN_OVERRIDABLE_OPTION "Documentation String" "option-in-project" TYPE STRING)
message("AN_OVERRIDABLE_OPTION FINAL SCOPE:'${AN_OVERRIDABLE_OPTION}' CACHE:'$CACHE{AN_OVERRIDABLE_OPTION}'")