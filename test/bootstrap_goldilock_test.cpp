#define BOOST_TEST_MODULE bootstrap_goldilock
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

  using namespace std::literals;

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, bootstrap_goldilock_test, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("bootstrap_goldilock", data.is_cmake_re, temp_dir);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path, test_env);

    // Verify that goldilock was bootstrapped successfully
    BOOST_REQUIRE(fs::exists(test_project_path / "build"));
  }
}
