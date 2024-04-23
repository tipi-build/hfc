#define BOOST_TEST_MODULE check_compiler_forwarding
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

  fs::path prepare_test_project(const std::string& template_name, const hfc::test::test_variant& test_data, const fs::path& clang_path) {
    fs::path test_project_path = prepare_project_to_be_tested(template_name, test_data.is_cmake_re);        
    fs::path toolchain_file = get_project_toolchain_path(test_project_path);

    std::string toolchain_content = pre::file::to_string(toolchain_file.generic_string());
    toolchain_content = regex_replace_all(toolchain_content, "{toolchain_placeholder_CMAKE_C_COMPILER}", clang_path.string());  // set the compiler to clang
    toolchain_content = regex_replace_all(toolchain_content, "{toolchain_placeholder_CMAKE_C_COMPILER_additional_params}", "CACHE STRING \"\" FORCE");
    toolchain_content = regex_replace_all(toolchain_content, "#<toolchain_activate_CMAKE_C_COMPILER>", "");  // uncomment that line

    pre::file::from_string(toolchain_file.generic_string(), toolchain_content);
    
    append_random_testdata_marker_as_toolchain_comment(toolchain_file, test_data);

    return test_project_path;
  }

   BOOST_DATA_TEST_CASE(check_compiler_forwarding_cmake, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path clang_path = bp::search_path("clang");
    BOOST_REQUIRE(clang_path.generic_string() != "");

    fs::path test_project_path = prepare_test_project("check_compiler_forwarding", data, clang_path);
    write_simple_main(test_project_path, {"MathFunctions.h", "MathFunctionscbrt.h"});
    
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctionscbrt.a"));

    run_command(get_cmake_configure_command(test_project_path, data), test_project_path);

    // check that the CMakeCache files of the library AND the dependency contain the expected entry
    // NOTE: checking this after configure because HFC will delete the build tree after a successful install
    std::string cmake_cache_clang_path_str = "CMAKE_C_COMPILER:STRING=" + clang_path.string();
    BOOST_REQUIRE(boost::contains(pre::file::to_string((test_project_path / "build" / "CMakeCache.txt").generic_string()), cmake_cache_clang_path_str));
    BOOST_REQUIRE(boost::contains(pre::file::to_string((test_project_path / "build" / "_deps" / "mathlib-build" / "CMakeCache.txt").generic_string()), cmake_cache_clang_path_str));

    run_command(get_cmake_build_command(test_project_path, data), test_project_path);

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctionscbrt.a"));
    
    remove_build_folder(data.is_cmake_re, test_project_path, "mathlib");
  }

  BOOST_DATA_TEST_CASE(check_compiler_forwarding_cmake_change_basedir, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path clang_path = bp::search_path("clang");
    BOOST_REQUIRE(clang_path.generic_string() != "");

    fs::path test_project_path = prepare_test_project("check_compiler_forwarding", data, clang_path);
    write_simple_main(test_project_path, {"MathFunctions.h", "MathFunctionscbrt.h"});
    
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(!fs::exists(test_project_path / "my-deps-cache"  / "mathlib-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(!fs::exists(test_project_path / "my-deps-cache" / "mathlib-install" / "lib" / "libMathFunctionscbrt.a"));

    auto HERMETIC_FETCHCONTENT_INSTALL_DIR= (test_project_path / "my-deps-cache"s).string(); 
    run_command(get_cmake_configure_command(test_project_path, data) + " -DHERMETIC_FETCHCONTENT_INSTALL_DIR="s + HERMETIC_FETCHCONTENT_INSTALL_DIR, test_project_path);

    // check that the CMakeCache files of the library AND the dependency contain the expected entry
    // NOTE: checking this after configure because HFC will delete the build tree after a successful install
    std::string cmake_cache_clang_path_str = "CMAKE_C_COMPILER:STRING=" + clang_path.string();
    BOOST_REQUIRE(boost::contains(pre::file::to_string((test_project_path / "build" / "CMakeCache.txt").generic_string()), cmake_cache_clang_path_str));
    BOOST_REQUIRE(boost::contains(pre::file::to_string((test_project_path / "build" / "_deps" / "mathlib-build" / "CMakeCache.txt").generic_string()), cmake_cache_clang_path_str));

    run_command(get_cmake_build_command(test_project_path, data), test_project_path);

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "my-deps-cache" / "mathlib-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(test_project_path / "my-deps-cache" / "mathlib-install" / "lib" / "libMathFunctionscbrt.a"));
    // Install should only be in the set FETCHCONTENT_INSTALL_DIR
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctionscbrt.a"));
    // Build tree should be in FETCHCONTENT_BASE_DIR
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-build-ep" / "CMakeFiles" / "hfc_content_build_mathlib-complete"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-build" / "libMathFunctionscbrt.a"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-build" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-build" / "build.ninja"));
    
    remove_build_folder(data.is_cmake_re, test_project_path, "mathlib");
  }


  BOOST_DATA_TEST_CASE(check_compiler_forwarding_autotools, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path clang_path = bp::search_path("clang");
    BOOST_REQUIRE(clang_path.generic_string() != "");

    fs::path test_project_path = prepare_test_project("check_compiler_forwarding_autotools", data, clang_path);    
    write_simple_main(test_project_path, { "lib.h" });

    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "Iconv-install" / "lib" / "libiconv.a"  ));

    std::string config_output = run_command(get_cmake_configure_command(test_project_path, data), test_project_path);
    std::string build_output = run_command(get_cmake_build_command(test_project_path, data), test_project_path);

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "Iconv-install" / "lib" / "libiconv.a"  ));

    // check that CC was correctly set in the makefiles & cmakecache
    std::string cmake_cache_clang_path_str = "CMAKE_C_COMPILER:STRING=" + clang_path.string();
    BOOST_REQUIRE(boost::contains(pre::file::to_string((test_project_path / "build" / "CMakeCache.txt").generic_string()), cmake_cache_clang_path_str));

    std::string makefile_CC_variable_line = "CC = "s + clang_path.string();    
    BOOST_REQUIRE(boost::contains(pre::file::to_string((test_project_path / "build" / "_deps" / "iconv-build" / "src" / "Makefile").generic_string()), makefile_CC_variable_line));
    
    remove_build_folder(data.is_cmake_re, test_project_path, "iconv");
    //fs::create_directories(template_path / "build");
  }
}