#define BOOST_TEST_MODULE hfc_version_stepping_test
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

  /// Extracts the HFC cmake/ directory from a given git commit into a destination path.
  /// This allows testing with a different version of HFC modules than what's in the working tree.
  inline void extract_hfc_cmake_from_commit(const std::string& commit_ref, const fs::path& destination) {
    fs::path source_tree = get_source_tree_dir();

    // Remove existing cmake dir at destination
    if(fs::exists(destination / "cmake")) {
      fs::remove_all(destination / "cmake");
    }
    fs::create_directories(destination / "cmake");
    fs::path git_bin = bp::search_path("git");
    fs::path tar_bin = bp::search_path("tar");

    if(git_bin.empty()) { throw std::runtime_error("git not found in PATH"); }
    if(tar_bin.empty()) { throw std::runtime_error("tar not found in PATH"); }

    fs::path archive_file = destination / "hfc_archive.tar";

    auto git_result = run_cmd(
      boost::this_process::environment(),
      bp::start_dir=(source_tree),
      git_bin, "-C", source_tree.generic_string(),
      "archive", "-o", archive_file.generic_string(),
      commit_ref, "--", "cmake/"
    );

    if(git_result.return_code != 0) {
      throw std::runtime_error(
        "Failed to create archive from commit " + commit_ref +
        ": " + git_result.output);
    }

    auto tar_result = run_cmd(
      boost::this_process::environment(),
      bp::start_dir=(destination),
      tar_bin, "-xf", archive_file.generic_string(), "-C", destination.generic_string()
    );

    fs::remove(archive_file);

    if(tar_result.return_code != 0) {
      throw std::runtime_error(
        "Failed to extract archive for commit " + commit_ref +
        ": " + tar_result.output);
    }
  }

  /// Copies the current (working tree) HFC cmake modules into the test project.
  /// This is equivalent to what prepare_project_to_be_tested does, but can be called
  /// again to "upgrade" the project to the current version.
  inline void install_current_hfc_cmake(const fs::path& project_path) {
    fs::path source_tree = get_source_tree_dir();

    if(fs::exists(project_path / "cmake")) {
      fs::remove_all(project_path / "cmake");
    }
    fs::create_directory(project_path / "cmake");
    fs::copy(source_tree / "cmake", project_path / "cmake", fs::copy_options::recursive);
  }

  /// Returns the commit hash of the parent of the current HEAD.
  /// This gives us the "V-1" version for stepping tests.
  inline std::string get_previous_version_commit() {
    fs::path source_tree = get_source_tree_dir();
    auto result = run_cmd(
      boost::this_process::environment(),
      bp::start_dir=(source_tree),
      "git -C " + source_tree.generic_string() + " log --format=%H -n1 HEAD~1"
    );

    if(result.return_code != 0 || result.output.empty()) {
      throw std::runtime_error("Failed to determine previous version commit");
    }

    std::string commit = result.output;
    boost::trim(commit);
    return commit;
  }

  /// Returns the commit hash of the current HEAD.
  inline std::string get_current_version_commit() {
    fs::path source_tree = get_source_tree_dir();
    auto result = run_cmd(
      boost::this_process::environment(),
      bp::start_dir=(source_tree),
      "git -C " + source_tree.generic_string() + " log --format=%H -n1 HEAD"
    );

    if(result.return_code != 0 || result.output.empty()) {
      throw std::runtime_error("Failed to determine current version commit");
    }

    std::string commit = result.output;
    boost::trim(commit);
    return commit;
  }


  
  // Test: Upgrade from V-1 to V (previous -> current)
  //
  // Scenario: A project configured with the previous HFC version upgrades to the
  // current version. After upgrade, reconfigure and build must succeed. The
  // dependency should either be reused from the existing install tree or cleanly
  // reconfigured.  
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, hfc_version_stepping_upgrade,
      boost::unit_test::data::make(hfc::test::test_variants()), data) {

    std::string previous_commit = get_previous_version_commit();
    std::string current_commit = get_current_version_commit();

    BOOST_TEST_MESSAGE("Testing upgrade from " << previous_commit << " to " << current_commit);

    // 1. Prepare test project with CURRENT modules (as prepare_project_to_be_tested does)
    fs::path project_path = prepare_project_to_be_tested("hfc_version_stepping", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, { "MathFunctions.h", "lib.h" }, "simple_example.cpp");

    // 2. Downgrade to previous version's cmake modules
    BOOST_TEST_MESSAGE("Installing HFC modules from previous version: " << previous_commit);
    extract_hfc_cmake_from_commit(previous_commit, project_path);

    std::string cmake_configure_command = get_cmake_configure_command(project_path, data);
    std::string cmake_build_command = get_cmake_build_command(project_path, data);

    auto project_binary = project_path / "build" / "MyExample";
    auto mathlib_install_dir = project_path / "build" / "_deps" / "mathlib-install";
    auto iconv_install_dir = project_path / "build" / "_deps" / "iconv_lib-install";

    // 3. Configure + Build with previous version
    std::cout << "[V-1 Configure]" << std::endl;
    auto result = run_cmd(test_env, bp::start_dir=(project_path),  cmake_configure_command);
    BOOST_TEST_MESSAGE(result.output);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Configure with previous version failed:\n" << result.output);

    std::cout << "[V-1 Build]" << std::endl;
    result = run_cmd(test_env, bp::start_dir=(project_path),  cmake_build_command);
    BOOST_TEST_MESSAGE(result.output);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Build with previous version failed:\n" << result.output);

    BOOST_REQUIRE(fs::exists(project_binary));
    BOOST_REQUIRE(fs::exists(mathlib_install_dir / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(iconv_install_dir / "lib" / "libiconv.a"));

    file_fingerprint binary_before_upgrade{project_binary};
    file_fingerprint mathlib_before_upgrade{mathlib_install_dir / "lib" / "libMathFunctions.a"};
    file_fingerprint iconv_before_upgrade{iconv_install_dir / "lib" / "libiconv.a"};

    // 4. UPGRADE: swap in current version's cmake modules
    std::cout << "[Upgrading HFC: V-1 -> V]" << std::endl;
    install_current_hfc_cmake(project_path);

    // 5. Reconfigure with current version
    std::cout << "[V Configure (after upgrade)]" << std::endl;
    result = run_cmd(test_env, bp::start_dir=(project_path),  cmake_configure_command);
    BOOST_TEST_MESSAGE(result.output);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Configure after upgrade to current version failed:\n" << result.output);

    // 6. Build with current version
    std::cout << "[V Build (after upgrade)]" << std::endl;
    result = run_cmd(test_env, bp::start_dir=(project_path),  cmake_build_command);
    BOOST_TEST_MESSAGE(result.output);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Build after upgrade to current version failed:\n" << result.output);

    BOOST_REQUIRE(fs::exists(project_binary));
    BOOST_REQUIRE(fs::exists(mathlib_install_dir / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(iconv_install_dir / "lib" / "libiconv.a"));

    // 7. Verify stability: reconfigure again should be idempotent
    file_fingerprint mathlib_after_upgrade{mathlib_install_dir / "lib" / "libMathFunctions.a"};
    file_fingerprint iconv_after_upgrade{iconv_install_dir / "lib" / "libiconv.a"};

    std::cout << "[V Configure 2 (stability check)]" << std::endl;
    result = run_cmd(test_env, bp::start_dir=(project_path),  cmake_configure_command);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Second configure after upgrade failed:\n" << result.output);

    std::cout << "[V Build 2 (stability check)]" << std::endl;
    result = run_cmd(test_env, bp::start_dir=(project_path),  cmake_build_command);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Second build after upgrade failed:\n" << result.output);

    // Neither library should have been rebuilt on the second pass
    BOOST_REQUIRE(mathlib_after_upgrade.is_unchanged());
    BOOST_REQUIRE(iconv_after_upgrade.is_unchanged());
  }

  
  // Test: Downgrade from V to V-1 (current -> previous)
  //
  // Scenario: A project configured with the current HFC version downgrades to the
  // previous version. After downgrade, reconfigure and build must succeed.  
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, hfc_version_stepping_downgrade,
      boost::unit_test::data::make(hfc::test::test_variants()), data) {

    std::string previous_commit = get_previous_version_commit();
    std::string current_commit = get_current_version_commit();

    BOOST_TEST_MESSAGE("Testing downgrade from " << current_commit << " to " << previous_commit);

    // 1. Prepare test project with current version (default behavior)
    fs::path project_path = prepare_project_to_be_tested("hfc_version_stepping", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, { "MathFunctions.h", "lib.h" }, "simple_example.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(project_path, data);
    std::string cmake_build_command = get_cmake_build_command(project_path, data);

    auto project_binary = project_path / "build" / "MyExample";
    auto mathlib_install_dir = project_path / "build" / "_deps" / "mathlib-install";
    auto iconv_install_dir = project_path / "build" / "_deps" / "iconv_lib-install";

    // 2. Configure + Build with current version
    std::cout << "[V Configure]" << std::endl;
    auto result = run_cmd(test_env, bp::start_dir=(project_path),  cmake_configure_command);
    BOOST_TEST_MESSAGE(result.output);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Configure with current version failed:\n" << result.output);

    std::cout << "[V Build]" << std::endl;
    result = run_cmd(test_env, bp::start_dir=(project_path),  cmake_build_command);
    BOOST_TEST_MESSAGE(result.output);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Build with current version failed:\n" << result.output);

    BOOST_REQUIRE(fs::exists(project_binary));
    BOOST_REQUIRE(fs::exists(mathlib_install_dir / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(iconv_install_dir / "lib" / "libiconv.a"));

    file_fingerprint binary_before_downgrade{project_binary};
    file_fingerprint mathlib_before_downgrade{mathlib_install_dir / "lib" / "libMathFunctions.a"};
    file_fingerprint iconv_before_downgrade{iconv_install_dir / "lib" / "libiconv.a"};

    // 3. DOWNGRADE: swap in previous version's cmake modules
    std::cout << "[Downgrading HFC: V -> V-1]" << std::endl;
    extract_hfc_cmake_from_commit(previous_commit, project_path);

    // 4. Reconfigure with previous version
    std::cout << "[V-1 Configure (after downgrade)]" << std::endl;
    result = run_cmd(test_env, bp::start_dir=(project_path),  cmake_configure_command);
    BOOST_TEST_MESSAGE(result.output);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Configure after downgrade to previous version failed:\n" << result.output);

    // 5. Build with previous version
    std::cout << "[V-1 Build (after downgrade)]" << std::endl;
    result = run_cmd(test_env, bp::start_dir=(project_path),  cmake_build_command);
    BOOST_TEST_MESSAGE(result.output);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Build after downgrade to previous version failed:\n" << result.output);

    BOOST_REQUIRE(fs::exists(project_binary));
    BOOST_REQUIRE(fs::exists(mathlib_install_dir / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(iconv_install_dir / "lib" / "libiconv.a"));

    // 6. Verify stability: reconfigure again should be idempotent
    file_fingerprint mathlib_after_downgrade{mathlib_install_dir / "lib" / "libMathFunctions.a"};
    file_fingerprint iconv_after_downgrade{iconv_install_dir / "lib" / "libiconv.a"};

    std::cout << "[V-1 Configure 2 (stability check)]" << std::endl;
    result = run_cmd(test_env, bp::start_dir=(project_path),  cmake_configure_command);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Second configure after downgrade failed:\n" << result.output);

    std::cout << "[V-1 Build 2 (stability check)]" << std::endl;
    result = run_cmd(test_env, bp::start_dir=(project_path),  cmake_build_command);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
      "Second build after downgrade failed:\n" << result.output);

    BOOST_REQUIRE(mathlib_after_downgrade.is_unchanged());
    BOOST_REQUIRE(iconv_after_downgrade.is_unchanged());
  }

  
  // Test: Round-trip V-1 -> V -> V-1
  //
  // Scenario: Upgrade then downgrade back. Verifies that version stepping in both
  // directions within the same build tree doesn't corrupt state.  
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, hfc_version_stepping_roundtrip,
      boost::unit_test::data::make(hfc::test::test_variants()), data) {

    std::string previous_commit = get_previous_version_commit();

    fs::path project_path = prepare_project_to_be_tested("hfc_version_stepping", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, { "MathFunctions.h", "lib.h" }, "simple_example.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(project_path, data);
    std::string cmake_build_command = get_cmake_build_command(project_path, data);

    auto project_binary = project_path / "build" / "MyExample";
    auto mathlib_lib = project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctions.a";
    auto iconv_lib = project_path / "build" / "_deps" / "iconv_lib-install" / "lib" / "libiconv.a";

    // Phase 1: Start with V-1
    std::cout << "[Phase 1: V-1]" << std::endl;
    extract_hfc_cmake_from_commit(previous_commit, project_path);
    run_command(cmake_configure_command, project_path, test_env);
    run_command(cmake_build_command, project_path, test_env);
    BOOST_REQUIRE(fs::exists(project_binary));
    BOOST_REQUIRE(fs::exists(mathlib_lib));
    BOOST_REQUIRE(fs::exists(iconv_lib));

    // Phase 2: Upgrade to V
    std::cout << "[Phase 2: Upgrade to V]" << std::endl;
    install_current_hfc_cmake(project_path);
    run_command(cmake_configure_command, project_path, test_env);
    run_command(cmake_build_command, project_path, test_env);
    BOOST_REQUIRE(fs::exists(project_binary));
    BOOST_REQUIRE(fs::exists(mathlib_lib));
    BOOST_REQUIRE(fs::exists(iconv_lib));

    file_fingerprint mathlib_at_v{mathlib_lib};
    file_fingerprint iconv_at_v{iconv_lib};

    // Phase 3: Downgrade back to V-1
    std::cout << "[Phase 3: Downgrade back to V-1]" << std::endl;
    extract_hfc_cmake_from_commit(previous_commit, project_path);
    run_command(cmake_configure_command, project_path, test_env);
    run_command(cmake_build_command, project_path, test_env);
    BOOST_REQUIRE(fs::exists(project_binary));
    BOOST_REQUIRE(fs::exists(mathlib_lib));
    BOOST_REQUIRE(fs::exists(iconv_lib));

    // Phase 4: Stability — one more configure+build should be no-op
    std::cout << "[Phase 4: Stability check]" << std::endl;
    file_fingerprint mathlib_at_v_minus_1{mathlib_lib};
    file_fingerprint iconv_at_v_minus_1{iconv_lib};
    run_command(cmake_configure_command, project_path, test_env);
    run_command(cmake_build_command, project_path, test_env);
    BOOST_REQUIRE(mathlib_at_v_minus_1.is_unchanged());
    BOOST_REQUIRE(iconv_at_v_minus_1.is_unchanged());
  }
  
  // Test: Upgrade with a clean build directory (fresh configure after version bump)
  //
  // Scenario: User deletes build/ and reconfigures after upgrading HFC. This
  // verifies the new version can bootstrap from scratch without leftover state in the build
  // tree a source cache state present (goldilock bootstrapping)
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, hfc_version_stepping_upgrade_clean_build,
      boost::unit_test::data::make(hfc::test::test_variants()), data) {

    std::string previous_commit = get_previous_version_commit();

    fs::path project_path = prepare_project_to_be_tested("hfc_version_stepping", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, { "MathFunctions.h", "lib.h" }, "simple_example.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(project_path, data);
    std::string cmake_build_command = get_cmake_build_command(project_path, data);

    auto project_binary = project_path / "build" / "MyExample";
    auto mathlib_lib = project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctions.a";
    auto iconv_lib = project_path / "build" / "_deps" / "iconv_lib-install" / "lib" / "libiconv.a";

    // Phase 1: Configure + build with V-1
    std::cout << "[V-1 Configure + Build]" << std::endl;
    extract_hfc_cmake_from_commit(previous_commit, project_path);
    run_command(cmake_configure_command, project_path, test_env);
    run_command(cmake_build_command, project_path, test_env);
    BOOST_REQUIRE(fs::exists(project_binary));
    BOOST_REQUIRE(fs::exists(iconv_lib));

    // Phase 2: Delete build directory (simulates clean rebuild after upgrade)
    std::cout << "[Clean build directory]" << std::endl;
    fs::remove_all(project_path / "build");
    fs::create_directories(project_path / "build");

    // Phase 3: Upgrade and configure from scratch
    std::cout << "[V Configure + Build (clean)]" << std::endl;
    install_current_hfc_cmake(project_path);
    run_command(cmake_configure_command, project_path, test_env);
    run_command(cmake_build_command, project_path, test_env);
    BOOST_REQUIRE(fs::exists(project_binary));
    BOOST_REQUIRE(fs::exists(mathlib_lib));
    BOOST_REQUIRE(fs::exists(iconv_lib));
  }

}
