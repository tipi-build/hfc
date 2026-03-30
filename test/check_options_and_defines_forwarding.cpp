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
#include <test_isolation_fixture.hpp>

#include <pre/file/string.hpp>

#include <string>
#include <vector>

 
namespace hfc::test { 
  namespace fs = boost::filesystem;
  namespace bp = boost::process;

  using namespace std::literals;

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_options_and_defines_forwarding_cmake, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_options_and_defines_forwarding", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    write_simple_main(test_project_path, {"MathFunctions.h", "MathFunctionscbrt.h"});

    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);


    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path, test_env);


    std::string cmake_build_command = get_cmake_build_command(test_project_path, data);
    run_command(cmake_build_command, test_project_path, test_env);

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

    std::string build_ninja_content = pre::file::to_string((test_project_path / "build" / "_deps" / "project-cmake-simple-build" / "build.ninja").generic_string());

    // Check for individual compile definitions (order may vary)
    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-DTIPI_TEAM=1"),
      "Compile definition TIPI_TEAM=1 not found in CMake build.ninja"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-DTIPI_TEAM_ZURICH=0"),
      "Compile definition TIPI_TEAM_ZURICH=0 not found in CMake build.ninja"
    );

    // Check that parent scope definitions are forwarded
    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-DPARENT_SCOPE_DEF=1"),
      "Parent scope compile definition PARENT_SCOPE_DEF=1 not found in CMake build.ninja"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-DPARENT_SCOPE_STRING"),
      "Parent scope compile definition PARENT_SCOPE_STRING not found in CMake build.ninja"
    );


    std::string content = pre::file::to_string( (test_project_path / "CMakeLists.txt").generic_string());
    boost::algorithm::replace_all(content,"#","");
    pre::file::from_string((test_project_path / "CMakeLists.txt").generic_string(), content);
    run_command(cmake_build_command, test_project_path, test_env);

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "project-cmake-simple-install" / "lib" / "libMathFunctionscbrt.a"));

    for (auto cmake_option : cmake_options) {
      BOOST_REQUIRE(boost::contains( pre::file::to_string((test_project_path / "build" / "_deps" / "project-cmake-simple-build" / "CMakeCache.txt").generic_string()), cmake_option));
    }
    BOOST_REQUIRE(boost::contains( pre::file::to_string((test_project_path / "build" / "_deps" / "project-cmake-simple-build" / "CMakeCache.txt").generic_string()), cmake_option_added_after_reconfigure));

    build_ninja_content = pre::file::to_string((test_project_path / "build" / "_deps" / "project-cmake-simple-build" / "build.ninja").generic_string());

    // Check for individual compile definitions after reconfigure (order may vary)
    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-DTIPI_TEAM=1"),
      "Compile definition TIPI_TEAM=1 not found in CMake build.ninja after reconfigure"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-DTIPI_TEAM_LOCATION=ZURICH"),
      "Compile definition TIPI_TEAM_LOCATION=ZURICH not found in CMake build.ninja after reconfigure"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-DTIPI_TEAM_ZURICH=0"),
      "Compile definition TIPI_TEAM_ZURICH=0 not found in CMake build.ninja after reconfigure"
    );

    // Verify parent scope definitions are still present after reconfigure
    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-DPARENT_SCOPE_DEF=1"),
      "Parent scope compile definition PARENT_SCOPE_DEF=1 not found in CMake build.ninja after reconfigure"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-DPARENT_SCOPE_STRING"),
      "Parent scope compile definition PARENT_SCOPE_STRING not found in CMake build.ninja after reconfigure"
    );
  }



  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_options_and_defines_forwarding_autotools, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_options_and_defines_forwarding_autotools", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    write_simple_main(test_project_path,{"lib.h"});
    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);

    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path, test_env);

    std::string cmake_build_command = get_cmake_build_command(test_project_path, data);
    run_command(cmake_build_command, test_project_path, test_env);
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "Iconv-install" / "lib" / "libiconv.a"  ));


    std::string makefile_content = pre::file::to_string((test_project_path / "build" / "_deps" / "iconv-build" / "src" / "Makefile").generic_string());

    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DTIPI_TEAM=1"),
      "Compile definition TIPI_TEAM=1 not found in autotools Makefile"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DTIPI_TEAM_ZURICH"),
      "Compile definition TIPI_TEAM_ZURICH not found in autotools Makefile"
    );

    // Check that parent scope definitions are forwarded
    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DPARENT_SCOPE_DEF=1"),
      "Parent scope compile definition PARENT_SCOPE_DEF=1 not found in autotools Makefile"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DPARENT_SCOPE_STRING"),
      "Parent scope compile definition PARENT_SCOPE_STRING not found in autotools Makefile"
    );


    std::string content = pre::file::to_string( (test_project_path / "CMakeLists.txt").generic_string());
    boost::algorithm::replace_all(content,"#","");
    pre::file::from_string((test_project_path / "CMakeLists.txt").generic_string(), content);

    run_command(cmake_build_command, test_project_path, test_env);
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "Iconv-install" / "lib" / "libiconv.a"  ));


    makefile_content = pre::file::to_string((test_project_path / "build" / "_deps" / "iconv-build" / "src" / "Makefile").generic_string());

    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DTIPI_TEAM=1"),
      "Compile definition TIPI_TEAM=1 not found in autotools Makefile after reconfigure"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DTIPI_TEAM_ZURICH"),
      "Compile definition TIPI_TEAM_ZURICH not found in autotools Makefile after reconfigure"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DTIPI_TEAM_LOCATION=ZURICH"),
      "Compile definition TIPI_TEAM_LOCATION=ZURICH not found in autotools Makefile after reconfigure"
    );

    // Verify parent scope definitions are still present after reconfigure
    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DPARENT_SCOPE_DEF=1"),
      "Parent scope compile definition PARENT_SCOPE_DEF=1 not found in autotools Makefile after reconfigure"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DPARENT_SCOPE_STRING"),
      "Parent scope compile definition PARENT_SCOPE_STRING not found in autotools Makefile after reconfigure"
    );


    std::vector<boost::regex> expected_compile_flags{
      boost::regex{"CFLAGS = .* -fPIC"}, // Check for content of CMAKE_C_COMPILE_OPTIONS_PIC when CMAKE_POSITION_INDEPENDENT_CODE is ON.
      boost::regex{"LDFLAGS = .* -ldl"},
      boost::regex{"CFLAGS = .* -Wextra"},  // Check for add_compile_options from main CMakeLists.txt
      boost::regex{"CFLAGS = .* -O2"},  // Check for add_compile_options from HERMETIC_TOOLCHAIN_EXTENSION
      boost::regex{"LDFLAGS = .* -lpthread"},  // Check for add_link_options from main CMakeLists.txt
      boost::regex{"LDFLAGS = .* -rdynamic"}  // Check for add_link_options from HERMETIC_TOOLCHAIN_EXTENSION
    };
    for (auto expected_compile_flag : expected_compile_flags) {
      BOOST_CHECK_MESSAGE(boost::regex_search( pre::file::to_string((test_project_path / "build" / "_deps" / "iconv-build" / "src" / "Makefile").generic_string()), expected_compile_flag),
      "Failed to match expected flag: " + expected_compile_flag.str());
    }
  }

}