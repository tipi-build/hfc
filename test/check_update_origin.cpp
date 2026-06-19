#define BOOST_TEST_MODULE check_update_origin_with_workaround
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

  struct deps_config {
    std::string repository_url{};
    std::string git_tag{};
  };

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_update_origin_with_workaround, boost::unit_test::data::make(hfc::test::test_variants()), data){
    deps_config original_cmake_config{
      "https://github.com/tipi-build/unit-test-cmake-template-2libs.git",
      "ecc756a4c3f1811cdfd637bd6d8f4e3feb6aff92"
    };

    deps_config original_autotools_config{
      "https://github.com/tipi-build/unittest-autotools-sample.git",
      "ad80b024eeda8f4c0a96eedf669dc453ed33a094"
    };

    deps_config fork_cmake_config{
      "https://github.com/nxxm/unit-test-cmake-template-2libs.git",
      "5a17249c3f09cbad5be52b42a44d1453071c0d24"
    };

    deps_config fork_autotools_config{
      "https://github.com/nxxm/unittest-autotools-sample.git",
      "da659594da39c7a3061407ed100bed1b36dbb7bc"
    };

    fs::path test_project_path = prepare_project_to_be_tested("check_update_origin", data.is_cmake_re, temp_dir);
    write_simple_main(test_project_path, {"MathFunctions.h", "MathFunctionscbrt.h", "lib.h"});
    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);

    {
      auto configure_output = run_command(cmake_configure_command, test_project_path, test_env);
      BOOST_REQUIRE(boost::contains(configure_output, "thirdparty/cache"));
      BOOST_REQUIRE(!boost::contains(configure_output, "thirdparty/v2_cache"));
    }
    run_command(get_cmake_build_command(test_project_path, data), test_project_path, test_env);

    fs::path real_source_path;
    if (fs::is_symlink(test_project_path / "build")) {
      fs::path build_folder_mirror = fs::read_symlink(test_project_path / "build").parent_path().parent_path();
      real_source_path = build_folder_mirror.parent_path() / build_folder_mirror.stem();
    } else {
      real_source_path = test_project_path;
    }

    BOOST_REQUIRE(fs::exists(real_source_path / "thirdparty" / "cache" / "mathlib-subbuild"));
    BOOST_REQUIRE(fs::exists(real_source_path / "thirdparty" / "cache" / "iconv-subbuild"));

    {
      auto content = pre::file::to_string((test_project_path / "CMakeLists.txt").generic_string());
      boost::replace_all(content, original_cmake_config.repository_url, fork_cmake_config.repository_url);
      boost::replace_all(content, original_cmake_config.git_tag, fork_cmake_config.git_tag);
      boost::replace_all(content, original_autotools_config.repository_url, fork_autotools_config.repository_url);
      boost::replace_all(content, original_autotools_config.git_tag, fork_autotools_config.git_tag);
      boost::replace_all(content,
        "  LANGUAGES CXX)\n",
        "  LANGUAGES CXX)\n\nset(HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR \"${CMAKE_SOURCE_DIR}/thirdparty/v2_cache\")\n");
      pre::file::from_string((test_project_path / "CMakeLists.txt").generic_string(), content);
    }

    {
      auto configure_output = run_command(cmake_configure_command, test_project_path, test_env);
      BOOST_REQUIRE(boost::contains(configure_output, "thirdparty/v2_cache"));
      BOOST_REQUIRE(!boost::contains(configure_output, "thirdparty/cache"));

    }

    run_command(get_cmake_build_command(test_project_path, data), test_project_path, test_env);
    BOOST_REQUIRE(fs::exists(real_source_path / "thirdparty" / "v2_cache" / "mathlib-subbuild"));
    BOOST_REQUIRE(fs::exists(real_source_path / "thirdparty" / "v2_cache" / "iconv-subbuild"));

    {
      auto content = pre::file::to_string((test_project_path / "CMakeLists.txt").generic_string());
      boost::replace_all(content, fork_cmake_config.repository_url, original_cmake_config.repository_url);
      boost::replace_all(content, fork_cmake_config.git_tag, original_cmake_config.git_tag);
      boost::replace_all(content, fork_autotools_config.repository_url, original_autotools_config.repository_url);
      boost::replace_all(content, fork_autotools_config.git_tag, original_autotools_config.git_tag);
      boost::replace_all(content,
        "\nset(HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR \"${CMAKE_SOURCE_DIR}/thirdparty/v2_cache\")\n",
        "\n");
      pre::file::from_string((test_project_path / "CMakeLists.txt").generic_string(), content);
    }
    {
      test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON";
      auto configure_output = run_command(cmake_configure_command, test_project_path, test_env);
      BOOST_REQUIRE(boost::contains(configure_output, "thirdparty/cache"));
      BOOST_REQUIRE(!boost::contains(configure_output, "thirdparty/v2_cache"));
    }
    run_command(get_cmake_build_command(test_project_path, data), test_project_path, test_env);
  }
}
