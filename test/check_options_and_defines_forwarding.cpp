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

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_options_and_defines_forwarding_cmake, boost::unit_test::data::make(hfc::test::test_variants_gcc_and_clang()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_options_and_defines_forwarding", data.is_cmake_re, temp_dir);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path, data.toolchain_name);

    write_simple_main(test_project_path, {"MathFunctions.h", "MathFunctionscbrt.h"});

    append_to_toolchain(project_toolchain, R"(
add_compile_options(-Wno-deprecated)
add_compile_options("SHELL:-ffunction-sections")
add_compile_definitions(TOOLCHAIN_DEFINE=1 TOOLCHAIN_STRING_DEFINE)
add_link_options(-Wl,--as-needed)
add_link_options("LINKER:-z,defs")
add_link_options("LINKER:SHELL:-z relro")
add_link_options("LINKER:SHELL:--warn-common --fatal-warnings")
)");

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

    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-Wno-deprecated"),
      "Toolchain compile option -Wno-deprecated not found in CMake build.ninja (but should be forwarded from toolchain)"
    );

    // SHELL: prefix should be stripped
    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-ffunction-sections"),
      "Toolchain compile option -ffunction-sections (from SHELL: prefix) not found in CMake build.ninja (but should be forwarded from toolchain)"
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(build_ninja_content, "SHELL:-ffunction-sections"),
      "Toolchain compile option contains untranslated SHELL: prefix in CMake build.ninja (should be translated to -ffunction-sections)"
    );

    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-DTOOLCHAIN_DEFINE=1"),
      "Toolchain compile definition TOOLCHAIN_DEFINE=1 not found in CMake build.ninja (but should be forwarded from toolchain)"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-DTOOLCHAIN_STRING_DEFINE"),
      "Toolchain compile definition TOOLCHAIN_STRING_DEFINE not found in CMake build.ninja (but should be forwarded from toolchain)"
    );
    // Static libraries are archived, not linked, so add_link_options() flags
    // don't land here — we only check they're not leaking untranslated.
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(build_ninja_content, "LINKER:-z,defs"),
      "Toolchain link option contains untranslated LINKER: prefix in CMake build.ninja (should be translated to -Wl,-z,defs)"
    );

    BOOST_REQUIRE_MESSAGE(
      !boost::contains(build_ninja_content, "LINKER:SHELL:-z relro"),
      "Toolchain link option contains untranslated LINKER:SHELL: prefix in CMake build.ninja (should be translated)"
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(build_ninja_content, "LINKER:SHELL:--warn-common"),
      "Toolchain link option contains untranslated LINKER:SHELL: prefix in CMake build.ninja (should be translated)"
    );

    // Link options appear on the executable, not on static libraries.
    std::string parent_build_ninja_content = pre::file::to_string((test_project_path / "build" / "build.ninja").generic_string());

    bool is_clang = boost::contains(data.toolchain_name, "clang");

    if (is_clang) {
      BOOST_REQUIRE_MESSAGE(
        boost::contains(parent_build_ninja_content, "-Xlinker -z") &&
        boost::contains(parent_build_ninja_content, "-Xlinker relro"),
        "Toolchain link option from 'LINKER:SHELL:-z relro' not properly translated in executable link command for Clang (should be -Xlinker -z -Xlinker relro). Toolchain: " + data.toolchain_name
      );
    } else {
      BOOST_REQUIRE_MESSAGE(
        boost::contains(parent_build_ninja_content, "-Wl,-z,relro"),
        "Toolchain link option from 'LINKER:SHELL:-z relro' not properly translated in executable link command for GCC (should be -Wl,-z,relro with comma-separated arguments). Toolchain: " + data.toolchain_name
      );
    }

    if (is_clang) {
      BOOST_REQUIRE_MESSAGE(
        boost::contains(parent_build_ninja_content, "-Xlinker --warn-common") &&
        boost::contains(parent_build_ninja_content, "-Xlinker --fatal-warnings"),
        "Toolchain link option from 'LINKER:SHELL:--warn-common --fatal-warnings' not properly translated in executable link command for Clang (should be -Xlinker --warn-common -Xlinker --fatal-warnings). Toolchain: " + data.toolchain_name
      );
    } else {
      BOOST_REQUIRE_MESSAGE(
        boost::contains(parent_build_ninja_content, "-Wl,--warn-common,--fatal-warnings"),
        "Toolchain link option from 'LINKER:SHELL:--warn-common --fatal-warnings' not properly translated in executable link command for GCC (should be -Wl,--warn-common,--fatal-warnings with comma-separated arguments). Toolchain: " + data.toolchain_name
      );
    }

    // HERMETIC_TOOLCHAIN_EXTENSION definitions (order is not guaranteed).
    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-DTIPI_TEAM=1"),
      "Compile definition TIPI_TEAM=1 not found in CMake build.ninja"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(build_ninja_content, "-DTIPI_TEAM_ZURICH=0"),
      "Compile definition TIPI_TEAM_ZURICH=0 not found in CMake build.ninja"
    );

    // Parent-scope definitions must not leak in.
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(build_ninja_content, "-DPARENT_SCOPE_DEF=1"),
      "Parent scope compile definition PARENT_SCOPE_DEF=1 found in CMake build.ninja (but should not be forwarded)"
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(build_ninja_content, "-DPARENT_SCOPE_STRING"),
      "Parent scope compile definition PARENT_SCOPE_STRING found in CMake build.ninja (but should not be forwarded)"
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

    BOOST_REQUIRE_MESSAGE(
      !boost::contains(build_ninja_content, "-DPARENT_SCOPE_DEF=1"),
      "Parent scope compile definition PARENT_SCOPE_DEF=1 found in CMake build.ninja after reconfigure (but should not be forwarded)"
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(build_ninja_content, "-DPARENT_SCOPE_STRING"),
      "Parent scope compile definition PARENT_SCOPE_STRING found in CMake build.ninja after reconfigure (but should not be forwarded)"
    );
  }



  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_options_and_defines_forwarding_autotools, boost::unit_test::data::make(hfc::test::test_variants_gcc_and_clang()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_options_and_defines_forwarding_autotools", data.is_cmake_re, temp_dir);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path, data.toolchain_name);

    write_simple_main(test_project_path,{"lib.h"});

    append_to_toolchain(project_toolchain, R"(
add_compile_options(-Wno-unused-parameter)
add_compile_options("SHELL:-fdata-sections")
add_compile_definitions(TOOLCHAIN_AUTOTOOLS_DEF=1 TOOLCHAIN_AUTOTOOLS_STRING)
add_link_options(-Wl,--no-undefined)
add_link_options("LINKER:--as-needed")
add_link_options("LINKER:SHELL:-z noexecstack")
add_link_options("LINKER:SHELL:--no-as-needed --warn-unresolved-symbols")

# Genex flags — the flags-resolver subproject must evaluate these before
# ./configure sees them.
add_compile_options("$<$<CONFIG:Release>:-DHFC_GENEX_AUTOTOOLS_RELEASE=1>")
add_compile_options("$<$<CONFIG:Debug>:-DHFC_GENEX_AUTOTOOLS_DEBUG_ONLY=1>")
add_compile_definitions("$<$<CONFIG:Release>:HFC_GENEX_AUTOTOOLS_DEF_RELEASE>")
add_link_options("$<$<PLATFORM_ID:Linux>:-Wl,--gc-sections>")
add_link_options("$<$<CONFIG:Release>:LINKER:--hash-style=gnu>")

# COMPILE_LANGUAGE genexes: C-only flags must land in CFLAGS only,
# CXX-only flags in CXXFLAGS only.
add_compile_options("$<$<COMPILE_LANGUAGE:C>:-DHFC_GENEX_AUTOTOOLS_C_ONLY=1>")
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-DHFC_GENEX_AUTOTOOLS_CXX_ONLY=1>")
add_compile_definitions("$<$<COMPILE_LANGUAGE:C>:HFC_GENEX_AUTOTOOLS_DEF_C_ONLY>")
add_compile_definitions("$<$<COMPILE_LANGUAGE:CXX>:HFC_GENEX_AUTOTOOLS_DEF_CXX_ONLY>")

# LINK_LANGUAGE on add_link_options: HERMETIC_CONFIG_LANGUAGE picks which
# language's view feeds LDFLAGS. Iconv defaults to C, so only the C branch lands.
add_link_options("$<$<LINK_LANGUAGE:C>:-Wl,--defsym=HFC_LINKLANG_C=0>")
add_link_options("$<$<LINK_LANGUAGE:CXX>:-Wl,--defsym=HFC_LINKLANG_CXX=0>")

# include_directories -> CPPFLAGS -I, link_directories -> LDFLAGS -L.
include_directories("/opt/hfc-test/include-always")
include_directories("$<$<CONFIG:Release>:/opt/hfc-test/include-release>")
include_directories("$<$<COMPILE_LANGUAGE:C>:/opt/hfc-test/include-c-only>")
include_directories("$<$<COMPILE_LANGUAGE:CXX>:/opt/hfc-test/include-cxx-only>")
link_directories("/opt/hfc-test/lib-always")
link_directories("$<$<CONFIG:Release>:/opt/hfc-test/lib-release>")
link_directories("$<$<COMPILE_LANGUAGE:C>:/opt/hfc-test/lib-c-only>")
link_directories("$<$<COMPILE_LANGUAGE:CXX>:/opt/hfc-test/lib-cxx-only>")
)");

    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);

    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path, test_env);

    std::string cmake_build_command = get_cmake_build_command(test_project_path, data);
    run_command(cmake_build_command, test_project_path, test_env);
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "Iconv-install" / "lib" / "libiconv.a"  ));


    std::string makefile_content = pre::file::to_string((test_project_path / "build" / "_deps" / "iconv-build" / "src" / "Makefile").generic_string());

    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-Wno-unused-parameter"),
      "Toolchain compile option -Wno-unused-parameter not found in autotools Makefile (but should be forwarded from toolchain)"
    );

    // SHELL: prefix should be stripped
    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-fdata-sections"),
      "Toolchain compile option -fdata-sections (from SHELL: prefix) not found in autotools Makefile (but should be forwarded from toolchain)"
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(makefile_content, "SHELL:-fdata-sections"),
      "Toolchain compile option contains untranslated SHELL: prefix in autotools Makefile (should be translated to -fdata-sections)"
    );

    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DTOOLCHAIN_AUTOTOOLS_DEF=1"),
      "Toolchain compile definition TOOLCHAIN_AUTOTOOLS_DEF=1 not found in autotools Makefile (but should be forwarded from toolchain)"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DTOOLCHAIN_AUTOTOOLS_STRING"),
      "Toolchain compile definition TOOLCHAIN_AUTOTOOLS_STRING not found in autotools Makefile (but should be forwarded from toolchain)"
    );

    bool is_clang = boost::contains(data.toolchain_name, "clang");

    // Literal -Wl, flags pass through verbatim; only the LINKER: pseudo-syntax
    // gets rewritten.
    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-Wl,--no-undefined"),
      "Toolchain link option -Wl,--no-undefined not found in autotools Makefile (but should be forwarded from toolchain). Toolchain: " + data.toolchain_name
    );

    if (is_clang) {
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-Xlinker --as-needed"),
        "Toolchain link option -Xlinker --as-needed (from LINKER: prefix) not found in autotools Makefile for Clang (but should be forwarded from toolchain). Toolchain: " + data.toolchain_name
      );
    } else {
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-Wl,--as-needed"),
        "Toolchain link option -Wl,--as-needed (from LINKER: prefix) not found in autotools Makefile for GCC (but should be forwarded from toolchain). Toolchain: " + data.toolchain_name
      );
    }
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(makefile_content, "LINKER:--as-needed"),
      "Toolchain link option contains untranslated LINKER: prefix in autotools Makefile (should be translated)"
    );

    if (is_clang) {
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-Xlinker -z") &&
        boost::contains(makefile_content, "-Xlinker noexecstack"),
        "Toolchain link option from 'LINKER:SHELL:-z noexecstack' not properly translated in autotools Makefile for Clang (should be -Xlinker -z -Xlinker noexecstack). Toolchain: " + data.toolchain_name
      );
    } else {
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-Wl,-z,noexecstack"),
        "Toolchain link option from 'LINKER:SHELL:-z noexecstack' not properly translated in autotools Makefile for GCC (should be -Wl,-z,noexecstack with comma-separated arguments). Toolchain: " + data.toolchain_name
      );
    }
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(makefile_content, "LINKER:SHELL:-z noexecstack"),
      "Toolchain link option contains untranslated LINKER:SHELL: prefix in autotools Makefile (should be translated)"
    );

    if (is_clang) {
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-Xlinker --no-as-needed") &&
        boost::contains(makefile_content, "-Xlinker --warn-unresolved-symbols"),
        "Toolchain link option from 'LINKER:SHELL:--no-as-needed --warn-unresolved-symbols' not properly translated in autotools Makefile for Clang (should be -Xlinker --no-as-needed -Xlinker --warn-unresolved-symbols). Toolchain: " + data.toolchain_name
      );
    } else {
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-Wl,--no-as-needed,--warn-unresolved-symbols"),
        "Toolchain link option from 'LINKER:SHELL:--no-as-needed --warn-unresolved-symbols' not properly translated in autotools Makefile for GCC (should be -Wl,--no-as-needed,--warn-unresolved-symbols with comma-separated arguments). Toolchain: " + data.toolchain_name
      );
    }
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(makefile_content, "LINKER:SHELL:--no-as-needed"),
      "Toolchain link option contains untranslated LINKER:SHELL: prefix in autotools Makefile (should be translated)"
    );

    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DTIPI_TEAM=1"),
      "Compile definition TIPI_TEAM=1 not found in autotools Makefile"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DTIPI_TEAM_ZURICH"),
      "Compile definition TIPI_TEAM_ZURICH not found in autotools Makefile"
    );

    BOOST_REQUIRE_MESSAGE(
      !boost::contains(makefile_content, "-DPARENT_SCOPE_DEF=1"),
      "Parent scope compile definition PARENT_SCOPE_DEF=1 found in autotools Makefile (but should not be forwarded)"
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(makefile_content, "-DPARENT_SCOPE_STRING"),
      "Parent scope compile definition PARENT_SCOPE_STRING found in autotools Makefile (but should not be forwarded)"
    );

    // Genexes must be fully resolved before reaching ./configure: Release
    // branches appear, Debug branch does not, no raw $< leaks through.
    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DHFC_GENEX_AUTOTOOLS_RELEASE=1"),
      "Toolchain genex option $<$<CONFIG:Release>:-DHFC_GENEX_AUTOTOOLS_RELEASE=1> was not resolved/forwarded into autotools Makefile"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-DHFC_GENEX_AUTOTOOLS_DEF_RELEASE"),
      "Toolchain genex compile_definitions $<$<CONFIG:Release>:HFC_GENEX_AUTOTOOLS_DEF_RELEASE> was not resolved/forwarded into autotools Makefile"
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(makefile_content, "-DHFC_GENEX_AUTOTOOLS_DEBUG_ONLY=1"),
      "Debug-branch genex option leaked into the Release autotools Makefile (config branch not evaluated correctly)"
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(makefile_content, "-Wl,--gc-sections"),
      "Toolchain genex link option $<$<PLATFORM_ID:Linux>:-Wl,--gc-sections> was not resolved/forwarded into autotools Makefile"
    );
    if (is_clang) {
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-Xlinker --hash-style=gnu"),
        "Toolchain genex producing LINKER:--hash-style=gnu was not translated for Clang in autotools Makefile. Toolchain: " + data.toolchain_name
      );
    } else {
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-Wl,--hash-style=gnu"),
        "Toolchain genex producing LINKER:--hash-style=gnu was not translated for GCC in autotools Makefile. Toolchain: " + data.toolchain_name
      );
    }
    {
      boost::smatch mm;
      std::string leaking_line;
      if (boost::regex_search(makefile_content, mm, boost::regex{R"([^\n]*(CFLAGS|CXXFLAGS|LDFLAGS)\s*=[^\n]*\$<[^\n]*)"})) {
        leaking_line = mm.str(0);
      }
      BOOST_REQUIRE_MESSAGE(
        leaking_line.empty(),
        "Unresolved generator expression ('$<') leaked into a flag line of the autotools Makefile. Offending line: " + leaking_line
      );
    }

    // COMPILE_LANGUAGE routing: C-only flag in CFLAGS, CXX-only flag in CXXFLAGS.
    auto extract_flag_line = [&](const std::string& var) -> std::string {
      boost::smatch mm;
      if (boost::regex_search(makefile_content, mm, boost::regex{"(?:^|\\n)" + var + R"(\s*=[^\n]*)"})) {
        return mm.str(0);
      }
      return {};
    };
    const std::string cflags_line   = extract_flag_line("CFLAGS");
    const std::string cxxflags_line = extract_flag_line("CXXFLAGS");
    BOOST_REQUIRE_MESSAGE(!cflags_line.empty(), "Could not locate a CFLAGS = line in autotools Makefile");

    BOOST_REQUIRE_MESSAGE(
      boost::contains(cflags_line, "-DHFC_GENEX_AUTOTOOLS_C_ONLY=1"),
      "$<COMPILE_LANGUAGE:C> flag missing from CFLAGS. CFLAGS line: " + cflags_line
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(cflags_line, "-DHFC_GENEX_AUTOTOOLS_CXX_ONLY=1"),
      "$<COMPILE_LANGUAGE:CXX> flag leaked into CFLAGS. CFLAGS line: " + cflags_line
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(cflags_line, "-DHFC_GENEX_AUTOTOOLS_DEF_C_ONLY"),
      "$<COMPILE_LANGUAGE:C> compile-definition missing from CFLAGS. CFLAGS line: " + cflags_line
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(cflags_line, "-DHFC_GENEX_AUTOTOOLS_DEF_CXX_ONLY"),
      "$<COMPILE_LANGUAGE:CXX> compile-definition leaked into CFLAGS. CFLAGS line: " + cflags_line
    );
    // Iconv is C-only, so CXXFLAGS may not exist. Check it only if present.
    if (!cxxflags_line.empty()) {
      BOOST_REQUIRE_MESSAGE(
        boost::contains(cxxflags_line, "-DHFC_GENEX_AUTOTOOLS_CXX_ONLY=1"),
        "$<COMPILE_LANGUAGE:CXX> flag missing from CXXFLAGS. CXXFLAGS line: " + cxxflags_line
      );
      BOOST_REQUIRE_MESSAGE(
        !boost::contains(cxxflags_line, "-DHFC_GENEX_AUTOTOOLS_C_ONLY=1"),
        "$<COMPILE_LANGUAGE:C> flag leaked into CXXFLAGS. CXXFLAGS line: " + cxxflags_line
      );
      BOOST_REQUIRE_MESSAGE(
        boost::contains(cxxflags_line, "-DHFC_GENEX_AUTOTOOLS_DEF_CXX_ONLY"),
        "$<COMPILE_LANGUAGE:CXX> compile-definition missing from CXXFLAGS. CXXFLAGS line: " + cxxflags_line
      );
      BOOST_REQUIRE_MESSAGE(
        !boost::contains(cxxflags_line, "-DHFC_GENEX_AUTOTOOLS_DEF_C_ONLY"),
        "$<COMPILE_LANGUAGE:C> compile-definition leaked into CXXFLAGS. CXXFLAGS line: " + cxxflags_line
      );
    }

    // Iconv defaults to HERMETIC_CONFIG_LANGUAGE=C, so LDFLAGS gets the C view.
    const std::string ldflags_line = extract_flag_line("LDFLAGS");
    BOOST_REQUIRE_MESSAGE(!ldflags_line.empty(), "Could not locate an LDFLAGS = line in autotools Makefile");
    BOOST_REQUIRE_MESSAGE(
      boost::contains(ldflags_line, "HFC_LINKLANG_C=0"),
      "$<LINK_LANGUAGE:C> link option missing from LDFLAGS (HERMETIC_CONFIG_LANGUAGE default=C). LDFLAGS line: " + ldflags_line
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(ldflags_line, "HFC_LINKLANG_CXX=0"),
      "$<LINK_LANGUAGE:CXX> link option leaked into LDFLAGS despite default HERMETIC_CONFIG_LANGUAGE=C. LDFLAGS line: " + ldflags_line
    );

    // Under default HERMETIC_CONFIG_LANGUAGE=C: unconditional, Release, and
    // C-only paths appear; CXX-only paths are filtered out.
    const std::string cppflags_line_c = extract_flag_line("CPPFLAGS");
    BOOST_REQUIRE_MESSAGE(!cppflags_line_c.empty(), "Could not locate a CPPFLAGS = line in autotools Makefile");
    BOOST_REQUIRE_MESSAGE(
      boost::contains(cppflags_line_c, "/opt/hfc-test/include-always"),
      "Unconditional include_directories missing from CPPFLAGS. CPPFLAGS line: " + cppflags_line_c
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(cppflags_line_c, "/opt/hfc-test/include-release"),
      "$<CONFIG:Release> include_directories missing from CPPFLAGS. CPPFLAGS line: " + cppflags_line_c
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(cppflags_line_c, "/opt/hfc-test/include-c-only"),
      "$<COMPILE_LANGUAGE:C> include_directories missing from CPPFLAGS under default HERMETIC_CONFIG_LANGUAGE=C. CPPFLAGS line: " + cppflags_line_c
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(cppflags_line_c, "/opt/hfc-test/include-cxx-only"),
      "$<COMPILE_LANGUAGE:CXX> include_directories leaked into CPPFLAGS under default HERMETIC_CONFIG_LANGUAGE=C. CPPFLAGS line: " + cppflags_line_c
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(ldflags_line, "-L/opt/hfc-test/lib-always"),
      "Unconditional link_directories missing from LDFLAGS. LDFLAGS line: " + ldflags_line
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(ldflags_line, "-L/opt/hfc-test/lib-release"),
      "$<CONFIG:Release> link_directories missing from LDFLAGS. LDFLAGS line: " + ldflags_line
    );
    BOOST_REQUIRE_MESSAGE(
      boost::contains(ldflags_line, "-L/opt/hfc-test/lib-c-only"),
      "$<COMPILE_LANGUAGE:C> link_directories missing from LDFLAGS under default HERMETIC_CONFIG_LANGUAGE=C. LDFLAGS line: " + ldflags_line
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(ldflags_line, "-L/opt/hfc-test/lib-cxx-only"),
      "$<COMPILE_LANGUAGE:CXX> link_directories leaked into LDFLAGS under default HERMETIC_CONFIG_LANGUAGE=C. LDFLAGS line: " + ldflags_line
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

    BOOST_REQUIRE_MESSAGE(
      !boost::contains(makefile_content, "-DPARENT_SCOPE_DEF=1"),
      "Parent scope compile definition PARENT_SCOPE_DEF=1 found in autotools Makefile after reconfigure (but should not be forwarded)"
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(makefile_content, "-DPARENT_SCOPE_STRING"),
      "Parent scope compile definition PARENT_SCOPE_STRING found in autotools Makefile after reconfigure (but should not be forwarded)"
    );


    std::vector<boost::regex> expected_compile_flags{
      boost::regex{"CFLAGS = .* -fPIC"}, // CMAKE_C_COMPILE_OPTIONS_PIC when CMAKE_POSITION_INDEPENDENT_CODE is ON
      boost::regex{"LDFLAGS = .* -ldl"},
      boost::regex{"CFLAGS = .* -O2"},  // from HERMETIC_TOOLCHAIN_EXTENSION add_compile_options
      boost::regex{"LDFLAGS = .* -rdynamic"}  // from HERMETIC_TOOLCHAIN_EXTENSION add_link_options
    };

    std::string makefile_for_flag_check = pre::file::to_string((test_project_path / "build" / "_deps" / "iconv-build" / "src" / "Makefile").generic_string());
    BOOST_CHECK_MESSAGE(
      !boost::contains(makefile_for_flag_check, "-Wextra"),
      "Parent scope compile option -Wextra found in autotools Makefile (but should not be forwarded)"
    );
    BOOST_CHECK_MESSAGE(
      !boost::contains(makefile_for_flag_check, "-lpthread"),
      "Parent scope link option -lpthread found in autotools Makefile (but should not be forwarded)"
    );
    for (auto expected_compile_flag : expected_compile_flags) {
      BOOST_CHECK_MESSAGE(boost::regex_search( pre::file::to_string((test_project_path / "build" / "_deps" / "iconv-build" / "src" / "Makefile").generic_string()), expected_compile_flag),
      "Failed to match expected flag: " + expected_compile_flag.str());
    }
  }

  // Same genexes and dependency as the default-C test, but with
  // HERMETIC_CONFIG_LANGUAGE CXX: CPPFLAGS and LDFLAGS must carry the CXX view.
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_options_and_defines_forwarding_autotools_language_cxx, boost::unit_test::data::make(hfc::test::test_variants()), data) {
    fs::path test_project_path = prepare_project_to_be_tested("check_options_and_defines_forwarding_autotools", data.is_cmake_re, temp_dir);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path, data.toolchain_name);

    write_simple_main(test_project_path, {"lib.h"});

    append_to_toolchain(project_toolchain, R"(
add_compile_definitions("$<$<COMPILE_LANGUAGE:C>:HFC_LANG_TEST_DEF_C>")
add_compile_definitions("$<$<COMPILE_LANGUAGE:CXX>:HFC_LANG_TEST_DEF_CXX>")
add_link_options("$<$<LINK_LANGUAGE:C>:-Wl,--defsym=HFC_LINKLANG_C=0>")
add_link_options("$<$<LINK_LANGUAGE:CXX>:-Wl,--defsym=HFC_LINKLANG_CXX=0>")
)");
    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);

    fs::path project_cml = test_project_path / "CMakeLists.txt";
    std::string project_cml_content = pre::file::to_string(project_cml.generic_string());
    // Inject HERMETIC_CONFIG_LANGUAGE CXX into the FetchContent_MakeHermetic call.
    boost::replace_first(project_cml_content,
      "HERMETIC_BUILD_SYSTEM autotools",
      "HERMETIC_BUILD_SYSTEM autotools\n  HERMETIC_CONFIG_LANGUAGE CXX");
    pre::file::from_string(project_cml.generic_string(), project_cml_content);

    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path, test_env);

    std::string cmake_build_command = get_cmake_build_command(test_project_path, data);
    run_command(cmake_build_command, test_project_path, test_env);
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "Iconv-install" / "lib" / "libiconv.a"));

    std::string makefile_content = pre::file::to_string((test_project_path / "build" / "_deps" / "iconv-build" / "src" / "Makefile").generic_string());

    auto extract_flag_line = [&](const std::string& var) -> std::string {
      boost::smatch mm;
      if (boost::regex_search(makefile_content, mm, boost::regex{"(?:^|\\n)" + var + R"(\s*=[^\n]*)"})) {
        return mm.str(0);
      }
      return {};
    };
    // CPPFLAGS carries the CXX-resolved compile_definitions.
    const std::string cppflags_line = extract_flag_line("CPPFLAGS");
    BOOST_REQUIRE_MESSAGE(!cppflags_line.empty(), "Could not locate a CPPFLAGS = line in autotools Makefile");
    BOOST_REQUIRE_MESSAGE(
      boost::contains(cppflags_line, "-DHFC_LANG_TEST_DEF_CXX"),
      "CXX-branch compile-definition missing from CPPFLAGS under HERMETIC_CONFIG_LANGUAGE=CXX. CPPFLAGS line: " + cppflags_line
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(cppflags_line, "-DHFC_LANG_TEST_DEF_C "),
      "C-branch compile-definition leaked into CPPFLAGS under HERMETIC_CONFIG_LANGUAGE=CXX. CPPFLAGS line: " + cppflags_line
    );

    const std::string ldflags_line = extract_flag_line("LDFLAGS");
    BOOST_REQUIRE_MESSAGE(!ldflags_line.empty(), "Could not locate an LDFLAGS = line in autotools Makefile");
    BOOST_REQUIRE_MESSAGE(
      boost::contains(ldflags_line, "HFC_LINKLANG_CXX=0"),
      "$<LINK_LANGUAGE:CXX> link option missing from LDFLAGS under HERMETIC_CONFIG_LANGUAGE=CXX. LDFLAGS line: " + ldflags_line
    );
    BOOST_REQUIRE_MESSAGE(
      !boost::contains(ldflags_line, "HFC_LINKLANG_C=0"),
      "$<LINK_LANGUAGE:C> link option leaked into LDFLAGS under HERMETIC_CONFIG_LANGUAGE=CXX. LDFLAGS line: " + ldflags_line
    );

    // HERMETIC_CONFIG_LANGUAGE is embedded as a comment in the proxy toolchain
    // so cmake-re's ABI hash flips when it changes.
    {
      fs::path iconv_proxy_toolchain = test_project_path / "build" / "_deps" / "Iconv-toolchain" / "hfc_hermetic_proxy_toolchain.cmake";
      BOOST_REQUIRE(fs::exists(iconv_proxy_toolchain));
      std::string proxy_toolchain_content = pre::file::to_string(iconv_proxy_toolchain.generic_string());
      BOOST_TEST_INFO("Proxy toolchain contents:\n" << proxy_toolchain_content);
      BOOST_REQUIRE(boost::contains(proxy_toolchain_content, "\n# CXX"));
    }
  }

}