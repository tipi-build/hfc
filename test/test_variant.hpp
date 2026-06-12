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
    test_variant(const fs::path& cmake_bin_, bool is_cmake_re_, const std::string& cli_args_ = "", const std::string& configure_time_cli_args_ = "", const std::string& build_time_cli_args_ = "", const std::string& toolchain_name_ = ""){
      cmake_bin = cmake_bin_;
      is_cmake_re = is_cmake_re_;
      command_line_arguments = cli_args_;
      build_time_command_line_arguments = build_time_cli_args_;
      toolchain_name = toolchain_name_;

      std::stringstream ss_configure_time_cli_args;
      ss_configure_time_cli_args << configure_time_cli_args_;

      if(is_cmake_re){
        ss_configure_time_cli_args << " -DCMAKE_RE_ENABLE=ON";
      }

      configure_time_command_line_arguments = ss_configure_time_cli_args.str();
    }

    fs::path cmake_bin{};
    bool is_cmake_re{};
    std::string command_line_arguments{};
    std::string configure_time_command_line_arguments{};
    std::string build_time_command_line_arguments{};
    std::string toolchain_name{};


    inline friend std::ostream& operator<<(std::ostream& os, const test_variant& data){
      os << "[ cmake_bin: '" << data.cmake_bin.generic_string() << "'"
        << ", is_cmake_re: '" << data.is_cmake_re << "'"
        << ", command_line_arguments: '" << data.command_line_arguments << "'"
        << ", configure_time_args_command_line_arguments: '" << data.configure_time_command_line_arguments << "'"
        << ", build_time_args_command_line_arguments: '" << data.build_time_command_line_arguments << "'"
        << ", toolchain_name: '" << data.toolchain_name << "' ]";
      return os;
    }
  };


  inline std::vector<test_variant> test_variants() {

    std::vector<test_variant> tests_variants_to_run{};

    bool skip_cmake = std::getenv("HFC_TEST_SKIP_CMAKE") != nullptr;
    bool skip_cmake_re = std::getenv("HFC_TEST_SKIP_CMAKE_RE") != nullptr;

    if (skip_cmake && skip_cmake_re) {
      throw std::runtime_error("Both HFC_TEST_SKIP_CMAKE and HFC_TEST_SKIP_CMAKE_RE are set, no test variants to run.");
    }

    if(!skip_cmake) {
      auto cmake_path = bp::search_path("cmake");
      if (cmake_path.string().empty()) {
        throw std::runtime_error("cmake is not found on PATH, tests can't be run.");
      }
      tests_variants_to_run.push_back( test_variant{cmake_path, false} );
    }

    if(!skip_cmake_re) {
      auto cmake_re_path = bp::search_path("cmake-re");
      if (cmake_re_path.string().empty()) {
        throw std::runtime_error("cmake-re is not found on PATH, tests can't be run.");
      }
      tests_variants_to_run.push_back( test_variant{cmake_re_path, true, "-vv --host"} );  // host build

      if(std::getenv("HFC_TEST_ENABLE_CONTAINERIZED_BUILDS_TEST") == "ON") {
        tests_variants_to_run.push_back( test_variant{cmake_re_path, true, "-vv"} );       // /!\ no --host
      }
    }

    return tests_variants_to_run;
  }

  inline std::vector<test_variant> test_variants_gcc_and_clang() {

    std::vector<test_variant> tests_variants_to_run{};

    bool skip_cmake = std::getenv("HFC_TEST_SKIP_CMAKE") != nullptr;
    bool skip_cmake_re = std::getenv("HFC_TEST_SKIP_CMAKE_RE") != nullptr;

    if (skip_cmake && skip_cmake_re) {
      throw std::runtime_error("Both HFC_TEST_SKIP_CMAKE and HFC_TEST_SKIP_CMAKE_RE are set, no test variants to run.");
    }

    std::vector<std::pair<std::string, std::string>> toolchains = {
      {"linux-toolchain.cmake", "gcc"},
      {"linux-toolchain-clang.cmake", "clang"}
    };

    if(!skip_cmake) {
      auto cmake_path = bp::search_path("cmake");
      if (cmake_path.string().empty()) {
        throw std::runtime_error("cmake is not found on PATH, tests can't be run.");
      }

      for (const auto& [toolchain, compiler] : toolchains) {
        auto compiler_path = bp::search_path(compiler);
        if (compiler_path.string().empty()) {
          std::cout << "WARNING: " << compiler << " is not found on PATH, skipping tests with " << toolchain << std::endl;
          continue;
        }

        tests_variants_to_run.push_back( test_variant{cmake_path, false, "", "", "", toolchain} );
      }
    }

    if(!skip_cmake_re) {
      auto cmake_re_path = bp::search_path("cmake-re");
      if (cmake_re_path.string().empty()) {
        throw std::runtime_error("cmake-re is not found on PATH, tests can't be run.");
      }

      for (const auto& [toolchain, compiler] : toolchains) {
        auto compiler_path = bp::search_path(compiler);
        if (compiler_path.string().empty()) {
          continue;
        }

        tests_variants_to_run.push_back( test_variant{cmake_re_path, true, "-vv --host", "", "", toolchain} );  // host build

        if(std::getenv("HFC_TEST_ENABLE_CONTAINERIZED_BUILDS_TEST") == "ON") {
          tests_variants_to_run.push_back( test_variant{cmake_re_path, true, "-vv", "", "", toolchain} );       // /!\ no --host
        }
      }
    }

    return tests_variants_to_run;
  }
}