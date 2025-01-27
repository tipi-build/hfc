include(FetchContent)
include(HermeticFetchContent)

FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/tipi-build/boost
    GIT_TAG        eab5b4b199f57aee86a4dc64cf3682b970507e09  # tag boost-1.85.0
)

FetchContent_MakeHermetic(
  Boost
  HERMETIC_TOOLCHAIN_EXTENSION 
  [=[
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE) 
    set(BOOST_IOSTREAMS_ENABLE_LZMA OFF CACHE BOOL "" FORCE) 
    set(BOOST_IOSTREAMS_ENABLE_ZSTD OFF CACHE BOOL "" FORCE)
    set(BOOST_ENABLE_PYTHON OFF CACHE BOOL "" FORCE)
  ]=]
)

HermeticFetchContent_MakeAvailableAtBuildTime(Boost)

find_package(Threads)
add_compile_definitions(BOOST_ALL_NO_LIB=1)

add_executable(goldilock "${CMAKE_CURRENT_LIST_DIR}/fake_goldilock.cpp")

add_library(hfc_project INTERFACE)
add_library(hfc_project::test_lib ALIAS hfc_project)


target_include_directories(hfc_project INTERFACE 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/>
)

target_link_libraries(hfc_project INTERFACE     
  Boost::system 
  Boost::filesystem 
  Boost::uuid 
  Boost::included_unit_test_framework
  Boost::process 
  Boost::asio
  Threads::Threads
)

if(HFC_TEST_SHARED_TOOLS_DIR)
    message(STATUS "Using cached HFC_TEST_SHARED_TOOLS_DIR value '${HFC_TEST_SHARED_TOOLS_DIR}'")
else()
    set(HFC_TEST_SHARED_TOOLS_DIR "${HERMETIC_FETCHCONTENT_TOOLS_DIR}")
    message(STATUS "Using HFC_TEST_SHARED_TOOLS_DIR value '${HFC_TEST_SHARED_TOOLS_DIR}'")
endif()

function(add_test_with_HFC_TOOLS_DIR_ENV test_name source_files env_value)
    message("Registering test ${test_name} (${source_files})")
    add_executable(${test_name} "${source_files}")
    set_target_properties(${test_name} PROPERTIES OUTPUT_NAME ${test_name})

    target_link_libraries(${test_name} hfc_project::test_lib)
    
    add_test(
        NAME ${test_name} 
        COMMAND "$<TARGET_FILE:${test_name}>"
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../   # wd = project root      
    )

    # add to all tests
    set_property(TEST ${test_name} PROPERTY LABELS "ALL")

    # ctest will set HFC_TEST_SHARED_TOOLS_DIR=<$env_value> the value defined above when running the tests
    if(NOT env_value STREQUAL "")
        set_property(TEST ${test_name} PROPERTY ENVIRONMENT "HFC_TEST_SHARED_TOOLS_DIR=${env_value};BOOST_TEST_LOG_LEVEL=all")
    endif()

endfunction()

# bootstrap goldilock
add_test_with_HFC_TOOLS_DIR_ENV(ensure_goldilock "${CMAKE_CURRENT_LIST_DIR}/boostrap_goldilock_test.cpp" "${HFC_TEST_SHARED_TOOLS_DIR}")
set_property(TEST ensure_goldilock PROPERTY RUN_SERIAL ON) # run first and not in parallel

# forced goldilock bootstrapping
#add_test_with_HFC_TOOLS_DIR_ENV(bootstrap_goldilock_forced "${CMAKE_CURRENT_LIST_DIR}/boostrap_goldilock_test.cpp" "") 

# all other tests
# -> those all use the HFC_TEST_SHARED_TOOLS_DIR to avoid re-building goldilock a thousand times over
set(test_source_files
    "${CMAKE_CURRENT_LIST_DIR}/check_PATCH_COMMAND.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/check_compiler_forwarding.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/check_options_and_defines_forwarding.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/check_subdir.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/check_install_tree_cacheability.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/check_subproject_targets_linking.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/check_without_install.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/check_openssl_build_system.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/check_install_without_configure.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/check_hfc_recursive.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/run_hfc_discover_targets_overridest_test.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/check_find_package.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/configure_stability_test.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/targets_cache_test.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/hermetic_dependency_make_executable_findable.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/goldilock_provisioning_test.cpp"
)

foreach(test_file IN LISTS test_source_files)
    get_filename_component(test_name ${test_file} NAME_WE) # without .cpp extension
    add_test_with_HFC_TOOLS_DIR_ENV(${test_name} "${test_file}" "${HFC_TEST_SHARED_TOOLS_DIR}")    
endforeach()

set_tests_properties (goldilock_provisioning_test  PROPERTIES
    ENVIRONMENT "FAKE_GOLDILOCK=$<TARGET_FILE:goldilock>;TRUE_GOLDILOCK=${HERMETIC_FETCHCONTENT_goldilock_BIN};STANDARD_COMPILER=${CMAKE_CXX_COMPILER}")
set_tests_properties(check_find_package PROPERTIES WILL_FAIL TRUE)
set_tests_properties(configure_stability_test PROPERTIES WILL_FAIL TRUE)