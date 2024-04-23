#define BOOST_TEST_MODULE check_options_and_defines_forwarding
#include <boost/test/included/unit_test.hpp> 

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>

#include <test_project.hpp>
#include <test_variant.hpp>
#include <test_helpers.hpp>

#include <pre/file/string.hpp>

#include <string>
#include <vector>

 
namespace hfc::test { 
  namespace fs = boost::filesystem;
  namespace bp = boost::process;

  using namespace std::literals;

  BOOST_DATA_TEST_CASE(check_options_and_defines_forwarding_cmake, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_options_and_defines_forwarding", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    write_simple_main(test_project_path, {"MathFunctions.h", "MathFunctionscbrt.h"});

    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);


    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path);


    std::string cmake_build_command = get_cmake_build_command(test_project_path, data);
    run_command(cmake_build_command, test_project_path);

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-install" / "lib" / "libMathFunctionscbrt.a"));

    std::vector<std::string> cmake_options {
      "SOME_BUILD_OPTION:BOOL=ON"s,
      "SOME_OTHER_BUILD_OPTION:BOOL=OFF"s
    };

    std::string cmake_option_added_after_reconfigure = "SOME_BUILD_PARAMETER:STRING=value";

    for (auto cmake_option : cmake_options) {
      BOOST_REQUIRE(boost::contains( pre::file::to_string((test_project_path / "build" / "_deps" / "project-cmake-simple-build" / "CMakeCache.txt").generic_string()), cmake_option));
    }
    BOOST_REQUIRE(!boost::contains( pre::file::to_string((test_project_path / "build" / "_deps" / "project-cmake-simple-build" / "CMakeCache.txt").generic_string()), cmake_option_added_after_reconfigure));

    BOOST_REQUIRE(boost::contains( pre::file::to_string((test_project_path / "build" / "_deps" / "project-cmake-simple-build" / "build.ninja").generic_string()), "DEFINES = -DTIPI_TEAM=1 -DTIPI_TEAM_ZURICH=0"));


    std::string content = pre::file::to_string( (test_project_path / "CMakeLists.txt").generic_string());
    boost::algorithm::replace_all(content,"#","");
    pre::file::from_string((test_project_path / "CMakeLists.txt").generic_string(), content);
    run_command(cmake_build_command, test_project_path);

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-install" / "lib" / "libMathFunctionscbrt.a"));

    for (auto cmake_option : cmake_options) {
      BOOST_REQUIRE(boost::contains( pre::file::to_string((test_project_path / "build" / "_deps" / "project-cmake-simple-build" / "CMakeCache.txt").generic_string()), cmake_option));
    }
    BOOST_REQUIRE(boost::contains( pre::file::to_string((test_project_path / "build" / "_deps" / "project-cmake-simple-build" / "CMakeCache.txt").generic_string()), cmake_option_added_after_reconfigure));

    BOOST_REQUIRE(boost::contains( pre::file::to_string((test_project_path / "build" / "_deps" / "project-cmake-simple-build" / "build.ninja").generic_string()), "DEFINES = -DTIPI_TEAM=1 -DTIPI_TEAM_LOCATION=ZURICH -DTIPI_TEAM_ZURICH=0"));
  }



  BOOST_DATA_TEST_CASE(check_options_and_defines_forwarding_autotools, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_options_and_defines_forwarding_autotools", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    write_simple_main(test_project_path,{"lib.h"});
    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);

    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path);

    std::string cmake_build_command = get_cmake_build_command(test_project_path, data);
    run_command(cmake_build_command, test_project_path);
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "Iconv-install" / "lib" / "libiconv.a"  ));


    std::string to_search_in_makefile = "-DTIPI_TEAM=1 -DTIPI_TEAM_ZURICH";
    BOOST_REQUIRE(boost::contains( pre::file::to_string((test_project_path / "build" / "_deps" / "iconv-build" / "src" / "Makefile").generic_string()), to_search_in_makefile));


    std::string content = pre::file::to_string( (test_project_path / "CMakeLists.txt").generic_string());
    boost::algorithm::replace_all(content,"#","");
    pre::file::from_string((test_project_path / "CMakeLists.txt").generic_string(), content);

    run_command(cmake_build_command, test_project_path);
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "Iconv-install" / "lib" / "libiconv.a"  ));


    to_search_in_makefile = "-DTIPI_TEAM=1 -DTIPI_TEAM_ZURICH -DTIPI_TEAM_LOCATION=ZURICH";
    BOOST_REQUIRE(boost::contains( pre::file::to_string((test_project_path / "build" / "_deps" / "iconv-build" / "src" / "Makefile").generic_string()), to_search_in_makefile));


    std::vector<boost::regex> expected_compile_flags{
      boost::regex{"CFLAGS = .* -fPIC"}, // Check for content of CMAKE_C_COMPILE_OPTIONS_PIC when CMAKE_POSITION_INDEPENDENT_CODE is ON.
      boost::regex{"LDFLAGS = .* -ldl"}
    };
    for (auto expected_compile_flag : expected_compile_flags) {
      BOOST_REQUIRE(boost::regex_search( pre::file::to_string((test_project_path / "build" / "_deps" / "iconv-build" / "src" / "Makefile").generic_string()), expected_compile_flag));
    }
  }

}