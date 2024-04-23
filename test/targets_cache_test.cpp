#define BOOST_TEST_MODULE hfc_targets_cache
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>

#include <test_project.hpp>
#include <test_variant.hpp>
#include <test_helpers.hpp>

#include <pre/file/string.hpp>
#include <pre/file/hash.hpp>

#include <test_helpers.hpp>

namespace hfc::test { 
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
  using namespace std::string_literals;

  struct targets_cache_test_data_set {
    std::string project_template;
    std::string hfc_content_name;
    bool targets_file_expected;
  };

  // make it printable for boost test  
  static inline std::ostream& operator<<(std::ostream& os, const targets_cache_test_data_set& tc_data) {
    os << "targets_cache_test_data_set: project_template='" << tc_data.project_template << "', hfc_content_name='" << tc_data.hfc_content_name << "', targets_file_expected='" << std::to_string(tc_data.targets_file_expected) << "'";
    return os;
  }


  static auto TEST_DATA_template_expectations = {
    targets_cache_test_data_set{ "hfc_targets_cache/from_cmake_package_config", "mathslib.cmake", true },
    targets_cache_test_data_set{ "hfc_targets_cache/cmake_export_declaration", "mathslib.cmake", true },
    targets_cache_test_data_set{ "hfc_targets_cache/autotools_export_declaration", "Iconv.cmake", true },
    targets_cache_test_data_set{ "hfc_targets_cache/cmake_no_install", "mathslib.cmake", false },
  };

  static auto TEST_DATA_hfc_makeAvailableAt_type = {
    "-DHFCTEST_BUILDTIME_DEPENDENCY=ON",
    "-DHFCTEST_CONFIGURETIME_DEPENDENCY=ON",
  };


  BOOST_DATA_TEST_CASE(
    targets_cache_from_cmake_install_package_config, 
    boost::unit_test::data::make(hfc::test::test_variants()) * boost::unit_test::data::make(TEST_DATA_hfc_makeAvailableAt_type) * boost::unit_test::data::make(TEST_DATA_template_expectations), 
    td_test_variant,
    td_makeAvailableAt_flag,
    td_targets_cache_test_data_set
  ){
    fs::path project_path = prepare_project_to_be_tested(td_targets_cache_test_data_set.project_template, td_test_variant.is_cmake_re);
    write_project_tipi_id(project_path);

    std::string cmake_configure_command = get_cmake_configure_command(project_path, td_test_variant, td_makeAvailableAt_flag);
    std::string cmake_build_command = get_cmake_build_command(project_path, td_test_variant);

    /* check we don't have leftovers... */
    BOOST_REQUIRE(is_empty_directory(project_path / "build"));

    auto project_dependency_targets_cache_file = project_path / "build" / "_deps" / "hermetic_targetcaches" / td_targets_cache_test_data_set.hfc_content_name;

    std::cout << "⚗️ [Configure]" << std::endl;
    run_command(cmake_configure_command, project_path);

    if(td_targets_cache_test_data_set.targets_file_expected) {
      BOOST_REQUIRE(fs::exists(project_dependency_targets_cache_file));
      BOOST_REQUIRE(fs::is_regular_file(project_dependency_targets_cache_file));
    }
    else {
      BOOST_REQUIRE(!fs::exists(project_dependency_targets_cache_file));
    }

    std::cout << "⚗️ [Build]" << std::endl;
    run_command(cmake_build_command, project_path);
  }

} 
