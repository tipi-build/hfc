#define BOOST_TEST_MODULE offline_source_override_test
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

  /// @brief Get path to the fresh-provisioning toolchain (no pre-provisioned goldilock)
  inline fs::path get_fresh_provisioning_toolchain(const fs::path& project_path) {
    return project_path / "toolchain" / "linux-toolchain-fresh-provisioning.cmake";
  }

  /// @brief Helper: create a local bare clone of a remote git repository
  inline fs::path create_local_bare_clone(const fs::path& temp_dir, const std::string& remote_url, const std::string& name) {
    fs::path bare_repo = temp_dir / (name + ".git");
    fs::create_directories(bare_repo);

    auto git_bin = bp::search_path("git");
    std::string clone_cmd = git_bin.string() + " clone --bare " + remote_url + " " + bare_repo.string();

    auto result = run_cmd(bp::start_dir = temp_dir, bp::shell, clone_cmd);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
                          "Failed to create bare clone of " + remote_url + ": " + result.output);

    return bare_repo;
  }

  /// @brief Helper: download a file to a local path
  inline fs::path download_file_locally(const fs::path& temp_dir, const std::string& url, const std::string& filename) {
    fs::path dest = temp_dir / filename;

    // Use curl to download
    auto curl_bin = bp::search_path("curl");
    BOOST_REQUIRE_MESSAGE(!curl_bin.empty(), "curl not found on PATH");

    std::string download_cmd = curl_bin.string() + " -fsSL -o " + dest.string() + " " + url;
    auto result = run_cmd(bp::start_dir = temp_dir, bp::shell, download_cmd);
    BOOST_REQUIRE_MESSAGE(result.return_code == 0,
                          "Failed to download " + url + ": " + result.output);
    BOOST_REQUIRE(fs::exists(dest));

    return dest;
  }

  //
  // HFC_GOLDILOCK_URL_PREBUILT tests
  //

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, goldilock_prebuilt_url_override_is_used, boost::unit_test::data::make(hfc::test::test_variants()), data){
    test_env.erase("HFC_TEST_SHARED_TOOLS_DIR");
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON";
    test_env["TIPI_CACHE_FORCE_ENABLE"] = "ON";
    test_env["TIPI_CACHE_CONSUME_ONLY"] = "ON";

    // Download the real prebuilt locally, then point the CMake variable at the local file:// URI
    fs::path local_mirror = temp_dir / "mirror";
    fs::create_directories(local_mirror);
    fs::path local_zip = download_file_locally(local_mirror,
      "https://github.com/tipi-build/goldilock/releases/download/v1.2.1/goldilock-linux.zip",
      "goldilock-local.zip");

    std::string custom_url = "file://" + local_zip.string();
    std::string custom_sha = "80729092c01a32a1f987438b2c026e993e0b81d8";

    fs::path template_path = prepare_project_to_be_tested("bootstrap_goldilock", data.is_cmake_re, temp_dir);
    write_simple_main(template_path, {}, "simple_example.cpp");

    std::string additional_vars = "-DHFC_GOLDILOCK_URL_PREBUILT_Linux_x86_64=\"" + custom_url + "\" -DHFC_GOLDILOCK_SHA_PREBUILT_Linux_x86_64=" + custom_sha;
    auto toolchain = get_fresh_provisioning_toolchain(template_path);
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data, additional_vars, toolchain);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Downloading prebuilt goldilock from " + custom_url),
                          "Expected the custom file:// prebuilt URL in download log. Output:\n" + output);
    BOOST_REQUIRE_MESSAGE(boost::contains(output, "goldilock has been downloaded"),
                          "Expected goldilock to be successfully downloaded from local mirror. Output:\n" + output);
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, goldilock_prebuilt_url_override_bad_sha_falls_through_to_source, boost::unit_test::data::make(hfc::test::test_variants()), data){
    test_env.erase("HFC_TEST_SHARED_TOOLS_DIR");
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON";
    test_env["TIPI_CACHE_FORCE_ENABLE"] = "ON";
    test_env["TIPI_CACHE_CONSUME_ONLY"] = "ON";

    // Download the real prebuilt locally but set a wrong SHA
    fs::path local_mirror = temp_dir / "mirror";
    fs::create_directories(local_mirror);
    fs::path local_zip = download_file_locally(local_mirror,
      "https://github.com/tipi-build/goldilock/releases/download/v1.2.1/goldilock-linux.zip",
      "goldilock-local.zip");

    std::string custom_url = "file://" + local_zip.string();
    std::string bad_sha = "0000000000000000000000000000000000000000";

    fs::path template_path = prepare_project_to_be_tested("bootstrap_goldilock", data.is_cmake_re, temp_dir);
    write_simple_main(template_path, {}, "simple_example.cpp");

    std::string additional_vars = "-DHFC_GOLDILOCK_URL_PREBUILT_Linux_x86_64=\"" + custom_url + "\" -DHFC_GOLDILOCK_SHA_PREBUILT_Linux_x86_64=" + bad_sha;
    auto toolchain = get_fresh_provisioning_toolchain(template_path);
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data, additional_vars, toolchain);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Downloading prebuilt goldilock from " + custom_url),
                          "Expected the custom file:// URL to be attempted. Output:\n" + output);
    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Building goldilock from source"),
                          "Expected fallback to source build after SHA mismatch. Output:\n" + output);
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, goldilock_prebuilt_url_override_no_sha_skips_verification, boost::unit_test::data::make(hfc::test::test_variants()), data){
    test_env.erase("HFC_TEST_SHARED_TOOLS_DIR");
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON";
    test_env["TIPI_CACHE_FORCE_ENABLE"] = "ON";
    test_env["TIPI_CACHE_CONSUME_ONLY"] = "ON";

    // Download the real prebuilt locally, set no SHA
    fs::path local_mirror = temp_dir / "mirror";
    fs::create_directories(local_mirror);
    fs::path local_zip = download_file_locally(local_mirror,
      "https://github.com/tipi-build/goldilock/releases/download/v1.2.1/goldilock-linux.zip",
      "goldilock-local.zip");

    std::string custom_url = "file://" + local_zip.string();
    // Deliberately NOT setting HFC_GOLDILOCK_SHA_PREBUILT

    fs::path template_path = prepare_project_to_be_tested("bootstrap_goldilock", data.is_cmake_re, temp_dir);
    write_simple_main(template_path, {}, "simple_example.cpp");

    std::string additional_vars = "-DHFC_GOLDILOCK_URL_PREBUILT_Linux_x86_64=\"" + custom_url + "\"";
    auto toolchain = get_fresh_provisioning_toolchain(template_path);
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data, additional_vars, toolchain);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Downloading prebuilt goldilock from " + custom_url),
                          "Expected the custom file:// URL to be used. Output:\n" + output);
    BOOST_REQUIRE_MESSAGE(boost::contains(output, "goldilock has been downloaded"),
                          "Expected goldilock download to succeed without SHA check. Output:\n" + output);
  }

  //
  // HFC_GOLDILOCK_GIT_REPOSITORY / HFC_GOLDILOCK_GIT_TAG tests
  //

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, goldilock_git_repository_override_is_used, boost::unit_test::data::make(hfc::test::test_variants()), data){
    test_env.erase("HFC_TEST_SHARED_TOOLS_DIR");
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON";
    test_env["TIPI_CACHE_FORCE_ENABLE"] = "ON";
    test_env["TIPI_CACHE_CONSUME_ONLY"] = "ON";

    // Create a local bare clone of goldilock to use as our custom source
    fs::path bare_repo = create_local_bare_clone(temp_dir,
      "https://github.com/tipi-build/goldilock.git", "goldilock");

    std::string custom_repo = "file://" + bare_repo.string();
    std::string custom_tag = "5f8b9de72c10a6216c89a8807db8d420cff05512";

    // Use the broken prebuilt data file to force source build path
    fs::path template_path = prepare_project_to_be_tested("bootstrap_goldilock", data.is_cmake_re, temp_dir);
    write_simple_main(template_path, {}, "simple_example.cpp");

    fs::remove(template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake");
    fs::path hfc_goldilock_base_value_from_test = get_data_dir() / "hfc_goldilock_base_value.cmake";
    fs::copy(hfc_goldilock_base_value_from_test, (template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake"));

    std::string additional_vars = "-DHFC_GOLDILOCK_GIT_REPOSITORY=\"" + custom_repo + "\" -DHFC_GOLDILOCK_GIT_TAG=" + custom_tag;
    auto toolchain = get_fresh_provisioning_toolchain(template_path);
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data, additional_vars, toolchain);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Building goldilock from source"),
                          "Expected source build to be triggered. Output:\n" + output);
    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Installing goldilock"),
                          "Expected goldilock to be built and installed from local bare clone. Output:\n" + output);
  }

  //
  // HFC_CMAKE_SBOM_GIT_REPOSITORY / HFC_CMAKE_SBOM_GIT_TAG tests
  //

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, cmake_sbom_git_repository_override_is_used, boost::unit_test::data::make(hfc::test::test_variants()), data){
    test_env.erase("HFC_TEST_SHARED_TOOLS_DIR");
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON";
    test_env["TIPI_CACHE_FORCE_ENABLE"] = "ON";
    test_env["TIPI_CACHE_CONSUME_ONLY"] = "ON";

    // Create a local bare clone of cmake-sbom
    fs::path bare_repo = create_local_bare_clone(temp_dir,
      "https://github.com/DEMCON/cmake-sbom.git", "cmake-sbom");

    std::string custom_repo = "file://" + bare_repo.string();
    std::string custom_tag = "97b1a0715af7726cae93d96d322c48584945f96b";

    fs::path template_path = prepare_project_to_be_tested("check_sbom_bootstrap", data.is_cmake_re, temp_dir);
    write_simple_main(template_path, {"MathFunctions.h", "MathFunctionscbrt.h"}, "simple_example.cpp");

    std::string additional_vars = "-DHFC_CMAKE_SBOM_GIT_REPOSITORY=\"" + custom_repo + "\" -DHFC_CMAKE_SBOM_GIT_TAG=" + custom_tag;
    auto toolchain = get_fresh_provisioning_toolchain(template_path);
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data, additional_vars, toolchain);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Enabling HFC cmake-sbom support (" + custom_repo),
                          "Expected the local file:// cmake-sbom repository URL in the log. Output:\n" + output);
    BOOST_REQUIRE_MESSAGE(boost::contains(output, custom_tag),
                          "Expected the custom cmake-sbom tag in the log. Output:\n" + output);
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, cmake_sbom_git_tag_override_changes_source_dir, boost::unit_test::data::make(hfc::test::test_variants()), data){
    test_env.erase("HFC_TEST_SHARED_TOOLS_DIR");
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON";
    test_env["TIPI_CACHE_FORCE_ENABLE"] = "ON";
    test_env["TIPI_CACHE_CONSUME_ONLY"] = "ON";

    // Create a local bare clone of cmake-sbom
    fs::path bare_repo = create_local_bare_clone(temp_dir,
      "https://github.com/DEMCON/cmake-sbom.git", "cmake-sbom");

    // Use a different valid tag — source dir name should reflect it
    std::string custom_repo = "file://" + bare_repo.string();
    std::string custom_tag = "3add8b81fc1bc23dfbddf0f7b0bfdca2efc1135c";

    fs::path template_path = prepare_project_to_be_tested("check_sbom_bootstrap", data.is_cmake_re, temp_dir);
    write_simple_main(template_path, {"MathFunctions.h", "MathFunctionscbrt.h"}, "simple_example.cpp");

    std::string additional_vars = "-DHFC_CMAKE_SBOM_GIT_REPOSITORY=\"" + custom_repo + "\" -DHFC_CMAKE_SBOM_GIT_TAG=" + custom_tag;
    auto toolchain = get_fresh_provisioning_toolchain(template_path);
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data, additional_vars, toolchain);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    // The source dir should contain the first 8 chars of the custom tag
    std::string expected_dir_fragment = "hfc_cmake_sbom-" + custom_tag.substr(0, 8);
    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Enabling HFC cmake-sbom support (" + custom_repo),
                          "Expected cmake-sbom bootstrap from local clone. Output:\n" + output);
    BOOST_REQUIRE_MESSAGE(boost::contains(output, expected_dir_fragment),
                          "Expected source dir to contain '" + expected_dir_fragment + "'. Output:\n" + output);
  }

}
