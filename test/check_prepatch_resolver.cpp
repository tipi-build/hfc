#define BOOST_TEST_MODULE check_prepatch_resolver
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

  BOOST_DATA_TEST_CASE(check_that_resolver_is_used_and_origin_changed, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path template_path = prepare_project_to_be_tested("check_prepatch_resolver", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h", "MathFunctionsmultiplybytwo.h"});

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);    
    run_command(cmake_configure_command, template_path);
    run_command(get_cmake_build_command(template_path, data), template_path);

    BOOST_REQUIRE(fs::exists(template_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctionscbrt.a"));
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctionsmultiplybytwo.a"));


    if(data.is_cmake_re){
      fs::path mathlib_install_path = template_path / "build" / "_deps" / "mathlib-install";
      fs::path mathlib_install_path_no_symlink = fs::read_symlink(mathlib_install_path);
      BOOST_REQUIRE(boost::contains(mathlib_install_path_no_symlink.generic_string() ,"unit-test-cmake-template-2libs-simulation-prepatch"));
    }
  }
}