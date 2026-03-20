#define BOOST_TEST_MODULE check_update_version
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
  using namespace std::string_literals;

  BOOST_DATA_TEST_CASE(check_version_update, boost::unit_test::data::make(hfc::test::test_variants()), data){
    std::string first_commit = "790f82e8a01b062b34133ef71dd94e9468717f37";
    std::string second_commit = "5cfd9d4e490d910acef72782e058739a83837305";
    fs::path test_project_path = prepare_project_to_be_tested("check_version_update", data.is_cmake_re);
    write_simple_main(test_project_path, {"version.hpp"});
    write_simple_main(test_project_path, {}, "simple_main.cpp" );

    auto test_environment = boost::this_process::environment();
    test_environment["TIPI_CACHE_FORCE_ENABLE"] = "OFF";
    test_environment["TIPI_CACHE_CONSUME_ONLY"] = "ON";

    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    auto result = run_cmd(test_environment, bp::start_dir=(test_project_path), bp::shell, cmake_configure_command);
    BOOST_REQUIRE(result.return_code == 0);

    auto cmake_cache_path = test_project_path / "build" / "_deps" / "version_update-build" / "CMakeCache.txt";
    auto content_cmake_cache = pre::file::to_string(cmake_cache_path.generic_string());
    BOOST_REQUIRE(boost::contains(content_cmake_cache, "FAKECACHEDEP_MODE:STRING=v1"));

    auto content = pre::file::to_string((test_project_path/ "CMakeLists.txt").generic_string());
    boost::replace_all(content, first_commit, second_commit);
    pre::file::from_string((test_project_path/ "CMakeLists.txt").generic_string(), content);

    result = run_cmd(test_environment, bp::start_dir=(test_project_path), bp::shell, cmake_configure_command);
    BOOST_REQUIRE(result.return_code == 0);
    content_cmake_cache = pre::file::to_string(cmake_cache_path.generic_string());
    BOOST_REQUIRE(boost::contains(content_cmake_cache, "FAKECACHEDEP_MODE:STRING=v2"));
  }
}
