#define BOOST_TEST_MODULE check_update_version
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

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_version_update, boost::unit_test::data::make(hfc::test::test_variants()), data){
    std::string first_commit = "790f82e8a01b062b34133ef71dd94e9468717f37";
    std::string second_commit = "5cfd9d4e490d910acef72782e058739a83837305";

    std::string first_commit_autotools = "ad80b024eeda8f4c0a96eedf669dc453ed33a094";
    std::string second_commit_autotools = "30258ed5d3227cd37500af8cded3cdc241c288a2";

    fs::path test_project_path = prepare_project_to_be_tested("check_version_update", data.is_cmake_re, temp_dir);
    write_simple_main(test_project_path, {"version.hpp"});
    write_simple_main(test_project_path, {}, "simple_main.cpp" );

    test_env["TIPI_CACHE_FORCE_ENABLE"] = "OFF";
    test_env["TIPI_CACHE_CONSUME_ONLY"] = "ON";

    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    auto result = run_cmd(test_env, bp::start_dir=(test_project_path), bp::shell, cmake_configure_command);
    if (result.return_code != 0) {
      BOOST_TEST_MESSAGE("Configure command failed with output:\n" << result.output);
    }
    BOOST_REQUIRE(result.return_code == 0);

    auto cmake_cache_path = test_project_path / "build" / "_deps" / "version_update-build" / "CMakeCache.txt";
    auto content_cmake_cache = pre::file::to_string(cmake_cache_path.generic_string());
    BOOST_REQUIRE(boost::contains(content_cmake_cache, "FAKECACHEDEP_MODE:STRING=v1"));

    auto content = pre::file::to_string((test_project_path/ "CMakeLists.txt").generic_string());
    boost::replace_all(content, first_commit, second_commit);
    boost::replace_all(content, first_commit_autotools, second_commit_autotools);
    pre::file::from_string((test_project_path/ "CMakeLists.txt").generic_string(), content);

    result = run_cmd(test_env, bp::start_dir=(test_project_path), bp::shell, cmake_configure_command);
    BOOST_REQUIRE(result.return_code == 0);
    content_cmake_cache = pre::file::to_string(cmake_cache_path.generic_string());
    BOOST_REQUIRE(boost::contains(content_cmake_cache, "FAKECACHEDEP_MODE:STRING=v2"));

    boost::replace_all(content, second_commit, first_commit);
    boost::replace_all(content, second_commit_autotools, first_commit_autotools);
    pre::file::from_string((test_project_path/ "CMakeLists.txt").generic_string(), content);
    result = run_cmd(test_env, bp::start_dir=(test_project_path), bp::shell, cmake_configure_command);
    BOOST_REQUIRE(result.return_code == 0);
    content_cmake_cache = pre::file::to_string(cmake_cache_path.generic_string());
    BOOST_REQUIRE(boost::contains(content_cmake_cache, "FAKECACHEDEP_MODE:STRING=v1"));
  }
}
