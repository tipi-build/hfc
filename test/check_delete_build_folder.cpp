#define BOOST_TEST_MODULE check_delete_build_folder
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>

#include <test_project.hpp>
#include <test_variant.hpp>
#include <test_helpers.hpp>

#include <pre/file/string.hpp>

 
namespace hfc::test { 
  namespace fs = boost::filesystem;
  namespace bp = boost::process;

  BOOST_DATA_TEST_CASE(check_if_we_can_remove_build_and_source_folder, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_install-reuse", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);
    write_simple_main(test_project_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(test_project_path,{}, "simple_main.cpp");

    auto content = pre::file::to_string(project_toolchain.generic_string());
    content += "\n  set(HERMETIC_FETCHCONTENT_REMOVE_BUILD_DIR_AFTER_INSTALL ON CACHE BOOL \"\") \n set(HERMETIC_FETCHCONTENT_REMOVE_SOURCE_DIR_AFTER_INSTALL ON CACHE BOOL \"\")\n";
    pre::file::from_string(project_toolchain.generic_string(), content);
    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path);
    std::string cmake_build_command = get_cmake_build_command(test_project_path, data);
    run_command(cmake_build_command, test_project_path);

    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-build" / "CMakeCache.txt"));
    if(!data.is_cmake_re){
      BOOST_REQUIRE(!fs::exists(test_project_path / "thirdparty" / "cache" / "project-cmake-simple-ecc756a4-src" / "CMakeLists.txt"));
    }else if(data.is_cmake_re){
      BOOST_REQUIRE(!fs::exists(get_mirror_test_project_path(test_project_path) / "thirdparty" / "cache" / "project-cmake-simple-ecc756a4-src" / "CMakeLists.txt" ));
    } 
  }

  BOOST_DATA_TEST_CASE(check_if_we_can_keep_build_and_source_folder, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_install-reuse", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);
    write_simple_main(test_project_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(test_project_path,{}, "simple_main.cpp");

    auto content = pre::file::to_string(project_toolchain.generic_string());
    content += "\n  set(HERMETIC_FETCHCONTENT_REMOVE_BUILD_DIR_AFTER_INSTALL OFF CACHE BOOL \"\") \n set(HERMETIC_FETCHCONTENT_REMOVE_SOURCE_DIR_AFTER_INSTALL OFF CACHE BOOL \"\")\n";
    pre::file::from_string(project_toolchain.generic_string(), content);
    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path);
    std::string cmake_build_command = get_cmake_build_command(test_project_path, data);
    run_command(cmake_build_command, test_project_path);

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-build" / "CMakeCache.txt"));
    if(!data.is_cmake_re){
      BOOST_REQUIRE(fs::exists(test_project_path / "thirdparty" / "cache" / "project-cmake-simple-ecc756a4-src" / "CMakeLists.txt"));
    }else if(data.is_cmake_re){
      BOOST_REQUIRE(fs::exists(get_mirror_test_project_path(test_project_path) / "thirdparty" / "cache" / "project-cmake-simple-ecc756a4-src" / "CMakeLists.txt" ));
    } 
  }

  BOOST_DATA_TEST_CASE(check_if_we_can_keep_build_and_source_folder_without_define_value, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_install-reuse", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);
    write_simple_main(test_project_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(test_project_path,{}, "simple_main.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path);
    std::string cmake_build_command = get_cmake_build_command(test_project_path, data);
    run_command(cmake_build_command, test_project_path);

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-build" / "CMakeCache.txt"));
    if(!data.is_cmake_re){
      BOOST_REQUIRE(fs::exists(test_project_path / "thirdparty" / "cache" / "project-cmake-simple-ecc756a4-src" / "CMakeLists.txt"));
    }else if(data.is_cmake_re){
      BOOST_REQUIRE(fs::exists(get_mirror_test_project_path(test_project_path) / "thirdparty" / "cache" / "project-cmake-simple-ecc756a4-src" / "CMakeLists.txt" ));
    } 
  }
}