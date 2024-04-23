#define BOOST_TEST_MODULE check_install_tree_cacheability_without_configure
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

  BOOST_DATA_TEST_CASE(check_install_tree_cacheability_without_configure, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_install-reuse", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    write_simple_main(test_project_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(test_project_path,{}, "simple_main.cpp");
    bp::environment test_environment = boost::this_process::environment();
    test_environment["TIPI_DISABLE_SET_MTIME"] = "ON";
    
    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);

    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path, test_environment);

    int result_configure = bp::system(bp::start_dir=(test_project_path), test_environment, cmake_configure_command,  bp::std_out > stdout, bp::std_err > stderr, bp::std_in < stdin);
    BOOST_REQUIRE(result_configure == 0 );

    for (const auto& entry : fs::recursive_directory_iterator{test_project_path / "build"}){
      if(fs::is_regular_file(entry.path())){
        BOOST_REQUIRE(entry.path().filename().generic_string() != "MyExample");
        BOOST_REQUIRE(entry.path().filename().generic_string() != "MySimpleMain");
        BOOST_REQUIRE(entry.path().filename().generic_string() != "libMathFunctions.a");
        BOOST_REQUIRE(entry.path().filename().generic_string() != "libMathFunctionscbrt.a");
      }
    }

    std::string cmake_build_command = get_cmake_build_command(test_project_path, data);
    run_command(cmake_build_command, test_project_path, test_environment);

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MySimpleMain" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-install" / "lib" / "libMathFunctionscbrt.a"));

    fs::remove_all(test_project_path / "build" / "_deps" / "project-cmake-simple-build");
    fs::remove_all(test_project_path / "build" / "_deps" / "project-cmake-simple-src");
    fs::remove_all(test_project_path / "build" / "_deps" / "project-cmake-simple-subbuild");
    std::future<std::string> out;

    if(data.is_cmake_re){
      //check that we dont download it again with cmake-re and we use the install tree instead of re-download source and rebuild it 
      BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-build"));
      BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-src"));
      BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-subbuild"));
    }

    std::string ninja_output = run_command(cmake_build_command, test_project_path, test_environment);
    BOOST_REQUIRE(boost::contains(ninja_output, "ninja: no work to do"));
      
    std::string content = pre::file::to_string( (test_project_path / "simple_example.cpp").generic_string());
    content = content+"\n\n";
    pre::file::from_string((test_project_path / "simple_example.cpp").generic_string(), content);

    if(data.is_cmake_re){
      git_commit_change("first commit", test_project_path, test_environment);
    }

    ninja_output = run_command(cmake_build_command, test_project_path, test_environment);
    BOOST_REQUIRE(boost::contains(ninja_output, "Building CXX object CMakeFiles/MyExample.dir/simple_example.cpp.o"));
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-build"));
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-src"));
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-subbuild"));
    BOOST_REQUIRE(!boost::contains(ninja_output, "Building CXX object CMakeFiles/MySimpleMain.dir/simple_main.cpp.o"));

    std::string new_main = R"(#include <iostream>
    int main(){std::cout<<"Hello, world!"<<std::endl; return 0;})";
    pre::file::from_string((test_project_path / "simple_main.cpp").generic_string(), new_main);

    if(data.is_cmake_re){
      git_commit_change("second commit", test_project_path, test_environment);
    }

    ninja_output = run_command(cmake_build_command, test_project_path, test_environment);
    BOOST_REQUIRE(boost::contains(ninja_output, "Building CXX object CMakeFiles/MySimpleMain.dir/simple_main.cpp.o"));
    BOOST_REQUIRE(!boost::contains(ninja_output, "Building CXX object CMakeFiles/MyExample.dir/simple_example.cpp.o"));

  }

}