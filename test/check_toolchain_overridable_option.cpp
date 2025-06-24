#define BOOST_TEST_MODULE check_toolchain_overridable_option
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/process.hpp>

#include <test_project.hpp>
#include <test_variant.hpp>
#include <test_helpers.hpp>

#include <pre/file/string.hpp>

namespace hfc::test {
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
  using namespace std::string_literals;


  static inline boost::regex echo_AN_OVERRIDABLE_OPTION_rx{"AN_OVERRIDABLE_OPTION (?<stage>[A-Z-]+) SCOPE:'(?<scope>[^']*)' CACHE:'(?<cache>[^']*)'"};

  struct inspect_option_value {

    std::vector<std::string> results;
    std::map<std::string, std::string> cache_values;
    std::map<std::string, std::string> scope_values;

    inspect_option_value(const std::string &process_output) {
      boost::algorithm::find_all_regex(results, process_output, echo_AN_OVERRIDABLE_OPTION_rx);

      for (auto result : results) {
        boost::smatch what;
        boost::regex_match(result, what, echo_AN_OVERRIDABLE_OPTION_rx);
        BOOST_TEST_MESSAGE( what["stage"] << " - scope:" << what["scope"] << ", cache:" << what["cache"] );
        scope_values[what["stage"]] = what["scope"];
        cache_values[what["stage"]] = what["cache"];

      }
    }
  };

  BOOST_DATA_TEST_CASE(project_option, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_toolchain_overridable_option", data.is_cmake_re);

    auto toolchain_without_option = test_project_path.string() + "/toolchain/toolchain_without_option.cmake "s;

    // Keeping default
    {
      std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data, "", toolchain_without_option);
      auto cmake_configure_output = run_command(cmake_configure_command, test_project_path);
      
      std::vector<std::string> toolchain_default_warning_occurences;
      boost::algorithm::find_all(toolchain_default_warning_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION toolchain default changed");
      BOOST_REQUIRE(toolchain_default_warning_occurences.empty());

      std::vector<std::string> project_default_warning_occurences;
      boost::algorithm::find_all(project_default_warning_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION project default changed");
      BOOST_REQUIRE(project_default_warning_occurences.empty());
      
      inspect_option_value configure{cmake_configure_output};

      BOOST_REQUIRE_EQUAL(configure.scope_values["BEFORE-TOOLCHAIN-LOADED"], "");
      BOOST_REQUIRE_EQUAL(configure.scope_values["BEFORE-TOOLCHAIN-LOADED"], configure.cache_values["BEFORE-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure.scope_values["AFTER-TOOLCHAIN-LOADED"], "");
      BOOST_REQUIRE_EQUAL(configure.scope_values["AFTER-TOOLCHAIN-LOADED"], configure.cache_values["AFTER-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure.scope_values["FINAL"], "option-in-project");
      BOOST_REQUIRE_EQUAL(configure.scope_values["FINAL"], configure.cache_values["FINAL"]);
    }


    
    // Overriding default
    {
      std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data, "-DAN_OVERRIDABLE_OPTION=override-from-cli",  toolchain_without_option);
      auto cmake_configure_output = run_command(cmake_configure_command, test_project_path);

      std::vector<std::string> toolchain_default_warning_occurences;
      boost::algorithm::find_all(toolchain_default_warning_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION toolchain default changed");
      BOOST_REQUIRE(toolchain_default_warning_occurences.empty());

      std::vector<std::string> project_default_warning_occurences;
      boost::algorithm::find_all(project_default_warning_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION project default changed");
      BOOST_REQUIRE(project_default_warning_occurences.empty());
      
      
      inspect_option_value configure_override{cmake_configure_output};

      BOOST_REQUIRE_EQUAL(configure_override.scope_values["BEFORE-TOOLCHAIN-LOADED"], "override-from-cli");
      BOOST_REQUIRE_EQUAL(configure_override.scope_values["BEFORE-TOOLCHAIN-LOADED"], configure_override.cache_values["BEFORE-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure_override.scope_values["AFTER-TOOLCHAIN-LOADED"], "override-from-cli");
      BOOST_REQUIRE_EQUAL(configure_override.scope_values["AFTER-TOOLCHAIN-LOADED"], configure_override.cache_values["AFTER-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure_override.scope_values["FINAL"], "override-from-cli");
      BOOST_REQUIRE_EQUAL(configure_override.scope_values["FINAL"], configure_override.cache_values["FINAL"]);
    }

    // Unsetting entry, setting back to unset (expecting default)
    {
      std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data, "-UAN_OVERRIDABLE_OPTION",  toolchain_without_option);
      auto cmake_configure_output = run_command(cmake_configure_command, test_project_path);

      std::vector<std::string> toolchain_default_warning_occurences;
      boost::algorithm::find_all(toolchain_default_warning_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION toolchain default changed");
      BOOST_REQUIRE(toolchain_default_warning_occurences.empty());

      std::vector<std::string> project_default_warning_occurences;
      boost::algorithm::find_all(project_default_warning_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION project default changed");
      BOOST_REQUIRE(project_default_warning_occurences.empty());

      inspect_option_value configure_undef{cmake_configure_output};

      BOOST_REQUIRE_EQUAL(configure_undef.scope_values["BEFORE-TOOLCHAIN-LOADED"], "");
      BOOST_REQUIRE_EQUAL(configure_undef.scope_values["BEFORE-TOOLCHAIN-LOADED"], configure_undef.cache_values["BEFORE-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure_undef.scope_values["AFTER-TOOLCHAIN-LOADED"], "");
      BOOST_REQUIRE_EQUAL(configure_undef.scope_values["AFTER-TOOLCHAIN-LOADED"], configure_undef.cache_values["AFTER-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure_undef.scope_values["FINAL"], "option-in-project");
      BOOST_REQUIRE_EQUAL(configure_undef.scope_values["FINAL"], configure_undef.cache_values["FINAL"]);
    }

  }


  BOOST_DATA_TEST_CASE(toolchain_option, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_toolchain_overridable_option", data.is_cmake_re);

    auto toolchain_with_option = test_project_path.string() + "/toolchain/toolchain_with_option.cmake"s;

    // Keeping default
    {
      std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data, "", toolchain_with_option);
      auto cmake_configure_output = run_command(cmake_configure_command, test_project_path);

      std::vector<std::string> toolchain_default_warning_occurences;
      boost::algorithm::find_all(toolchain_default_warning_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION toolchain default changed");
      BOOST_REQUIRE(toolchain_default_warning_occurences.empty());

      std::vector<std::string> project_default_warning_occurences;
      boost::algorithm::find_all(project_default_warning_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION project default changed");
      BOOST_REQUIRE(project_default_warning_occurences.empty());
      
      inspect_option_value configure{cmake_configure_output};

      BOOST_REQUIRE_EQUAL(configure.scope_values["BEFORE-TOOLCHAIN-LOADED"], "");
      BOOST_REQUIRE_EQUAL(configure.scope_values["BEFORE-TOOLCHAIN-LOADED"], configure.cache_values["BEFORE-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure.scope_values["AFTER-TOOLCHAIN-LOADED"], "value-in-toolchain");
      BOOST_REQUIRE_EQUAL(configure.scope_values["AFTER-TOOLCHAIN-LOADED"], configure.cache_values["AFTER-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure.scope_values["FINAL"], "value-in-toolchain");
      BOOST_REQUIRE_EQUAL(configure.scope_values["FINAL"], configure.cache_values["FINAL"]);
    }


    
    // Overriding default
    {
      std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data, "-DAN_OVERRIDABLE_OPTION=override-from-cli", toolchain_with_option);
      auto cmake_configure_output = run_command(cmake_configure_command, test_project_path);
      
      std::vector<std::string> toolchain_default_warning_occurences;
      boost::algorithm::find_all(toolchain_default_warning_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION toolchain default changed");
      BOOST_REQUIRE(toolchain_default_warning_occurences.empty());

      std::vector<std::string> project_default_warning_occurences;
      boost::algorithm::find_all(project_default_warning_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION project default changed");
      BOOST_REQUIRE(project_default_warning_occurences.empty());

      inspect_option_value configure_override{cmake_configure_output};

      BOOST_REQUIRE_EQUAL(configure_override.scope_values["BEFORE-TOOLCHAIN-LOADED"], "override-from-cli");
      BOOST_REQUIRE_EQUAL(configure_override.scope_values["BEFORE-TOOLCHAIN-LOADED"], configure_override.cache_values["BEFORE-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure_override.scope_values["AFTER-TOOLCHAIN-LOADED"], "override-from-cli");
      BOOST_REQUIRE_EQUAL(configure_override.scope_values["AFTER-TOOLCHAIN-LOADED"], configure_override.cache_values["AFTER-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure_override.scope_values["FINAL"], "override-from-cli");
      BOOST_REQUIRE_EQUAL(configure_override.scope_values["FINAL"], configure_override.cache_values["FINAL"]);
    }

    // Toolchain Maintainer changes default
    {
      auto toolchain_content = pre::file::to_string(toolchain_with_option);
      boost::algorithm::replace_all(toolchain_content, "AN_OVERRIDABLE_OPTION \"Documentation string\" \"value-in-toolchain\"", "AN_OVERRIDABLE_OPTION \"Documentation string\" \"changed-value-in-toolchain\"");
      pre::file::from_string(toolchain_with_option, toolchain_content); 

      auto cmake_build_output = run_command(get_cmake_build_command(test_project_path, data), test_project_path);
      
      std::vector<std::string> toolchain_default_warning_occurences;
      boost::algorithm::find_all(toolchain_default_warning_occurences, cmake_build_output, "AN_OVERRIDABLE_OPTION toolchain default changed");
      BOOST_REQUIRE(toolchain_default_warning_occurences.size() == 1);

      std::vector<std::string> project_default_warning_occurences;
      boost::algorithm::find_all(project_default_warning_occurences, cmake_build_output, "A_OVERRIDABLE_OPTION project default changed");
      BOOST_REQUIRE(project_default_warning_occurences.empty());

      inspect_option_value configure_override{cmake_build_output};

      BOOST_REQUIRE_EQUAL(configure_override.scope_values["BEFORE-TOOLCHAIN-LOADED"], "override-from-cli");
      BOOST_REQUIRE_EQUAL(configure_override.scope_values["BEFORE-TOOLCHAIN-LOADED"], configure_override.cache_values["BEFORE-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure_override.scope_values["AFTER-TOOLCHAIN-LOADED"], "override-from-cli");
      BOOST_REQUIRE_EQUAL(configure_override.scope_values["AFTER-TOOLCHAIN-LOADED"], configure_override.cache_values["AFTER-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure_override.scope_values["FINAL"], "override-from-cli");
      BOOST_REQUIRE_EQUAL(configure_override.scope_values["FINAL"], configure_override.cache_values["FINAL"]);
    }


    // Unsetting entry, setting back to unset (expecting default)
    {
      std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data, "-UAN_OVERRIDABLE_OPTION",  toolchain_with_option);
      auto cmake_configure_output = run_command(cmake_configure_command, test_project_path);

      std::vector<std::string> toolchain_default_warning_occurences;
      boost::algorithm::find_all(toolchain_default_warning_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION toolchain default changed");
      BOOST_REQUIRE(toolchain_default_warning_occurences.empty());

      std::vector<std::string> project_default_warning_occurences;
      boost::algorithm::find_all(project_default_warning_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION project default changed");
      BOOST_REQUIRE(project_default_warning_occurences.empty());

      inspect_option_value configure_undef{cmake_configure_output};

      BOOST_REQUIRE_EQUAL(configure_undef.scope_values["BEFORE-TOOLCHAIN-LOADED"], "");
      BOOST_REQUIRE_EQUAL(configure_undef.scope_values["BEFORE-TOOLCHAIN-LOADED"], configure_undef.cache_values["BEFORE-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure_undef.scope_values["AFTER-TOOLCHAIN-LOADED"], "changed-value-in-toolchain");
      BOOST_REQUIRE_EQUAL(configure_undef.scope_values["AFTER-TOOLCHAIN-LOADED"], configure_undef.cache_values["AFTER-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure_undef.scope_values["FINAL"], "changed-value-in-toolchain");
      BOOST_REQUIRE_EQUAL(configure_undef.scope_values["FINAL"], configure_undef.cache_values["FINAL"]);
    }
        
  }

  BOOST_DATA_TEST_CASE(non_overridable_option, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_toolchain_overridable_option", data.is_cmake_re);

    auto toolchain_with_non_overridable_option = test_project_path.string() + "/toolchain/toolchain_with_non_overridable_option.cmake"s;

    // Trying to override default
    {
      std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data, "-DAN_OVERRIDABLE_OPTION=override-from-cli", toolchain_with_non_overridable_option);

      auto result = run_cmd(boost::this_process::environment(), bp::start_dir=(test_project_path), bp::shell, cmake_configure_command);
      BOOST_TEST_MESSAGE(result.output);
      BOOST_REQUIRE_NE(result.return_code, 0);
      std::string cmake_configure_output = result.output;

      std::vector<std::string> override_error_occurences;
      boost::algorithm::find_all(override_error_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION is a non-overridable default");
      BOOST_REQUIRE(override_error_occurences.size() == 1);
    }

   }

   BOOST_DATA_TEST_CASE(non_overridable_option_CXX_standard, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_toolchain_overridable_option", data.is_cmake_re);

    auto toolchain_with_non_overridable_option = test_project_path.string() + "/toolchain/toolchain_with_non_overridable_option.cmake"s;

    // Trying to override default
    {
      std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data, "-DCMAKE_CXX_STANDARD=23", toolchain_with_non_overridable_option);

      auto result = run_cmd(boost::this_process::environment(), bp::start_dir=(test_project_path), bp::shell, cmake_configure_command);
      BOOST_TEST_MESSAGE(result.output);
      BOOST_REQUIRE_NE(result.return_code, 0);
      std::string cmake_configure_output = result.output;

      std::vector<std::string> override_error_occurences;
      boost::algorithm::find_all(override_error_occurences, cmake_configure_output, "CMAKE_CXX_STANDARD is a non-overridable default");
      BOOST_REQUIRE(override_error_occurences.size() == 1);
    }

   }

   BOOST_DATA_TEST_CASE(non_overridable_option_later, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_toolchain_overridable_option", data.is_cmake_re);

    auto toolchain_with_non_overridable_option = test_project_path.string() + "/toolchain/toolchain_with_non_overridable_option.cmake"s;

    // Keeping default
    {
      std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data, "", toolchain_with_non_overridable_option);
      auto cmake_configure_output = run_command(cmake_configure_command, test_project_path);

      std::vector<std::string> toolchain_default_warning_occurences;
      boost::algorithm::find_all(toolchain_default_warning_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION toolchain default changed");
      BOOST_REQUIRE(toolchain_default_warning_occurences.empty());

      std::vector<std::string> project_default_warning_occurences;
      boost::algorithm::find_all(project_default_warning_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION project default changed");
      BOOST_REQUIRE(project_default_warning_occurences.empty());
      
      inspect_option_value configure{cmake_configure_output};

      BOOST_REQUIRE_EQUAL(configure.scope_values["BEFORE-TOOLCHAIN-LOADED"], "");
      BOOST_REQUIRE_EQUAL(configure.scope_values["BEFORE-TOOLCHAIN-LOADED"], configure.cache_values["BEFORE-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure.scope_values["AFTER-TOOLCHAIN-LOADED"], "value-in-toolchain");
      BOOST_REQUIRE_EQUAL(configure.scope_values["AFTER-TOOLCHAIN-LOADED"], configure.cache_values["AFTER-TOOLCHAIN-LOADED"]);

      BOOST_REQUIRE_EQUAL(configure.scope_values["FINAL"], "value-in-toolchain");
      BOOST_REQUIRE_EQUAL(configure.scope_values["FINAL"], configure.cache_values["FINAL"]);
    }

    // Trying to override default
    {
      std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data, "-DAN_OVERRIDABLE_OPTION=override-from-cli", toolchain_with_non_overridable_option);

      auto result = run_cmd(boost::this_process::environment(), bp::start_dir=(test_project_path), bp::shell, cmake_configure_command);
      BOOST_TEST_MESSAGE(result.output);
      BOOST_REQUIRE_NE(result.return_code, 0);
      std::string cmake_configure_output = result.output;

      std::vector<std::string> override_error_occurences;
      boost::algorithm::find_all(override_error_occurences, cmake_configure_output, "AN_OVERRIDABLE_OPTION is a non-overridable default");
      BOOST_REQUIRE(override_error_occurences.size() == 1);
    }

   }

}