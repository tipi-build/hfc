#define BOOST_TEST_MODULE goldilock_provisioning_test
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>

#include <test_project.hpp>
#include <test_variant.hpp>
#include <test_helpers.hpp>
#include <test_isolation_fixture.hpp>

#include <pre/file/string.hpp>


namespace hfc::test {
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
  using namespace std::string_literals;

  struct move_a_goldilock_on_the_path {

    move_a_goldilock_on_the_path(const std::string& input, const fs::path& temp_dir, bp::environment& env)
      : test_env_(env)
      , temp_bin_dir_(temp_dir / "bin")
    {
      BOOST_REQUIRE_MESSAGE(test_env_.count(input),
                            "Required environment variable '"s + input + "' is not set");

      // Create temporary bin directory
      fs::create_directories(temp_bin_dir_);
      destination_path_ = temp_bin_dir_ / "goldilock";

      // Copy the goldilock binary to temp location
      fs::copy_file(fs::path(test_env_[input].to_string()), destination_path_,
                    fs::copy_options::overwrite_existing);

      // Make it executable
      fs::permissions(destination_path_,
                     fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exe |
                     fs::perms::others_read | fs::perms::others_exe);

      // Prepend temp bin directory to PATH
      original_path_ = test_env_["PATH"].to_string();
      test_env_["PATH"] = temp_bin_dir_.string() + ":" + original_path_;
    }

    ~move_a_goldilock_on_the_path() {
      // Restore original PATH
      test_env_["PATH"] = original_path_;

      // Clean up
      if(fs::exists(destination_path_)){
        fs::remove(destination_path_);
      }
    }

    bp::environment& test_env_;
    fs::path temp_bin_dir_;
    fs::path destination_path_;
    std::string original_path_;
  };


    struct remove_goldilock_from_path {
    remove_goldilock_from_path() {
      goldilock_path = bp::search_path("goldilock");
      if(goldilock_path.size() != 0){
        goldilock_rename = goldilock_path.parent_path() / "not_use_goldilock_path";
        rename(goldilock_path, goldilock_rename);
      }
    }

    ~remove_goldilock_from_path() {
      if(goldilock_path.size() != 0){
        rename(goldilock_rename, goldilock_path);
      }
    }

    fs::path goldilock_path{};
    fs::path goldilock_rename{};

  };

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, incompatible_goldilock_on_PATH_test, boost::unit_test::data::make(hfc::test::test_variants()), data){
    move_a_goldilock_on_the_path goldilock_move("FAKE_GOLDILOCK", temp_dir, test_env);
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"]="ON";
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE(boost::contains(output,"version v0.0.1"));
    BOOST_REQUIRE(boost::contains(output,"does not meet the minimum version"));
    BOOST_REQUIRE(boost::contains(output,"Downloading prebuilt goldilock from"));

  }

    BOOST_DATA_TEST_CASE_F(test_isolation_fixture, incompatible_goldilock_on_crash_PATH_test, boost::unit_test::data::make(hfc::test::test_variants()), data){
    move_a_goldilock_on_the_path goldilock_move("FAKE_GOLDILOCK", temp_dir, test_env);
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"]="ON";
    test_env["GOLDILOCK_CRASH"]="ON";
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    auto output = run_command(cmake_configure_command, template_path, test_env);
    std::cout<<output<<std::endl;

    BOOST_REQUIRE(boost::contains(output,"ran & matched version expectation: FALSE"));
    BOOST_REQUIRE(boost::contains(output,"Downloading prebuilt goldilock from"));

  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, compatible_goldilock_on_PATH_test, boost::unit_test::data::make(hfc::test::test_variants()), data){
    move_a_goldilock_on_the_path goldilock_move("TRUE_GOLDILOCK", temp_dir, test_env);
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"]="ON";
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE(boost::contains(output,"goldilock has been found on the system"));
    BOOST_REQUIRE(!boost::contains(output,"Downloading prebuilt goldilock from"));
  }


  //! Test the absolute default, no HERMETIC_FETCHCONTENT_LOG_DEBUG mode ON
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_goldilock_compatible_download_no_debug, boost::unit_test::data::make(hfc::test::test_variants()), data){
    remove_goldilock_from_path remove_goldilock{};
    test_env.erase("HERMETIC_FETCHCONTENT_LOG_DEBUG");
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE(boost::contains(output,"[download 100% complete]"));
    BOOST_REQUIRE(boost::contains(output,"goldilock has been downloaded"));
    BOOST_REQUIRE(!boost::contains(output,"Building goldilock from source"));
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_goldilock_compatible_download, boost::unit_test::data::make(hfc::test::test_variants()), data){
    remove_goldilock_from_path remove_goldilock{};
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"]="ON";
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE(boost::contains(output,"[download 100% complete]"));
    BOOST_REQUIRE(!boost::contains(output,"Building goldilock from source"));
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_goldilock_compatible_download_with_incompatible_goldilock_on_PATH, boost::unit_test::data::make(hfc::test::test_variants()), data){
    move_a_goldilock_on_the_path goldilock_move("FAKE_GOLDILOCK", temp_dir, test_env);
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"]="ON";
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE(boost::contains(output,"[download 100% complete]"));
    BOOST_REQUIRE(boost::contains(output,"goldilock has been downloaded"));
    BOOST_REQUIRE(!boost::contains(output,"Building goldilock from source"));
  }


  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_goldilock_imcompatible_download, boost::unit_test::data::make(hfc::test::test_variants()), data){
    move_a_goldilock_on_the_path goldilock_move("FAKE_GOLDILOCK", temp_dir, test_env);
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"]="ON";
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");
    fs::remove(template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake");
    fs::path hfc_goldilock_base_value_from_test = get_data_dir() / "hfc_goldilock_base_value.cmake";
    fs::copy(hfc_goldilock_base_value_from_test, (template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake"));

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE(boost::contains(output,"[download 100% complete]"));
    BOOST_REQUIRE(boost::contains(output,"Building goldilock from source"));
    BOOST_REQUIRE(boost::contains(output," Installing goldilock"));
  }



  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_goldilock_imcompatible_download_without_goldilock_on_path, boost::unit_test::data::make(hfc::test::test_variants()), data){
    remove_goldilock_from_path remove_goldilock{};
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"]="ON";
    
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");
    fs::remove(template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake");
    fs::path hfc_goldilock_base_value_from_test = get_data_dir() / "hfc_goldilock_base_value.cmake";
    fs::copy(hfc_goldilock_base_value_from_test, (template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake"));

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE(boost::contains(output,"[download 100% complete]"));
    BOOST_REQUIRE(boost::contains(output,"Building goldilock from source"));
    BOOST_REQUIRE(boost::contains(output," Installing goldilock"));
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_goldilock_crosscompiling, boost::unit_test::data::make(hfc::test::test_variants()), data){
    move_a_goldilock_on_the_path goldilock_move("FAKE_GOLDILOCK", temp_dir, test_env);
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"]="ON";
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");
    fs::remove(template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake");
    fs::path hfc_goldilock_base_value_from_test = get_data_dir() / "hfc_goldilock_base_value.cmake";
    fs::copy(hfc_goldilock_base_value_from_test, (template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake"));

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data, "" ,(template_path/ "toolchain" / "linux-toolchain-clang-crosscompile.cmake"));
    auto output = run_command(cmake_configure_command, template_path, test_env);

    std::string cmake_cache_goldilock_content = pre::file::to_string((template_path / "build" / ".hfc_tools" / "hfc_goldilock-build" / "CMakeCache.txt").generic_string());
    std::string cmake_cache_content = pre::file::to_string((template_path / "build" / "CMakeCache.txt").generic_string());
    std::string compiler_by_default_in_cache="CMAKE_CXX_COMPILER:FILEPATH="+test_env["STANDARD_COMPILER"].to_string();

    BOOST_REQUIRE(boost::contains(cmake_cache_goldilock_content,compiler_by_default_in_cache));
    fs::path clang_path = bp::search_path("clang++");
    std::string clang_in_cache = "CMAKE_CXX_COMPILER:STRING="+clang_path.generic_string();
    BOOST_REQUIRE(boost::contains(cmake_cache_content,clang_in_cache));
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_goldilock_no_crosscompiling, boost::unit_test::data::make(hfc::test::test_variants()), data){
    move_a_goldilock_on_the_path goldilock_move("FAKE_GOLDILOCK", temp_dir, test_env);
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"]="ON";
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");
    fs::remove(template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake");
    fs::path hfc_goldilock_base_value_from_test = get_data_dir() / "hfc_goldilock_base_value.cmake";
    fs::copy(hfc_goldilock_base_value_from_test, (template_path / "cmake" / "modules" / "hfc_goldilock_base_value.cmake"));

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data, "" ,(template_path/ "toolchain" / "linux-toolchain-clang.cmake"));
    auto output = run_command(cmake_configure_command, template_path, test_env);

    std::string cmake_cache_goldilock_content = pre::file::to_string((template_path / "build" / ".hfc_tools" / "hfc_goldilock-build" / "CMakeCache.txt").generic_string());
    std::string cmake_cache_content = pre::file::to_string((template_path / "build" / "CMakeCache.txt").generic_string());
    fs::path clang_path = bp::search_path("clang++");
    std::string clang_in_cache = "CMAKE_CXX_COMPILER:STRING="+clang_path.generic_string();
    BOOST_REQUIRE(boost::contains(cmake_cache_goldilock_content,clang_in_cache));
    BOOST_REQUIRE(boost::contains(cmake_cache_content,clang_in_cache));
  }

  BOOST_AUTO_TEST_CASE(check_goldilock_version_consistency) {
    // Get paths to both goldilock configuration files
    fs::path main_config = get_source_tree_dir() / "cmake" / "modules" / "hfc_goldilock_base_value.cmake";
    fs::path test_config = get_data_dir() / "hfc_goldilock_base_value.cmake";

    // Read both files
    BOOST_REQUIRE_MESSAGE(fs::exists(main_config),
                          "Main goldilock config not found at: " + main_config.string());
    BOOST_REQUIRE_MESSAGE(fs::exists(test_config),
                          "Test goldilock config not found at: " + test_config.string());

    std::string main_content = pre::file::to_string(main_config.string());
    std::string test_content = pre::file::to_string(test_config.string());

    // Helper lambda to extract value after a pattern
    auto extract_value = [](const std::string& content, const std::string& pattern) -> std::string {
      boost::regex regex(pattern + R"(\s+([^\s\)]+))");
      boost::smatch match;
      if (boost::regex_search(content, match, regex)) {
        return match[1].str();
      }
      return "";
    };

    // Extract versions and revisions
    std::string main_version = extract_value(main_content, "GOLDILOCK_MINIMUM_VERSION");
    std::string test_version = extract_value(test_content, "GOLDILOCK_MINIMUM_VERSION");
    std::string main_revision = extract_value(main_content, "GOLDILOCK_REVISION");
    std::string test_revision = extract_value(test_content, "GOLDILOCK_REVISION");

    // Verify values were extracted
    BOOST_REQUIRE_MESSAGE(!main_version.empty(),
                          "Failed to extract GOLDILOCK_MINIMUM_VERSION from main config");
    BOOST_REQUIRE_MESSAGE(!test_version.empty(),
                          "Failed to extract GOLDILOCK_MINIMUM_VERSION from test config");
    BOOST_REQUIRE_MESSAGE(!main_revision.empty(),
                          "Failed to extract GOLDILOCK_REVISION from main config");
    BOOST_REQUIRE_MESSAGE(!test_revision.empty(),
                          "Failed to extract GOLDILOCK_REVISION from test config");

    // Compare versions and revisions
    BOOST_CHECK_MESSAGE(main_version == test_version,
                        "GOLDILOCK_MINIMUM_VERSION mismatch!\n"
                        "  Main: " + main_version + "\n"
                        "  Test: " + test_version);

    BOOST_CHECK_MESSAGE(main_revision == test_revision,
                        "GOLDILOCK_REVISION mismatch!\n"
                        "  Main: " + main_revision + "\n"
                        "  Test: " + test_revision);
  }

}