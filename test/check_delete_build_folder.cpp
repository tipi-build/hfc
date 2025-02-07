#define BOOST_TEST_MODULE check_delete_build_folder
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

  struct td_struct_POST_BUILD_DATA_DELETION {
    std::string cmake_flags = "";
    bool expect_post_build_mathlib_BINARY_DIR_exists = true;
    bool expect_post_build_mathlib_SOURCE_DIR_exists = true;
  };

  static std::vector<td_struct_POST_BUILD_DATA_DELETION> TEST_DATA_hfc_delete_post_build = {
    /* remove BINARY_DIR and SOURCE_DIR  */
    { "-DHERMETIC_FETCHCONTENT_REMOVE_BUILD_DIR_AFTER_INSTALL=ON -DHERMETIC_FETCHCONTENT_REMOVE_SOURCE_DIR_AFTER_INSTALL=ON", false, false  },
    
    /* remove only BINARY_DIR */ 
    { "-DHERMETIC_FETCHCONTENT_REMOVE_BUILD_DIR_AFTER_INSTALL=ON -DHERMETIC_FETCHCONTENT_REMOVE_SOURCE_DIR_AFTER_INSTALL=OFF", false, true },
    { "-DHERMETIC_FETCHCONTENT_REMOVE_BUILD_DIR_AFTER_INSTALL=ON", false, true },

    /* remove only SOURCE_DIR */ 
    { "-DHERMETIC_FETCHCONTENT_REMOVE_BUILD_DIR_AFTER_INSTALL=OFF -DHERMETIC_FETCHCONTENT_REMOVE_SOURCE_DIR_AFTER_INSTALL=ON", true, false},
    { "-DHERMETIC_FETCHCONTENT_REMOVE_SOURCE_DIR_AFTER_INSTALL=ON", true, false },

    /* remove none (default too) */
    { "-DHERMETIC_FETCHCONTENT_REMOVE_BUILD_DIR_AFTER_INSTALL=OFF -DHERMETIC_FETCHCONTENT_REMOVE_SOURCE_DIR_AFTER_INSTALL=OFF", true, true },
    { "", true, true }
  };


  static auto TEST_DATA_make_mathlib_available_at = {
    "buildtime",
    "configuretime"
  };

  
  // make it printable for boost test  
  static inline std::ostream& operator<<(std::ostream& os, const td_struct_POST_BUILD_DATA_DELETION& tc_data) {
    os <<  "td_struct_POST_BUILD_DATA_DELETION: cmake_flags='" << tc_data.cmake_flags 
        << "', expect_post_build_mathlib_BINARY_DIR_exists=" << std::to_string(tc_data.expect_post_build_mathlib_BINARY_DIR_exists)
        << ", expect_post_build_mathlib_SOURCE_DIR_exists=" << std::to_string(tc_data.expect_post_build_mathlib_SOURCE_DIR_exists);
    return os;
  }


  BOOST_DATA_TEST_CASE(test_post_build_removal_feature, 
    boost::unit_test::data::make(hfc::test::test_variants()) * boost::unit_test::data::make(TEST_DATA_hfc_delete_post_build) * boost::unit_test::data::make(TEST_DATA_make_mathlib_available_at),
    td_test_variant,
    td_test_case,
    td_make_mathlib_available_at
  ) {

    fs::path project_path = prepare_project_to_be_tested("check_delete_sources_after_build", td_test_variant.is_cmake_re);
    write_project_tipi_id(project_path);

    // that's the same every time
    std::string cmake_build_command = get_cmake_build_command(project_path, td_test_variant);
  


    auto get_cmakeflags = [&](std::string injected_data) {
      return td_test_case.cmake_flags + " -DTEST_DATA_INJECTED="s + injected_data + " -DTEST_DATA_MAKE_AVAILABLE_AT="s + td_make_mathlib_available_at;
    };

    {
      std::string first_run_cmakeflags = get_cmakeflags("first");
      std::string first_configure_cmd = get_cmake_configure_command(project_path, td_test_variant, first_run_cmakeflags);
      bool first_configure_success = false;

      try {
        run_command(first_configure_cmd, project_path); // the clone will be done after the configure command, no need to build for this one
        first_configure_success = true;
      }
      catch(...) {
        first_configure_success = false;
      }

      BOOST_REQUIRE(first_configure_success);
      run_command(cmake_build_command, project_path);
    }

    fs::path expected_mathlib_binary_dir = project_path / "build" / "_deps" / "mathlib-build" / "CMakeCache.txt";
    fs::path expected_mathlib_source_dir = (td_test_variant.is_cmake_re) 
      ? get_mirror_test_project_path(project_path) / "thirdparty" / "cache" / "mathlib-ecc756a4-src" / "CMakeLists.txt"
      : project_path / "thirdparty" / "cache" / "mathlib-ecc756a4-src" / "CMakeLists.txt"
    ;

    BOOST_REQUIRE(fs::exists(expected_mathlib_source_dir) == td_test_case.expect_post_build_mathlib_SOURCE_DIR_exists);
    BOOST_REQUIRE(fs::exists(expected_mathlib_binary_dir) == td_test_case.expect_post_build_mathlib_BINARY_DIR_exists);

    // now reconfigure injecting the "second run testdata"
    // dependending on the input settings hfc will have to fetch the
    // dependency again to do a build
    // this way this test ensures that no matter the parameter combination
    // of HERMETIC_FETCHCONTENT_REMOVE_BUILD_DIR_AFTER_INSTALL // HERMETIC_FETCHCONTENT_REMOVE_SOURCE_DIR_AFTER_INSTALL
    // we wiil still manage to get the correct outcome
    {
      std::string second_run_cmakeflags = get_cmakeflags("second");
      std::string second_configure_cmd = get_cmake_configure_command(project_path, td_test_variant, second_run_cmakeflags);
      bool second_configure_success = false;

      try {
        run_command(second_configure_cmd, project_path); // the clone will be done after the configure command, no need to build for this one
        second_configure_success = true;
      }
      catch(...) {
        second_configure_success = false;
      }

      BOOST_REQUIRE(second_configure_success);
      run_command(cmake_build_command, project_path);
    }

    BOOST_REQUIRE(fs::exists(expected_mathlib_source_dir) == td_test_case.expect_post_build_mathlib_SOURCE_DIR_exists);
    BOOST_REQUIRE(fs::exists(expected_mathlib_binary_dir) == td_test_case.expect_post_build_mathlib_BINARY_DIR_exists);
  }

}