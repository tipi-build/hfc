#pragma once
#include <vector>
#include <iostream>
#include <optional>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/test/data/test_case.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>


namespace hfc::test {
  namespace fs = boost::filesystem;
  namespace bp = boost::process;



  struct test_variant{
    test_variant(const fs::path& cmake_bin_, bool is_cmake_re_, const std::string& cli_args_ = "", const std::string& configure_time_cli_args_ = "", const std::string& build_time_cli_args_ = ""){
      cmake_bin = cmake_bin_;
      is_cmake_re = is_cmake_re_;
      command_line_arguments = cli_args_;
      build_time_command_line_arguments = build_time_cli_args_;

      std::stringstream ss_configure_time_cli_args;
      ss_configure_time_cli_args << configure_time_cli_args_;

      if(is_cmake_re){
        ss_configure_time_cli_args << " -DCMAKE_RE_ENABLE=ON";
        ss_configure_time_cli_args << " -DUUID=" << to_string(boost::uuids::random_generator()());
      }

      configure_time_command_line_arguments = ss_configure_time_cli_args.str();
    }

    fs::path cmake_bin{};
    bool is_cmake_re{};
    std::string command_line_arguments{};
    std::string configure_time_command_line_arguments{};
    std::string build_time_command_line_arguments{};


    inline friend std::ostream& operator<<(std::ostream& os, const test_variant& data){
      os << "[ cmake_bin: '" << data.cmake_bin.generic_string() << "'"
        << ", is_cmake_re: '" << data.is_cmake_re << "'"
        << ", command_line_arguments: '" << data.command_line_arguments << "'"
        << ", configure_time_args_command_line_arguments: '" << data.configure_time_command_line_arguments << "'"
        << ", build_time_args_command_line_arguments: '" << data.build_time_command_line_arguments << "' ]";
      return os;
    }
  };


  inline std::vector<test_variant> test_variants() { 
    
    std::vector<test_variant> tests_variants_to_run{}; 

    auto cmake_path = bp::search_path("cmake");
    if (cmake_path.string().empty()) {
      auto error = std::runtime_error("ERROR: cmake is not found on PATH, tests can't be run.");
      std::cout << error.what() << std::endl;
      throw error;
    }
    tests_variants_to_run.push_back( test_variant{cmake_path, false} );

    auto cmake_re_path = bp::search_path("cmake-re");
    if (cmake_re_path.string().empty()) {
      auto error = std::runtime_error("ERROR: cmake-re is not found on PATH, tests can't be run.");
      std::cout << error.what() << std::endl;
      throw error;
    }
    tests_variants_to_run.push_back( test_variant{cmake_re_path, true, "-vv --host"} );  // host build
    
    if(std::getenv("HFC_TEST_ENABLE_CONTAINERIZED_BUILDS_TEST") == "ON") {
      tests_variants_to_run.push_back( test_variant{cmake_re_path, true, "-vv"} );       // /!\ no --host
    }
    
    
    return tests_variants_to_run;
  }
}