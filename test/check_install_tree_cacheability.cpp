#define BOOST_TEST_MODULE check_install_tree_cacheability
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

  BOOST_DATA_TEST_CASE(check_install_tree_cacheability, boost::unit_test::data::make(test_variants()), data) {

    fs::path test_project_path = prepare_project_to_be_tested("check_install-reuse", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);


    auto check_that_nothing_got_built_at_configure_time = [&]() { 
      BOOST_REQUIRE(fs::exists(test_project_path / "build" / "CMakeCache.txt" ));
      BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "MyExample" ));
      BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "MySimpleMain" ));
      BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-install" / "lib" / "libMathFunctions.a"));
      BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-install" / "lib" / "libMathFunctionscbrt.a"));
    };

    auto check_that_expected_binaries_were_built = [&]() {
      BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
      BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MySimpleMain" ));
      BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-install" / "lib" / "libMathFunctions.a"));
      BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-install" / "lib" / "libMathFunctionscbrt.a"));
    };

    auto remove_all_built_files_except_deps_install_trees = [&]() {
      fs::remove_all(test_project_path / "build" / "_deps" / "project-cmake-simple-build");
      fs::remove_all(test_project_path / "build" / "_deps" / "project-cmake-simple-build-ep");
      fs::remove_all(test_project_path / "build" / "_deps" / "project-cmake-simple-src");
      fs::remove_all(test_project_path / "build" / "_deps" / "project-cmake-simple-subbuild");
      for (const auto& entry : fs::directory_iterator{test_project_path / "build"}) {
        if(!boost::contains(entry.path().filename().string(),"deps") ){
          fs::remove_all(entry.path());
        }
      }
    };

    bp::environment test_environment = boost::this_process::environment();
    test_environment["TIPI_DISABLE_SET_MTIME"] = "ON";
    auto cmake_run_configure = [&](std::future<std::string>& out_configure) {
      std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
      std::cout << "Configure command: " << cmake_configure_command << std::endl;
      int result_configure = bp::system(test_environment, bp::start_dir=(test_project_path), cmake_configure_command,  bp::std_out > out_configure, bp::std_err > stderr, bp::std_in < stdin);
      return result_configure;
    };

    auto cmake_run_build = [&](std::future<std::string>& out_build)  {
      std::string cmake_build_command = get_cmake_build_command(test_project_path, data, "-d explain");
      std::cout << "Build command: " << cmake_build_command << std::endl;
      int result = bp::system(test_environment, bp::start_dir=(test_project_path), cmake_build_command,  bp::std_out > out_build, bp::std_err > stderr, bp::std_in < stdin);
      return result;
    };

    write_simple_main(test_project_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(test_project_path,{}, "simple_main.cpp");

    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);

    // First configure + build, Expectation : everything builds from source
    {
      std::future<std::string> out_configure;
      auto result_configure = cmake_run_configure(out_configure);
      std::cout << "configure output is :\n" << out_configure.get() << std::endl;

      check_that_nothing_got_built_at_configure_time();
      BOOST_REQUIRE(result_configure  == 0);

      std::future<std::string> out_build;
      auto result_build = cmake_run_build(out_build);
      std::cout << "build output is :\n" << out_build.get() << std::endl;

      check_that_expected_binaries_were_built();
      BOOST_REQUIRE(result_build  == 0);
    }

    remove_all_built_files_except_deps_install_trees();

    // Second configure + build, Expectation : only the main project rebuilds, dependency taken from cache
    {
      std::future<std::string> out_configure;
      auto result_configure = cmake_run_configure(out_configure);

      std::string configure_output = out_configure.get();
      std::cout << "configure output is : \n" << configure_output << std::endl;
      BOOST_REQUIRE(!boost::contains(configure_output, "Configure done project-cmake-simple"));
      BOOST_REQUIRE(result_configure == 0);
      
      std::future<std::string> out_build;
      auto result_build = cmake_run_build(out_build);

      std::string ninja_output = out_build.get();
      std::cout<<"build output is :\n"<< ninja_output << std::endl;

      BOOST_REQUIRE(!boost::contains(ninja_output, "Building CXX object CMakeFiles/MathFunctionscbrt.dir/MathFunctionscbrt.cxx.o"));
      BOOST_REQUIRE(!boost::contains(ninja_output, "Building CXX object CMakeFiles/MathFunctions.dir/MathFunctions.cxx.o"));

      BOOST_REQUIRE(result_build == 0);
    }

    // Third build (no reconfigure), Expectation : no work to do. 
    {
      std::future<std::string> out_build;
      auto result_build = cmake_run_build(out_build);

      std::string ninja_output = out_build.get();
      std::cout<<"build output is :\n"<< ninja_output << std::endl;
        
      BOOST_REQUIRE(boost::contains(ninja_output, "ninja: no work to do"));
      BOOST_REQUIRE(result_build == 0);
    }

    // Change a file that is not using the dependency, Expectation : nothing else should rebuild
    {
      std::string new_main = R"(#include <iostream>
      int main(){std::cout<<"Hello, world!"<<std::endl; return 0;})";
      pre::file::from_string((test_project_path / "simple_main.cpp").generic_string(), new_main);

      if(data.is_cmake_re) {
        git_commit_change("first commit", test_project_path, test_environment);
      }

      std::future<std::string> out_build;
      auto result_build = cmake_run_build(out_build);

      std::string ninja_output = out_build.get();
      std::cout<<"build output is :\n"<< ninja_output << std::endl;
        
      BOOST_REQUIRE(boost::contains(ninja_output, "Building CXX object CMakeFiles/MySimpleMain.dir/simple_main.cpp.o"));
      BOOST_REQUIRE(!boost::contains(ninja_output, "Building CXX object CMakeFiles/MyExample.dir/simple_example.cpp.o"));
      BOOST_REQUIRE(result_build == 0);
    }
    
  }
}