#define BOOST_TEST_MODULE check_bypass_provider_test
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
  using namespace std::string_literals;


  // ============================================================================
  // Test: HERMETIC_FETCHCONTENT_BYPASS_PROVIDER_FOR_PACKAGES correctness
  //
  // Scenario: A dependency (iconv_lib) exports a target whose
  // INTERFACE_LINK_LIBRARIES contains "Threads::Threads". When the parent
  // project consumes this target cache, hfc_targets_cache_consume processes
  // INTERFACE_LINK_LIBRARIES and triggers find_package(Threads QUIET) through
  // the dependency provider.
  //
  // Expected behavior: Threads is in BYPASS_PROVIDER_FOR_PACKAGES, so it must
  // be resolved via native find_package(Threads BYPASS_PROVIDER) and NOT
  // consumed into the HFC target cache system.
  //
  // Bug manifestation: Threads gets incorrectly consumed (registered as an HFC
  // dependency with wrong install prefix / cache file data), which:
  //   - Corrupts the summary file (Threads appears in consumed_contents)
  //   - On reconfigure, stale CACHE entries cause find_package(Threads) to
  //     take the wrong code path in hfc_provide_dependency_FINDPACKAGE
  //   - Downstream deps that consume the summary file get wrong Threads data
  // ============================================================================
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_bypass_provider_fresh_configure,
      boost::unit_test::data::make(hfc::test::test_variants()), data) {

    fs::path project_path = prepare_project_to_be_tested("check_bypass_provider", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, { "MathFunctions.h", "lib.h" }, "simple_example.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(project_path, data);
    std::string cmake_build_command = get_cmake_build_command(project_path, data);

    auto project_binary = project_path / "build" / "MyExample";
    auto mathlib_install_dir = project_path / "build" / "_deps" / "mathlib-install";
    auto iconv_install_dir = project_path / "build" / "_deps" / "iconv_lib-install";
    auto mathlib_summary = project_path / "build" / "_deps" / "hermetic_targetcaches" / "mathlib.hfcsummary.cmake";

    // 1. Fresh configure + build
    std::cout << "[Fresh Configure]" << std::endl;
    auto result = run_cmd(test_env, bp::start_dir=(project_path), bp::shell, cmake_configure_command);
    BOOST_TEST_MESSAGE(result.output);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Fresh configure failed:\n" << result.output);

    std::cout << "[Fresh Build]" << std::endl;
    result = run_cmd(test_env, bp::start_dir=(project_path), bp::shell, cmake_build_command);
    BOOST_TEST_MESSAGE(result.output);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Fresh build failed:\n" << result.output);

    BOOST_REQUIRE(fs::exists(project_binary));
    BOOST_REQUIRE(fs::exists(iconv_install_dir / "lib" / "libiconv.a"));
    BOOST_REQUIRE(fs::exists(mathlib_install_dir / "lib" / "libMathFunctions.a"));

    // 2. Verify: mathlib's summary file must NOT contain "Threads" in consumed_contents
    //    If it does, the bypass was broken and Threads was incorrectly consumed.
    if(fs::exists(mathlib_summary)) {
      std::string summary_content = pre::file::to_string(mathlib_summary.generic_string());
      BOOST_TEST_MESSAGE("mathlib summary:\n" << summary_content);

      bool threads_in_summary = summary_content.find("Threads") != std::string::npos;
      BOOST_REQUIRE_MESSAGE(!threads_in_summary,
        "BUG: 'Threads' was incorrectly consumed into mathlib's summary file.\n"
        "This means HERMETIC_FETCHCONTENT_BYPASS_PROVIDER_FOR_PACKAGES is not working correctly.\n"
        "Summary content:\n" << summary_content);
    }
  }


  // ============================================================================
  // Test: Bypass provider survives reconfigure (stale cache scenario)
  //
  // On reconfigure, CACHE INTERNAL variables from the previous run persist.
  // If Threads was incorrectly registered (HERMETIC_FETCHCONTENT_Threads_FOUND=ON)
  // during the first configure, it would cause the provider to take the wrong
  // branch on reconfigure.
  // ============================================================================
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_bypass_provider_reconfigure,
      boost::unit_test::data::make(hfc::test::test_variants()), data) {

    fs::path project_path = prepare_project_to_be_tested("check_bypass_provider", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, { "MathFunctions.h", "lib.h" }, "simple_example.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(project_path, data);
    std::string cmake_build_command = get_cmake_build_command(project_path, data);

    auto project_binary = project_path / "build" / "MyExample";
    auto mathlib_summary = project_path / "build" / "_deps" / "hermetic_targetcaches" / "mathlib.hfcsummary.cmake";

    // 1. First configure + build
    std::cout << "[First Configure]" << std::endl;
    auto result = run_cmd(test_env, bp::start_dir=(project_path), bp::shell, cmake_configure_command);
    BOOST_TEST_MESSAGE(result.output);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "First configure failed:\n" << result.output);

    std::cout << "[First Build]" << std::endl;
    result = run_cmd(test_env, bp::start_dir=(project_path), bp::shell, cmake_build_command);
    BOOST_TEST_MESSAGE(result.output);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "First build failed:\n" << result.output);

    BOOST_REQUIRE(fs::exists(project_binary));

    // 2. Reconfigure (same build dir — CMakeCache.txt persists)
    std::cout << "[Reconfigure]" << std::endl;
    result = run_cmd(test_env, bp::start_dir=(project_path), bp::shell, cmake_configure_command);
    BOOST_TEST_MESSAGE(result.output);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Reconfigure failed:\n" << result.output);

    // 3. Rebuild
    std::cout << "[Rebuild]" << std::endl;
    result = run_cmd(test_env, bp::start_dir=(project_path), bp::shell, cmake_build_command);
    BOOST_TEST_MESSAGE(result.output);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Rebuild after reconfigure failed:\n" << result.output);

    BOOST_REQUIRE(fs::exists(project_binary));

    // 4. Verify: mathlib's summary must still not contain Threads after reconfigure
    if(fs::exists(mathlib_summary)) {
      std::string summary_content = pre::file::to_string(mathlib_summary.generic_string());
      BOOST_TEST_MESSAGE("mathlib summary after reconfigure:\n" << summary_content);

      bool threads_in_summary = summary_content.find("Threads") != std::string::npos;
      BOOST_REQUIRE_MESSAGE(!threads_in_summary,
        "BUG: 'Threads' was incorrectly consumed into mathlib's summary file after reconfigure.\n"
        "This means stale CACHE entries caused the bypass to fail.\n"
        "Summary content:\n" << summary_content);
    }
  }

}
