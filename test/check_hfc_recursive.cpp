#define BOOST_TEST_MODULE check_hfc_recursive
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
  using namespace std::literals;

  struct test_data_set {
    std::string project_template;
    std::vector<std::string> includes_for_maincpp{};
    std::string additional_cmake_flags = "";
    bool expect_configure_success = true;
  };

  // make it printable for boost test  
  static inline std::ostream& operator<<(std::ostream& os, const test_data_set& tc_data) {
    os << "targets_cache_test_data_set: project_template='" << tc_data.project_template 
       << "' | additional_cmake_flags='" << tc_data.additional_cmake_flags 
       << "' | expect_configure_success='" << std::to_string(tc_data.expect_configure_success) 
       << "' | includes_for_maincpp=[" << boost::algorithm::join(tc_data.includes_for_maincpp, ", ") << "]";
    return os;
  }

  static auto TEST_DATA_cases = {
    test_data_set{ "hfc-recursive-cmake", { "More_MathFunctions.h" }, "" },
    test_data_set{ "hfc-recursive-autotools", { "print_hello_tipi.h" }, "" },
    test_data_set{ "hfc-recursive-cmake_autotools", { "Other_MathFunctions.h" }, "" },
    test_data_set{ "hfc-recursive-cmake-dependecy-aliases", { "MathFunctions.h" }, "-DTEST_PARAMETER_DEFINE_ALIAS=ON" },
    test_data_set{ "hfc-recursive-cmake-dependecy-aliases", { "MathFunctions.h" }, "-DTEST_PARAMETER_DEFINE_ALIAS=OFF" },
    test_data_set{ "hfc-recursive-cmake-dependecy-declaration-order", { "MathFunctions.h" }, "-DTEST_PARAMETER_RUN_NEGATIVE_CASE=ON", false /* expect configure to fail */ },
    test_data_set{ "hfc-recursive-cmake-dependecy-declaration-order", { "MathFunctions.h" }, "-DTEST_PARAMETER_RUN_NEGATIVE_CASE=OFF" }
  };

  BOOST_DATA_TEST_CASE(
    check_hfc_recursive_cmake_dependency_aliases, 
    boost::unit_test::data::make(hfc::test::test_variants()) * boost::unit_test::data::make(TEST_DATA_cases),    
    tc_variant,
    tc_case
  ) {
    fs::path test_project_path = prepare_project_to_be_tested(tc_case.project_template, tc_variant.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);
    
    append_random_testdata_marker_as_toolchain_comment(project_toolchain, tc_variant);
    write_simple_main(test_project_path, tc_case.includes_for_maincpp);

    // Note: the cmake files do a part of the actual test case success evaluation
    auto configure_command = get_cmake_configure_command(test_project_path, tc_variant, tc_case.additional_cmake_flags);

    if(tc_case.expect_configure_success) {
      run_command(configure_command, test_project_path);
    }
    else {
      bool configure_failed = false;
      try {
        run_command(configure_command, test_project_path);
      }
      catch(...) {
        configure_failed = true;
      }

      BOOST_REQUIRE(configure_failed);
      return; // success, stop here...
    }
    
    auto build_command = get_cmake_build_command(test_project_path, tc_variant);
    run_command(build_command, test_project_path);

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample"));
  }

}