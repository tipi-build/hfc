#define BOOST_TEST_MODULE check_subproject_targets_linking
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

  BOOST_DATA_TEST_CASE(check_subproject_targets_linking, boost::unit_test::data::make(test_variants()), data){
    fs::path template_path = prepare_project_to_be_tested("check_subproject_targets_linking",data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
  
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    run_command(cmake_configure_command, template_path);

    std::string cmake_build_command = get_cmake_build_command(template_path, data);
    run_command(cmake_build_command, template_path);

    BOOST_REQUIRE(fs::exists(template_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "project-cmake-nested-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "project-cmake-nested-install" / "lib" / "libMathFunctionscbrt.a"));

  }
}