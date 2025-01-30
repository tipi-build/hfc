#define BOOST_TEST_MODULE goldilock_provisioning_test
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

  struct move_a_goldilock_on_the_path {

    move_a_goldilock_on_the_path(const std::string input) {
      bp::environment test_env = boost::this_process::environment();
      if (!test_env.count(input)) {
        BOOST_REQUIRE(false);
      }
      if(fs::exists(destination_path_)){
        fs::remove(destination_path_);
      }
      fs::copy(fs::path(test_env[input].to_string()),destination_path_);
    }

    ~move_a_goldilock_on_the_path() {
      if(fs::exists(destination_path_)){
        fs::remove(destination_path_);
      }
    }

    fs::path destination_path_ = "/usr/local/bin/goldilock";

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

  BOOST_DATA_TEST_CASE(incompatible_goldilock_on_PATH_test, boost::unit_test::data::make(hfc::test::test_variants()), data){
    move_a_goldilock_on_the_path goldilock_move("FAKE_GOLDILOCK");
    bp::environment test_env = boost::this_process::environment();
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"]="ON";
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE(boost::contains(output,"/usr/local/bin/goldilock is at version v0.0.1"));
    BOOST_REQUIRE(boost::contains(output,"does not meet the minimum version"));
    BOOST_REQUIRE(boost::contains(output,"Downloading prebuilt goldilock from"));

  }

    BOOST_DATA_TEST_CASE(incompatible_goldilock_on_crash_PATH_test, boost::unit_test::data::make(hfc::test::test_variants()), data){
    move_a_goldilock_on_the_path goldilock_move("FAKE_GOLDILOCK");
    bp::environment test_env = boost::this_process::environment();
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

  BOOST_DATA_TEST_CASE(compatible_goldilock_on_PATH_test, boost::unit_test::data::make(hfc::test::test_variants()), data){
    move_a_goldilock_on_the_path goldilock_move("TRUE_GOLDILOCK");
    bp::environment test_env = boost::this_process::environment();
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
  BOOST_DATA_TEST_CASE(check_goldilock_compatible_download_no_debug, boost::unit_test::data::make(hfc::test::test_variants()), data){
    remove_goldilock_from_path remove_goldilock{};
    bp::environment test_env = boost::this_process::environment();
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

  BOOST_DATA_TEST_CASE(check_goldilock_compatible_download, boost::unit_test::data::make(hfc::test::test_variants()), data){
    remove_goldilock_from_path remove_goldilock{};
    bp::environment test_env = boost::this_process::environment();
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"]="ON";
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE(boost::contains(output,"[download 100% complete]"));
    BOOST_REQUIRE(!boost::contains(output,"Building goldilock from source"));
  }

  BOOST_DATA_TEST_CASE(check_goldilock_compatible_download_with_incompatible_goldilock_on_PATH, boost::unit_test::data::make(hfc::test::test_variants()), data){
    move_a_goldilock_on_the_path goldilock_move("FAKE_GOLDILOCK");
    bp::environment test_env = boost::this_process::environment();
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


  BOOST_DATA_TEST_CASE(check_goldilock_imcompatible_download, boost::unit_test::data::make(hfc::test::test_variants()), data){
    move_a_goldilock_on_the_path goldilock_move("FAKE_GOLDILOCK");
    bp::environment test_env = boost::this_process::environment();
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



  BOOST_DATA_TEST_CASE(check_goldilock_imcompatible_download_without_goldilock_on_path, boost::unit_test::data::make(hfc::test::test_variants()), data){
    remove_goldilock_from_path remove_goldilock{};
    bp::environment test_env = boost::this_process::environment();
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

  BOOST_DATA_TEST_CASE(check_goldilock_crosscompiling, boost::unit_test::data::make(hfc::test::test_variants()), data){
    move_a_goldilock_on_the_path goldilock_move("FAKE_GOLDILOCK");
    bp::environment test_env = boost::this_process::environment();
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

  BOOST_DATA_TEST_CASE(check_goldilock_no_crosscompiling, boost::unit_test::data::make(hfc::test::test_variants()), data){
    move_a_goldilock_on_the_path goldilock_move("FAKE_GOLDILOCK");
    bp::environment test_env = boost::this_process::environment();
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

}