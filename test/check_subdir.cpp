#define BOOST_TEST_MODULE check_subdir
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

  BOOST_DATA_TEST_CASE(check_cmakelists_in_subfolder, boost::unit_test::data::make(test_variants()), data) {

    fs::path test_project_path = prepare_project_to_be_tested("check_subdir", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    auto check_that_expected_binaries_were_built = [&]() {
      BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MySimpleMain" ));
      BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-install" / "lib" / "libMathFunctions.a"));
      BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-install" / "lib" / "libMathFunctionscbrt.a"));
    };

    bp::environment test_environment = boost::this_process::environment();
    test_environment["TIPI_DISABLE_SET_MTIME"] = "ON";
    auto cmake_run_configure = [&]() {
      std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
      return run_command(cmake_configure_command, test_project_path, test_environment);
    };

    auto cmake_run_build = [&]() {
      std::string cmake_build_command = get_cmake_build_command(test_project_path, data, "-d explain");
      return run_command(cmake_build_command, test_project_path, test_environment);
    };

    write_simple_main(test_project_path,{}, "simple_main.cpp");

    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);

    // First configure + build, Expectation : everything builds from source
    {
      cmake_run_configure();
      cmake_run_build();

      check_that_expected_binaries_were_built();
    }

  }
}