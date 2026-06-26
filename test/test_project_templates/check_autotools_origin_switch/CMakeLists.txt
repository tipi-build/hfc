# Placeholder. check_autotools_origin_switch.cpp generates the real consumer
# CMakeLists.txt for each origin (original then fork), pointing at the real
# autotools sample. This file only exists so the template directory is valid
# before the test rewrites it.
cmake_minimum_required(VERSION 3.27.6)
project(AutotoolsOriginSwitch LANGUAGES CXX)
