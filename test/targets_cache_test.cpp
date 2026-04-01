#define BOOST_TEST_MODULE hfc_targets_cache
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>

#include <test_project.hpp>
#include <test_variant.hpp>
#include <test_helpers.hpp>

#include <pre/file/string.hpp>
#include <pre/file/hash.hpp>

#include <test_helpers.hpp>

#include <optional>

namespace hfc::test { 
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
  using namespace std::string_literals;

  struct targets_cache_test_data_set {
    std::string project_template;
    std::string hfc_content_name;
    bool targets_file_expected;
    bool expect_configure_success = true;
    bool expect_build_success = true;
    std::string additional_cmake_flags = "";
    bool inject_random_TESTDATA_INJECTED_value = false;
  };

  // make it printable for boost test  
  static inline std::ostream& operator<<(std::ostream& os, const targets_cache_test_data_set& tc_data) {
    os << "targets_cache_test_data_set: project_template='" << tc_data.project_template << "', "
      << "hfc_content_name='" << tc_data.hfc_content_name << "', "
      << "targets_file_expected='" << std::to_string(tc_data.targets_file_expected) << "', "
      << "expect_configure_success='" << tc_data.expect_configure_success << "', "
      << "expect_build_success='" << tc_data.expect_build_success << "', "
      << "additional_cmake_flags='" << tc_data.additional_cmake_flags << "'";
    return os;
  }


  static auto TEST_DATA_template_expectations = {
    targets_cache_test_data_set{ "hfc_targets_cache/FORCE_SYSTEM_findModuleforwarding", "Iconv.cmake", false, false, false }, // this needs to fail without FORCE_SYSTEM_Iconv set
    targets_cache_test_data_set{ "hfc_targets_cache/FORCE_SYSTEM_findModuleforwarding", "Iconv.cmake", true, false, false, "-DFORCE_SYSTEM_Iconv=OFF" },
    targets_cache_test_data_set{ "hfc_targets_cache/FORCE_SYSTEM_findModuleforwarding", "Iconv.cmake", true, true, true, "-DFORCE_SYSTEM_Iconv=ON", true /* inject_random_TESTDATA_INJECTED_value */  },
    targets_cache_test_data_set{ "hfc_targets_cache/from_cmake_package_config", "mathslib.cmake", true },
    targets_cache_test_data_set{ "hfc_targets_cache/cmake_export_declaration", "mathslib.cmake", true },
    targets_cache_test_data_set{ "hfc_targets_cache/autotools_export_declaration", "Iconv.cmake", true },
    targets_cache_test_data_set{ "hfc_targets_cache/cmake_no_install", "mathslib.cmake", false },
    targets_cache_test_data_set{ "hfc_targets_cache/alternate_exports_naming", "mathslib.cmake", true },
    targets_cache_test_data_set{ "hfc_targets_cache/alternate_exports_naming", "mathslib.cmake", false, false /* expect configure failure */, false, "-DHFCTEST_NEGATIVE_CASE=ON" /* enable the negative test in the project */ },

    // aliasing tests
    targets_cache_test_data_set{ "hfc_targets_cache/target_aliasing", "mathslib.cmake", true, true , true /* expect build and  configure success */, "-D=HFC_TEST_RENAMETARGET=OFF" }, 
    targets_cache_test_data_set{ "hfc_targets_cache/target_aliasing", "mathslib.cmake", true, true , true /* expect build and  configure success */, "-D=HFC_TEST_RENAMETARGET=ON" }, // rename instead of just adding an alias
    targets_cache_test_data_set{ "hfc_targets_cache/target_aliasing", "mathslib.cmake", true, true , true /* expect build and  configure success */, "-D=HFC_TEST_FORCE_SYSTEM_RENAME_TARGET=OFF" }, 
    targets_cache_test_data_set{ "hfc_targets_cache/target_aliasing", "mathslib.cmake", true, true , true /* expect build and  configure success */, "-D=HFC_TEST_FORCE_SYSTEM_RENAME_TARGET=ON" }, // rename instead of just adding an alias
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

    std::string TESTDATA_INJECTED_value = to_string(boost::uuids::random_generator()()); // gen it always even if we don't use it.

    auto get_configure_cmake_flags = [&]() {
      std::string result = td_makeAvailableAt_flag;

      if(td_targets_cache_test_data_set.additional_cmake_flags != "") {
        result += " "s + td_targets_cache_test_data_set.additional_cmake_flags;
      }

      if(td_targets_cache_test_data_set.inject_random_TESTDATA_INJECTED_value) {
        result += " -DTESTDATA_INJECTED="s + TESTDATA_INJECTED_value;
      }

      return result;
    };

    std::string cmake_configure_command = get_cmake_configure_command(project_path, td_test_variant, get_configure_cmake_flags());
    std::string cmake_build_command = get_cmake_build_command(project_path, td_test_variant);

    /* check we don't have leftovers... */
    BOOST_REQUIRE(is_empty_directory(project_path / "build"));

    auto project_dependency_targets_cache_file = project_path / "build" / "_deps" / "hermetic_targetcaches" / td_targets_cache_test_data_set.hfc_content_name;

    std::cout << "⚗️ [Configure]" << std::endl;
    bool configure_success = false;

    std::string configure_output = "";

    try {
      configure_output = run_command(cmake_configure_command, project_path);
      configure_success = true;
    }
    catch(...) {
      configure_success = false;
    }

    BOOST_REQUIRE(configure_success == td_targets_cache_test_data_set.expect_configure_success);

    if(!td_targets_cache_test_data_set.expect_configure_success) {


      std::cout << "⚗️ [SUCCESS by expected failure]" << std::endl;
      return;
    }

    // so... we expect that injected test data to appear during configure because that's the
    // most obvious choice for message() to be printed
    if(td_targets_cache_test_data_set.inject_random_TESTDATA_INJECTED_value) {
      BOOST_REQUIRE(boost::regex_search(configure_output, boost::regex{TESTDATA_INJECTED_value}));
    }


    if(td_targets_cache_test_data_set.targets_file_expected) {
      BOOST_REQUIRE(fs::exists(project_dependency_targets_cache_file));
      BOOST_REQUIRE(fs::is_regular_file(project_dependency_targets_cache_file));
    }
    else {
      BOOST_REQUIRE(!fs::exists(project_dependency_targets_cache_file));
    }

    std::cout << "⚗️ [Build]" << std::endl;

    bool build_success = false;
    try {
      run_command(cmake_build_command, project_path);
      build_success = true;
    }
    catch(...) {
      build_success = false;
    }

    BOOST_REQUIRE(build_success == td_targets_cache_test_data_set.expect_build_success);    
  }

} 
