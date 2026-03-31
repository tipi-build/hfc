#pragma once

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/predef/os.h>
#include <pre/file/string.hpp>

namespace hfc::test {
  namespace fs = boost::filesystem;
  namespace bp = boost::process;

  /// @brief Fixture to isolate test environments by setting unique TIPI_HOME_DIR
  /// and TIPI_DISTRO_MODE=none for each test execution.
  /// This prevents tests from interfering with each other when run in parallel.
  struct test_isolation_fixture {
    fs::path temp_dir;
    bp::environment test_env;

    test_isolation_fixture()
      : temp_dir(fs::temp_directory_path() / fs::unique_path("hfc_test_%%%%-%%%%-%%%%"))
      , test_env(boost::this_process::environment())
    {
      // Create unique temporary directory for this test
      fs::create_directories(temp_dir);

      // Set TIPI_DISTRO_MODE to none
      test_env["TIPI_DISTRO_MODE"] = "none";

      // Set TIPI_HOME_DIR to unique temporary directory
      test_env["TIPI_HOME_DIR"] = temp_dir.string();

      // Setup TIPI credentials from environment or fallback to dot files
      setup_tipi_credentials();

      std::cout << "[Test Isolation] Created temp dir: " << temp_dir.string() << std::endl;
    }

    ~test_isolation_fixture() {
      // Clean up temporary directory
      if (fs::exists(temp_dir)) {
        try {
          fs::remove_all(temp_dir);
          std::cout << "[Test Isolation] Removed temp dir: " << temp_dir.string() << std::endl;
        } catch (const std::exception& e) {
          std::cerr << "[Test Isolation] Warning: Failed to remove temp dir "
                    << temp_dir.string() << ": " << e.what() << std::endl;
        }
      }
    }

    // Delete copy and move operations to prevent issues
    test_isolation_fixture(const test_isolation_fixture&) = delete;
    test_isolation_fixture& operator=(const test_isolation_fixture&) = delete;
    test_isolation_fixture(test_isolation_fixture&&) = delete;
    test_isolation_fixture& operator=(test_isolation_fixture&&) = delete;

  private:
    /// @brief Setup TIPI credentials by copying files if environment variables are not set
    void setup_tipi_credentials() {
#if BOOST_OS_WINDOWS
      const fs::path tipi_share_dir = "C:\\.tipi";
#else
      const fs::path tipi_share_dir = "/usr/local/share/.tipi";
#endif

      // If environment variables are not set, copy credential files to isolated TIPI_HOME_DIR
      // so the tests can find them there

      // Copy .access_token if TIPI_ACCESS_TOKEN env var is not set
      if (!test_env.count("TIPI_ACCESS_TOKEN")) {
        fs::path access_token_file = tipi_share_dir / ".access_token";
        if (fs::exists(access_token_file)) {
          fs::path dest_access_token = temp_dir / ".access_token";
          fs::copy_file(access_token_file, dest_access_token, fs::copy_options::overwrite_existing);
        }
      }

      // Copy .refresh_token if TIPI_REFRESH_TOKEN env var is not set
      if (!test_env.count("TIPI_REFRESH_TOKEN")) {
        fs::path refresh_token_file = tipi_share_dir / ".refresh_token";
        if (fs::exists(refresh_token_file)) {
          fs::path dest_refresh_token = temp_dir / ".refresh_token";
          fs::copy_file(refresh_token_file, dest_refresh_token, fs::copy_options::overwrite_existing);
        }
      }

      // Copy .vault if TIPI_VAULT_PASSPHRASE env var is not set
      if (!test_env.count("TIPI_VAULT_PASSPHRASE")) {
        fs::path vault_file = tipi_share_dir / ".vault";
        if (fs::exists(vault_file)) {
          fs::path dest_vault = temp_dir / ".vault";
          fs::copy_file(vault_file, dest_vault, fs::copy_options::overwrite_existing);
        }
      }

      // Always copy .access_key if it exists (needed when using vault)
      fs::path access_key_file = tipi_share_dir / ".access_key";
      if (fs::exists(access_key_file)) {
        fs::path dest_access_key = temp_dir / ".access_key";
        fs::copy_file(access_key_file, dest_access_key, fs::copy_options::overwrite_existing);
      }
    }
  };
}
