#define BOOST_TEST_MODULE check_PATCH_COMMAND
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
  using namespace std::string_literals;

  inline void remove_build_folder(bool is_cmake_re, fs::path template_path, std::string lib_name){
    if(is_cmake_re){
      for (const auto& entry : fs::directory_iterator{template_path / "build" / "_deps" / (lib_name+"-build") }){
        fs::remove_all(entry.path());
      }

      for (const auto& entry : fs::directory_iterator{template_path / "build"}){
        fs::remove_all(entry.path());
      }
    }
    fs::remove_all(template_path / "build");
  }

   BOOST_DATA_TEST_CASE(check_PATCH_COMMAND_cmake, boost::unit_test::data::make(hfc::test::test_variants()), data){

    fs::path test_project_path = prepare_project_to_be_tested("check_PATCH_COMMAND", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    write_simple_main(test_project_path, {"MathFunctionscbrt-MOVED-BY-PATCH_COMMAND.h"});
    
    BOOST_REQUIRE(!fs::exists(test_project_path / "thirdparty" / "cache" / "mathlib-ecc756a4-src" / "MathFunctionscbrt-MOVED-BY-PATCH_COMMAND.h" ));
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "include" / "MathFunctionscbrt-MOVED-BY-PATCH_COMMAND.h"));

    run_command(get_cmake_configure_command(test_project_path, data), test_project_path);

    run_command(get_cmake_build_command(test_project_path, data), test_project_path);

    if ( !data.is_cmake_re ) {
      BOOST_REQUIRE(fs::exists(test_project_path / "thirdparty" / "cache" / "mathlib-ecc756a4-src" / "MathFunctionscbrt-MOVED-BY-PATCH_COMMAND.h" ));
    }
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "include" / "MathFunctionscbrt-MOVED-BY-PATCH_COMMAND.h"));
    
    remove_build_folder(data.is_cmake_re, test_project_path, "mathlib");


    run_command(get_cmake_configure_command(test_project_path, data), test_project_path);

    // Check that it is still patched after reconfigure
    if ( !data.is_cmake_re ) { // in cmake-re it is in mirror
      BOOST_REQUIRE(fs::exists(test_project_path / "thirdparty" / "cache" / "mathlib-ecc756a4-src" / "MathFunctionscbrt-MOVED-BY-PATCH_COMMAND.h" ));
    }

    run_command(get_cmake_build_command(test_project_path, data), test_project_path);

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "include" / "MathFunctionscbrt-MOVED-BY-PATCH_COMMAND.h"));

    remove_build_folder(data.is_cmake_re, test_project_path, "mathlib");
  }

}