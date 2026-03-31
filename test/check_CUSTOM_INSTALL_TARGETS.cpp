#define BOOST_TEST_MODULE check_CUSTOM_INSTALL_TARGETS
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>

#include <test_project.hpp>
#include <test_variant.hpp>
#include <test_helpers.hpp>
#include <test_isolation_fixture.hpp>

#include <pre/file/string.hpp>

 
namespace hfc::test { 
  namespace fs = boost::filesystem;
  namespace bp = boost::process;

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_CUSTOM_INSTALL_TARGETS_works, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_CUSTOM_INSTALL_TARGETS", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);
    write_simple_main(test_project_path,{"MathFunctions.h"});  
    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path, test_env);
    std::string cmake_build_command = get_cmake_build_command(test_project_path, data);
    run_command(cmake_build_command, test_project_path, test_env);

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-build" / "libMathFunctions.a"));
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "mathlib-build" / "libMathFunctionscbrt.a"));
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctionscbrt.a"));

  
  }

}