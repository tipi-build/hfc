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

  /// @brief Get path to the offline test_data directory from the HFC_TEST_DATA_DIR environment variable
  inline fs::path get_test_data_dir() {
    const char* env_val = std::getenv("HFC_TEST_DATA_DIR");
    BOOST_REQUIRE_MESSAGE(env_val != nullptr,
                          "HFC_TEST_DATA_DIR environment variable must be set to the path of the test_data directory");
    fs::path test_data_dir(env_val);
    BOOST_REQUIRE_MESSAGE(fs::exists(test_data_dir),
                          "HFC_TEST_DATA_DIR path does not exist: " + test_data_dir.string());
    return test_data_dir;
  }

  /// @brief Inject set() calls into a CMakeLists.txt before include(HermeticFetchContent)
  inline void inject_cmake_variables(const fs::path& project_path, const std::vector<std::pair<std::string, std::string>>& variables) {
    fs::path cmakelists = project_path / "CMakeLists.txt";
    std::string content = pre::file::to_string(cmakelists.string());

    std::string injection;
    for (const auto& [name, value] : variables) {
      injection += "set(" + name + " \"" + value + "\")\n";
    }

    std::string marker = "include(HermeticFetchContent)";
    auto pos = content.find(marker);
    content.insert(pos, injection + "\n");

    pre::file::from_string(cmakelists.string(), content);
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, goldilock_prebuilt_url_override_is_used, boost::unit_test::data::make(hfc::test::test_variants()), data){
    test_env.erase("HFC_TEST_SHARED_TOOLS_DIR");
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON";
    test_env["TIPI_CACHE_FORCE_ENABLE"] = "ON";
    test_env["TIPI_CACHE_CONSUME_ONLY"] = "ON";
    test_env["VERBOSE"] = "1";

    fs::path test_data_dir = get_test_data_dir();
    fs::path local_zip = test_data_dir / "goldilock-linux.zip";
    BOOST_REQUIRE_MESSAGE(fs::exists(local_zip), "Missing test data: " + local_zip.string());

    std::string custom_url = "file://" + local_zip.string();
    std::string custom_sha = "80729092c01a32a1f987438b2c026e993e0b81d8";

    fs::path template_path = prepare_project_to_be_tested("bootstrap_goldilock", data.is_cmake_re, temp_dir);
    write_simple_main(template_path, {}, "simple_example.cpp");

    inject_cmake_variables(template_path, {
      {"HFC_GOLDILOCK_URL_PREBUILT_Linux_x86_64", custom_url},
      {"HFC_GOLDILOCK_SHA_PREBUILT_Linux_x86_64", custom_sha}
    });

    auto toolchain = get_fresh_provisioning_toolchain(template_path);
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data, "", toolchain);
    auto output = run_command(cmake_configure_command, template_path, test_env);
    BOOST_TEST_MESSAGE("Configure output: " + output);

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
    test_env["VERBOSE"] = "1";

    fs::path test_data_dir = get_test_data_dir();
    fs::path local_zip = test_data_dir / "goldilock-linux.zip";
    BOOST_REQUIRE_MESSAGE(fs::exists(local_zip), "Missing test data: " + local_zip.string());
    fs::path goldilock_repo = test_data_dir / "goldilock";
    BOOST_REQUIRE_MESSAGE(fs::exists(goldilock_repo / ".git"), "Missing test data git repo: " + goldilock_repo.string());

    std::string custom_url = "file://" + local_zip.string();
    std::string bad_sha = "0000000000000000000000000000000000000000";
    std::string source_repo = "file://" + goldilock_repo.string();
    std::string source_tag = "1a2cea848752f1f76390daa40269b32c6d2b2143";

    fs::path template_path = prepare_project_to_be_tested("bootstrap_goldilock", data.is_cmake_re, temp_dir);
    write_simple_main(template_path, {}, "simple_example.cpp");

    fs::remove(template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake");
    fs::path hfc_goldilock_base_value_from_test = get_data_dir() / "hfc_goldilock_base_value.cmake";
    fs::copy(hfc_goldilock_base_value_from_test, (template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake"));

    inject_cmake_variables(template_path, {
      {"HFC_GOLDILOCK_URL_PREBUILT_Linux_x86_64", custom_url},
      {"HFC_GOLDILOCK_SHA_PREBUILT_Linux_x86_64", bad_sha},
      {"HFC_GOLDILOCK_GIT_REPOSITORY", source_repo},
      {"HFC_GOLDILOCK_GIT_TAG", source_tag}
    });

    auto toolchain = get_fresh_provisioning_toolchain(template_path);
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data, "", toolchain);
    auto output = run_command(cmake_configure_command, template_path, test_env);
    BOOST_TEST_MESSAGE("Configure output: " + output);

    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Downloading prebuilt goldilock from " + custom_url),
                          "Expected the custom file:// URL to be attempted. Output:\n" + output);
    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Building goldilock from source"),
                          "Expected fallback to source build after SHA mismatch. Output:\n" + output);
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, goldilock_prebuilt_url_override_empty_sha_falls_through_to_source, boost::unit_test::data::make(hfc::test::test_variants()), data){
    test_env.erase("HFC_TEST_SHARED_TOOLS_DIR");
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON";
    test_env["TIPI_CACHE_FORCE_ENABLE"] = "ON";
    test_env["TIPI_CACHE_CONSUME_ONLY"] = "ON";
    test_env["VERBOSE"] = "1";

    fs::path test_data_dir = get_test_data_dir();
    fs::path local_zip = test_data_dir / "goldilock-linux.zip";
    BOOST_REQUIRE_MESSAGE(fs::exists(local_zip), "Missing test data: " + local_zip.string());
    fs::path goldilock_repo = test_data_dir / "goldilock";
    BOOST_REQUIRE_MESSAGE(fs::exists(goldilock_repo / ".git"), "Missing test data git repo: " + goldilock_repo.string());

    std::string custom_url = "file://" + local_zip.string();
    std::string source_repo = "file://" + goldilock_repo.string();
    std::string source_tag = "1a2cea848752f1f76390daa40269b32c6d2b2143";
    // Empty SHA means integrity cannot be verified, prebuilt must be rejected

    fs::path template_path = prepare_project_to_be_tested("bootstrap_goldilock", data.is_cmake_re, temp_dir);
    write_simple_main(template_path, {}, "simple_example.cpp");

    fs::remove(template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake");
    fs::path hfc_goldilock_base_value_from_test = get_data_dir() / "hfc_goldilock_base_value.cmake";
    fs::copy(hfc_goldilock_base_value_from_test, (template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake"));

    inject_cmake_variables(template_path, {
      {"HFC_GOLDILOCK_URL_PREBUILT_Linux_x86_64", custom_url},
      {"HFC_GOLDILOCK_SHA_PREBUILT_Linux_x86_64", ""},
      {"HFC_GOLDILOCK_GIT_REPOSITORY", source_repo},
      {"HFC_GOLDILOCK_GIT_TAG", source_tag}
    });

    auto toolchain = get_fresh_provisioning_toolchain(template_path);
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data, "", toolchain);
    auto output = run_command(cmake_configure_command, template_path, test_env);
    BOOST_TEST_MESSAGE("Configure output: " + output);

    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Downloading prebuilt goldilock from " + custom_url),
                          "Expected the custom file:// URL to be attempted. Output:\n" + output);
    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Building goldilock from source"),
                          "Expected fallback to source build when SHA is empty. Output:\n" + output);
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, goldilock_git_repository_override_is_used, boost::unit_test::data::make(hfc::test::test_variants()), data){
    test_env.erase("HFC_TEST_SHARED_TOOLS_DIR");
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON";
    test_env["TIPI_CACHE_FORCE_ENABLE"] = "ON";
    test_env["TIPI_CACHE_CONSUME_ONLY"] = "ON";
    test_env["VERBOSE"] = "1";

    fs::path test_data_dir = get_test_data_dir();
    fs::path goldilock_repo = test_data_dir / "goldilock";
    BOOST_REQUIRE_MESSAGE(fs::exists(goldilock_repo / ".git"), "Missing test data git repo: " + goldilock_repo.string());

    std::string custom_repo = "file://" + goldilock_repo.string();
    std::string custom_tag = "1a2cea848752f1f76390daa40269b32c6d2b2143";

    // Use the broken prebuilt data file to force source build path
    fs::path template_path = prepare_project_to_be_tested("bootstrap_goldilock", data.is_cmake_re, temp_dir);
    write_simple_main(template_path, {}, "simple_example.cpp");

    fs::remove(template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake");
    fs::path hfc_goldilock_base_value_from_test = get_data_dir() / "hfc_goldilock_base_value.cmake";
    fs::copy(hfc_goldilock_base_value_from_test, (template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake"));

    inject_cmake_variables(template_path, {
      {"HFC_GOLDILOCK_GIT_REPOSITORY", custom_repo},
      {"HFC_GOLDILOCK_GIT_TAG", custom_tag}
    });

    auto toolchain = get_fresh_provisioning_toolchain(template_path);
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data, "", toolchain);
    auto output = run_command(cmake_configure_command, template_path, test_env);
    BOOST_TEST_MESSAGE("Configure output: " + output);

    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Building goldilock from source"),
                          "Expected source build to be triggered. Output:\n" + output);
    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Installing goldilock"),
                          "Expected goldilock to be built and installed from local bare clone. Output:\n" + output);
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, cmake_sbom_git_repository_override_is_used, boost::unit_test::data::make(hfc::test::test_variants()), data){
    test_env.erase("HFC_TEST_SHARED_TOOLS_DIR");
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON";
    test_env["TIPI_CACHE_FORCE_ENABLE"] = "ON";
    test_env["TIPI_CACHE_CONSUME_ONLY"] = "ON";
    test_env["VERBOSE"] = "1";

    fs::path test_data_dir = get_test_data_dir();
    fs::path cmake_sbom_repo = test_data_dir / "cmake-sbom";
    BOOST_REQUIRE_MESSAGE(fs::exists(cmake_sbom_repo / ".git"), "Missing test data git repo: " + cmake_sbom_repo.string());
    fs::path local_zip = test_data_dir / "goldilock-linux.zip";
    BOOST_REQUIRE_MESSAGE(fs::exists(local_zip), "Missing test data: " + local_zip.string());
    fs::path dep_repo = test_data_dir / "unit-test-cmake-template-2libs";
    BOOST_REQUIRE_MESSAGE(fs::exists(dep_repo / ".git"), "Missing test data git repo: " + dep_repo.string());

    std::string custom_repo = "file://" + cmake_sbom_repo.string();
    std::string custom_tag = "78e973801e48fb51513fa12b3420c9a7298ce0ca";
    std::string goldilock_url = "file://" + local_zip.string();
    std::string goldilock_sha = "80729092c01a32a1f987438b2c026e993e0b81d8";
    std::string dep_repo_url = "file://" + dep_repo.string();

    fs::path template_path = prepare_project_to_be_tested("check_offline_source_override", data.is_cmake_re, temp_dir);
    write_simple_main(template_path, {"MathFunctions.h", "MathFunctionscbrt.h"}, "simple_example.cpp");

    inject_cmake_variables(template_path, {
      {"HFC_CMAKE_SBOM_GIT_REPOSITORY", custom_repo},
      {"HFC_CMAKE_SBOM_GIT_TAG", custom_tag},
      {"HFC_GOLDILOCK_URL_PREBUILT_Linux_x86_64", goldilock_url},
      {"HFC_GOLDILOCK_SHA_PREBUILT_Linux_x86_64", goldilock_sha},
      {"HFC_TEST_DEP_GIT_REPOSITORY", dep_repo_url}
    });

    auto toolchain = get_fresh_provisioning_toolchain(template_path);
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data, "", toolchain);
    auto output = run_command(cmake_configure_command, template_path, test_env);
    BOOST_TEST_MESSAGE("Configure output: " + output);

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
    test_env["VERBOSE"] = "1";

    fs::path test_data_dir = get_test_data_dir();
    fs::path cmake_sbom_repo = test_data_dir / "cmake-sbom";
    BOOST_REQUIRE_MESSAGE(fs::exists(cmake_sbom_repo / ".git"), "Missing test data git repo: " + cmake_sbom_repo.string());
    fs::path local_zip = test_data_dir / "goldilock-linux.zip";
    BOOST_REQUIRE_MESSAGE(fs::exists(local_zip), "Missing test data: " + local_zip.string());
    fs::path dep_repo = test_data_dir / "unit-test-cmake-template-2libs";
    BOOST_REQUIRE_MESSAGE(fs::exists(dep_repo / ".git"), "Missing test data git repo: " + dep_repo.string());

    // Use a different valid tag — source dir name should reflect it
    std::string custom_repo = "file://" + cmake_sbom_repo.string();
    std::string custom_tag = "93f8d2f0bc9442e4e9d2b485cf936339621f2501";
    std::string goldilock_url = "file://" + local_zip.string();
    std::string goldilock_sha = "80729092c01a32a1f987438b2c026e993e0b81d8";
    std::string dep_repo_url = "file://" + dep_repo.string();

    fs::path template_path = prepare_project_to_be_tested("check_offline_source_override", data.is_cmake_re, temp_dir);
    write_simple_main(template_path, {"MathFunctions.h", "MathFunctionscbrt.h"}, "simple_example.cpp");

    inject_cmake_variables(template_path, {
      {"HFC_CMAKE_SBOM_GIT_REPOSITORY", custom_repo},
      {"HFC_CMAKE_SBOM_GIT_TAG", custom_tag},
      {"HFC_GOLDILOCK_URL_PREBUILT_Linux_x86_64", goldilock_url},
      {"HFC_GOLDILOCK_SHA_PREBUILT_Linux_x86_64", goldilock_sha},
      {"HFC_TEST_DEP_GIT_REPOSITORY", dep_repo_url}
    });

    auto toolchain = get_fresh_provisioning_toolchain(template_path);
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data, "", toolchain);
    auto output = run_command(cmake_configure_command, template_path, test_env);
    BOOST_TEST_MESSAGE("Configure output: " + output);

    // The source dir should contain the first 8 chars of the custom tag
    std::string expected_dir_fragment = "hfc_cmake_sbom-" + custom_tag.substr(0, 8);
    BOOST_REQUIRE_MESSAGE(boost::contains(output, "Enabling HFC cmake-sbom support (" + custom_repo),
                          "Expected cmake-sbom bootstrap from local clone. Output:\n" + output);
    BOOST_REQUIRE_MESSAGE(boost::contains(output, expected_dir_fragment),
                          "Expected source dir to contain '" + expected_dir_fragment + "'. Output:\n" + output);
  }

}
