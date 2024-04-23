#define BOOST_TEST_MODULE check_find_package
#include <boost/test/included/unit_test.hpp> 

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>

#include <test_project.hpp>
#include <test_variant.hpp>
#include <test_helpers.hpp>

#include <pre/file/string.hpp>

#include <string>
#include <vector>

 
namespace hfc::test { 
  namespace fs = boost::filesystem;
  namespace bp = boost::process;

  using namespace std::literals;

  // negative test case to the above
  BOOST_DATA_TEST_CASE(check_find_package_neg, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_find_package", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);
    write_simple_main(test_project_path, {"MathFunctions.h", "MathFunctionscbrt.h", "lib.h"});

    auto result = run_cmd(bp::start_dir=(test_project_path), bp::shell, get_cmake_configure_command(test_project_path, data, "-DTEST_DATA_DISABLE_FIND_HfcDependencyProvidedLib=ON"));
    BOOST_REQUIRE_NE(result.return_code, 0); // no success please!
    
    // example output 
    // -- [HERMETIC_FETCHCONTENT 2024-11-27T21:06:30] Received find_package() request for package name HfcDependencyProvidedLib
    // -- [HERMETIC_FETCHCONTENT 2024-11-27T21:06:30]  - not in list of hermetic dependencies
    BOOST_REQUIRE(boost::regex_search(result.output, boost::regex{"-- \\[HERMETIC_FETCHCONTENT .+\\] Received find_package\\(\\) request for package name HfcDependencyProvidedLib"}));
    BOOST_REQUIRE(boost::regex_search(result.output, boost::regex{"-- \\[HERMETIC_FETCHCONTENT .+\\]  - not in list of hermetic dependencies"}));
  }

  BOOST_DATA_TEST_CASE(check_find_package, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_find_package", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);
    write_simple_main(test_project_path, { "MathFunctions.h", "MathFunctionscbrt.h", "lib.h" });

    std::string configure_output = run_command(get_cmake_configure_command(test_project_path, data), test_project_path);
    BOOST_REQUIRE(boost::contains(configure_output , "Received find_package() request for package name HfcDependencyProvidedLib"));
    BOOST_REQUIRE(boost::regex_search(configure_output, boost::regex{"-- \\[HERMETIC_FETCHCONTENT .+\\] Received find_package\\(\\) request for package name HfcDependencyProvidedLib"}));
    BOOST_REQUIRE(boost::regex_search(configure_output, boost::regex{"loading target cache from .*hermetic_targetcaches/HfcDependencyProvidedLib.cmake"}));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "HfcDependencyProvidedLib-install" / "lib" / "libiconv.a"));

    run_command(get_cmake_build_command(test_project_path, data), test_project_path);
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctionscbrt.a"));
  }
}