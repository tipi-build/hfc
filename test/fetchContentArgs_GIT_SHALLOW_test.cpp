#define BOOST_TEST_MODULE fetchContentArgs_GIT_SHALLOW_test
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

  struct td_struct_check_GIT_SHALLOW_args {
    bool success_expected;
    std::string cmake_flags;
  };

  static std::vector<td_struct_check_GIT_SHALLOW_args> TEST_DATA_clone_shallow = {
    { true, "-DHFCTEST_DATA_GIT_SHALLOW_VALUE=ON" },
    { false, "-DHFCTEST_DATA_GIT_SHALLOW_VALUE=OFF" },
    { false, "" },  // off by default...
  };

  // make it printable for boost test  
  static inline std::ostream& operator<<(std::ostream& os, const td_struct_check_GIT_SHALLOW_args& tc_data) {
    os << "test data set: success_expected='" << std::to_string(tc_data.success_expected) << "', cmake_flags='" << tc_data.cmake_flags << "'";
    return os;
  }


  BOOST_DATA_TEST_CASE(check_cmakelists_in_subfolder, 
    boost::unit_test::data::make(test_variants()) * boost::unit_test::data::make(TEST_DATA_clone_shallow),
    td_test_variant,
    td_clone_shallow
  ) {

    fs::path project_path = prepare_project_to_be_tested("fetchContentArgs_GIT_SHALLOW", td_test_variant.is_cmake_re);
    write_project_tipi_id(project_path);

    std::string cmake_configure_command = get_cmake_configure_command(project_path, td_test_variant, td_clone_shallow.cmake_flags);

    bool configure_success = false;

    try {
      run_command(cmake_configure_command, project_path); // the clone will be done after the configure command, no need to build for this one
      configure_success = true;
    }
    catch(...) {
      configure_success = false;
    }

    BOOST_REQUIRE(configure_success);

    // the project write a dependency_sourcedir.txt in the sources root, read that
    auto dependency_source_result_file = project_path / "build" / "dependency_sourcedir.txt";
    BOOST_REQUIRE(fs::exists(dependency_source_result_file));

    fs::path cloned_sources_path = pre::file::to_string(dependency_source_result_file.generic_string());
    BOOST_REQUIRE(fs::exists(cloned_sources_path));

    // check if the repo is a shallow clone 
    auto git_bin = bp::search_path("git");
    std::string check_is_shallow_cmd = git_bin.generic_string() + " rev-parse --is-shallow-repository"s;

    auto gitcmd_result = run_cmd(bp::start_dir=cloned_sources_path, bp::shell, check_is_shallow_cmd);

    // validate agains expected outcome
    bool is_shallow = (gitcmd_result.output == "true");
    BOOST_TEST_MESSAGE(gitcmd_result.output);
    BOOST_REQUIRE(is_shallow == td_clone_shallow.success_expected);
  }
}