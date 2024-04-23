#define BOOST_TEST_MODULE check_compiler
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>

#include <test_project.hpp>
#include <test_helpers.hpp>
#include <pre/file/string.hpp>

 
namespace hfc::test { 

  namespace fs = boost::filesystem;
  namespace bp = boost::process;

  BOOST_AUTO_TEST_CASE(hfc_cmake_targets_discover_overrides_behavior){
    fs::path template_path = prepare_project_to_be_tested("hfc_cmake_targets_discover_overrides_test", false);
    fs::path cmake_binary = bp::search_path("cmake");

    std::string cmake_configure_command = cmake_binary.generic_string() + " -B " + (template_path / "build").string();
    int result_configure = bp::system(bp::start_dir=(template_path), cmake_configure_command ,  bp::std_out > stdout, bp::std_err > stderr, bp::std_in < stdin);
    BOOST_REQUIRE(result_configure == 0 );
  }

}