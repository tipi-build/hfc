#define BOOST_TEST_MODULE hermetic_dependency_make_executable_findable
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>

#include <test_project.hpp>
#include <test_variant.hpp>

#include <pre/file/string.hpp>
#include <pre/file/hash.hpp>

#include <test_helpers.hpp>

namespace hfc::test { 
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
  using namespace std::string_literals;

  struct test_data_set {
    std::string project_template;
  };

  // make it printable for boost test  
  static inline std::ostream& operator<<(std::ostream& os, const test_data_set& tc_data) {
    os << "targets_cache_test_data_set: project_template='" << tc_data.project_template << "'";
    return os;
  }

  static auto TEST_DATA_template_expectations = {
    test_data_set{ "hermetic_dependency_make_executable_findable/autotool_dependency" },
    test_data_set{ "hermetic_dependency_make_executable_findable/cmake_dependency" }
  };

  static auto TEST_DATA_hfc_TEST_INPUT_MAKE_EXECUTABLE_FINDABLE = {
    "-DTEST_INPUT_MAKE_EXECUTABLE_FINDABLE=ON",
    "-DTEST_INPUT_MAKE_EXECUTABLE_FINDABLE=OFF",
    ""  /* no flag on purpose to test for default behavior */
  };

  BOOST_DATA_TEST_CASE(
    targets_cache_from_cmake_install_package_config, 
    boost::unit_test::data::make(hfc::test::test_variants()) * boost::unit_test::data::make(TEST_DATA_template_expectations) * boost::unit_test::data::make(TEST_DATA_hfc_TEST_INPUT_MAKE_EXECUTABLE_FINDABLE), 
    td_test_variant,
    td_data_set,
    td_data_cmake_variable_test_flag
  ){
    fs::path project_path = prepare_project_to_be_tested(td_data_set.project_template, td_test_variant.is_cmake_re);
    write_project_tipi_id(project_path);

    std::string cmake_configure_command = get_cmake_configure_command(project_path, td_test_variant, td_data_cmake_variable_test_flag);
    std::string cmake_build_command = get_cmake_build_command(project_path, td_test_variant);

    /* check we don't have leftovers... */
    BOOST_REQUIRE(is_empty_directory(project_path / "build"));

    std::cout << "⚗️ [Configure]" << std::endl;
    run_command(cmake_configure_command, project_path);

    std::cout << "⚗️ [Build]" << std::endl;
    run_command(cmake_build_command, project_path);
  }

} 
