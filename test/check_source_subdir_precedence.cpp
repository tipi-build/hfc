#define BOOST_TEST_MODULE check_source_subdir_precedence
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

  // When SOURCE_SUBDIR is specified, HFC must add ONLY the CMakeLists.txt found
  // in that subdirectory and never fall back to (or prefer) the source tree root.
  //
  // The pinned commit of the dependency repo contains BOTH:
  //  - a decoy root CMakeLists.txt that message(FATAL_ERROR ...)s if it is ever add_subdirectory()'d, and
  //  - the real, buildable project under build/cmake (the declared SOURCE_SUBDIR)
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, source_subdir_takes_precedence_over_root, boost::unit_test::data::make(test_variants()), data) {
    fs::path test_project_path = prepare_project_to_be_tested("check_source_subdir_precedence", data.is_cmake_re, temp_dir);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    auto check_that_expected_binaries_were_built = [&]() {
      BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MySimpleMain" ));
    };

    test_env["TIPI_DISABLE_SET_MTIME"] = "ON";
    auto cmake_run_configure = [&]() {
      std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
      return run_command(cmake_configure_command, test_project_path, test_env);
    };

    auto cmake_run_build = [&]() {
      std::string cmake_build_command = get_cmake_build_command(test_project_path, data, "-d explain");
      return run_command(cmake_build_command, test_project_path, test_env);
    };

    write_simple_main(test_project_path,{}, "simple_main.cpp");
    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);

    {
      cmake_run_configure();
      cmake_run_build();
      check_that_expected_binaries_were_built();
    }
  }
}
