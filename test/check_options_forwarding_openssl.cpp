#define BOOST_TEST_MODULE check_options_forwarding_openssl
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

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_options_forwarding_openssl, boost::unit_test::data::make(hfc::test::test_variants_gcc_and_clang()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_options_forwarding_openssl", data.is_cmake_re, temp_dir);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path, data.toolchain_name);

    // Add compile options, definitions, and link options to the toolchain file
    append_to_toolchain(project_toolchain, R"(
# Toolchain-level flags that should be propagated
add_compile_options(-Wno-deprecated-declarations)
add_compile_options("SHELL:-fno-omit-frame-pointer")
add_compile_definitions(TOOLCHAIN_OPENSSL_DEF=1 TOOLCHAIN_OPENSSL_STRING)
add_link_options(-Wl,--build-id)
add_link_options("LINKER:--hash-style=gnu")
add_link_options("LINKER:SHELL:-z now")
add_link_options("LINKER:SHELL:--no-allow-shlib-undefined --no-undefined")

# Generator-expression-based toolchain flags — must be resolved by the
# flags-resolver nested subproject before they reach ./Configure.
add_compile_options("$<$<CONFIG:Release>:-DHFC_GENEX_RELEASE=1>")
add_compile_options("$<$<CONFIG:Debug>:-DHFC_GENEX_DEBUG_ONLY=1>")
add_compile_definitions("$<$<CONFIG:Release>:HFC_GENEX_DEF_RELEASE>")
add_link_options("$<$<PLATFORM_ID:Linux>:-Wl,--gc-sections>")
add_link_options("$<$<CONFIG:Release>:LINKER:--as-needed>")
# $<LINK_LANGUAGE:...> on add_link_options — OpenSSL is C-only so it uses
# the default HERMETIC_CONFIG_LANGUAGE=C; expect only the C-branch flag.
add_link_options("$<$<LINK_LANGUAGE:C>:-Wl,--defsym=HFC_LINKLANG_C=0>")
add_link_options("$<$<LINK_LANGUAGE:CXX>:-Wl,--defsym=HFC_LINKLANG_CXX=0>")
)");

    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);

    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path, test_env);

    fs::path adapter_cmake_lists = test_project_path / "build" / "_deps" / "OpenSSL-adapter" / "CMakeLists.txt";
    BOOST_REQUIRE(fs::exists(adapter_cmake_lists));

    std::string adapter_content = pre::file::to_string(adapter_cmake_lists.generic_string());

    BOOST_REQUIRE_MESSAGE(
      boost::contains(adapter_content, "no-asm"),
      "HERMETIC_CONFIG_EXTRA_ARGS 'no-asm' not found in OpenSSL adapter CMakeLists.txt"
    );

    BOOST_REQUIRE_MESSAGE(
      boost::contains(adapter_content, "no-shared"),
      "HERMETIC_CONFIG_EXTRA_ARGS 'no-shared' not found in OpenSSL adapter CMakeLists.txt"
    );

    std::vector<boost::regex> expected_compile_flags{
      boost::regex{"CFLAGS = .* -Wall"},
      boost::regex{"CXXFLAGS = .* -Wall"},
      boost::regex{"LDFLAGS = .* -ldl"}
    };

    // make sure that the HERMETIC_CONFIG_EXTRA_ARGS are being forwarded to the proxy toolchain
    {
      fs::path openssl_proxy_toolchain = test_project_path / "build" / "_deps" / "OpenSSL-toolchain" / "hfc_hermetic_proxy_toolchain.cmake";
      BOOST_REQUIRE(fs::exists(openssl_proxy_toolchain));
      std::string proxy_toolchain_content = pre::file::to_string(openssl_proxy_toolchain.generic_string());
      BOOST_TEST_INFO("Proxy toolchain contents:\n" << proxy_toolchain_content);
      BOOST_REQUIRE(boost::contains(proxy_toolchain_content, "\n# no-asm no-shared"));
    }

    fs::path openssl_makefile = test_project_path / "build" / "_deps" / "OpenSSL-build" / "src" / "Makefile";
    if(fs::exists(openssl_makefile)) {
      std::string makefile_content = pre::file::to_string(openssl_makefile.generic_string());

      // Check that toolchain-level flags ARE forwarded
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-Wno-deprecated"),
        "Toolchain compile option -Wno-deprecated not found in OpenSSL Makefile (but should be forwarded from toolchain)"
      );

      // Check SHELL: prefix is properly translated
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-fno-omit-frame-pointer"),
        "Toolchain compile option -fno-omit-frame-pointer (from SHELL: prefix) not found in OpenSSL Makefile (but should be forwarded from toolchain)"
      );
      BOOST_REQUIRE_MESSAGE(
        !boost::contains(makefile_content, "SHELL:-fno-omit-frame-pointer"),
        "Toolchain compile option contains untranslated SHELL: prefix in OpenSSL Makefile (should be translated to -fno-omit-frame-pointer)"
      );

      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-DTOOLCHAIN_OPENSSL_DEF=1"),
        "Toolchain compile definition TOOLCHAIN_OPENSSL_DEF=1 not found in OpenSSL Makefile (but should be forwarded from toolchain)"
      );
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-DTOOLCHAIN_OPENSSL_STRING"),
        "Toolchain compile definition TOOLCHAIN_OPENSSL_STRING not found in OpenSSL Makefile (but should be forwarded from toolchain)"
      );
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-Wl,--build-id"),
        "Toolchain link option -Wl,--build-id not found in OpenSSL Makefile (but should be forwarded from toolchain)"
      );

      // Check LINKER: prefix is properly translated
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-Wl,--hash-style=gnu"),
        "Toolchain link option -Wl,--hash-style=gnu (from LINKER: prefix) not found in OpenSSL Makefile (but should be forwarded from toolchain)"
      );
      BOOST_REQUIRE_MESSAGE(
        !boost::contains(makefile_content, "LINKER:--hash-style=gnu"),
        "Toolchain link option contains untranslated LINKER: prefix in OpenSSL Makefile (should be translated to -Wl,--hash-style=gnu)"
      );

      // Determine if we're using Clang toolchain
      bool is_clang = boost::contains(data.toolchain_name, "clang");

      // Check LINKER:SHELL: prefix is properly translated
      if (is_clang) {
        // Clang: -Xlinker -z -Xlinker now (repeated prefix)
        BOOST_REQUIRE_MESSAGE(
          boost::contains(makefile_content, "-Xlinker -z") &&
          boost::contains(makefile_content, "-Xlinker now"),
          "Toolchain link option from 'LINKER:SHELL:-z now' not properly translated in OpenSSL Makefile for Clang (should be -Xlinker -z -Xlinker now). Toolchain: " + data.toolchain_name
        );
      } else {
        // GCC: -Wl,-z,now (comma-separated, single -Wl, prefix)
        BOOST_REQUIRE_MESSAGE(
          boost::contains(makefile_content, "-Wl,-z,now"),
          "Toolchain link option from 'LINKER:SHELL:-z now' not properly translated in OpenSSL Makefile for GCC (should be -Wl,-z,now with comma-separated arguments). Toolchain: " + data.toolchain_name
        );
      }
      BOOST_REQUIRE_MESSAGE(
        !boost::contains(makefile_content, "LINKER:SHELL:-z now"),
        "Toolchain link option contains untranslated LINKER:SHELL: prefix in OpenSSL Makefile (should be translated)"
      );

      // Check LINKER:SHELL:--no-allow-shlib-undefined --no-undefined is properly translated
      if (is_clang) {
        // Clang: -Xlinker --no-allow-shlib-undefined -Xlinker --no-undefined (repeated prefix)
        BOOST_REQUIRE_MESSAGE(
          boost::contains(makefile_content, "-Xlinker --no-allow-shlib-undefined") &&
          boost::contains(makefile_content, "-Xlinker --no-undefined"),
          "Toolchain link option from 'LINKER:SHELL:--no-allow-shlib-undefined --no-undefined' not properly translated in OpenSSL Makefile for Clang (should be -Xlinker --no-allow-shlib-undefined -Xlinker --no-undefined). Toolchain: " + data.toolchain_name
        );
      } else {
        // GCC: -Wl,--no-allow-shlib-undefined,--no-undefined (comma-separated, single -Wl, prefix)
        BOOST_REQUIRE_MESSAGE(
          boost::contains(makefile_content, "-Wl,--no-allow-shlib-undefined,--no-undefined"),
          "Toolchain link option from 'LINKER:SHELL:--no-allow-shlib-undefined --no-undefined' not properly translated in OpenSSL Makefile for GCC (should be -Wl,--no-allow-shlib-undefined,--no-undefined with comma-separated arguments). Toolchain: " + data.toolchain_name
        );
      }
      BOOST_REQUIRE_MESSAGE(
        !boost::contains(makefile_content, "LINKER:SHELL:--no-allow-shlib-undefined"),
        "Toolchain link option contains untranslated LINKER:SHELL: prefix in OpenSSL Makefile (should be translated)"
      );

      // Check that HERMETIC_TOOLCHAIN_EXTENSION flags ARE forwarded
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-DHFC_TOOLCHAIN_EXTENSION_TEST=1"),
        "HERMETIC_TOOLCHAIN_EXTENSION compile definition HFC_TOOLCHAIN_EXTENSION_TEST=1 not found in OpenSSL Makefile"
      );
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-O2"),
        "HERMETIC_TOOLCHAIN_EXTENSION compile option -O2 not found in OpenSSL Makefile"
      );
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-lpthread"),
        "HERMETIC_TOOLCHAIN_EXTENSION link option -lpthread not found in OpenSSL Makefile"
      );

      // Check that parent scope flags are NOT forwarded
      BOOST_REQUIRE_MESSAGE(
        !boost::contains(makefile_content, "-DTIPI_TEAM=1"),
        "Parent scope compile definition TIPI_TEAM=1 found in OpenSSL Makefile (but should not be forwarded)"
      );
      BOOST_REQUIRE_MESSAGE(
        !boost::contains(makefile_content, "-DTIPI_TEAM_ZURICH"),
        "Parent scope compile definition TIPI_TEAM_ZURICH found in OpenSSL Makefile (but should not be forwarded)"
      );
      // Note: We don't check for -Wall or -ldl from parent scope as they might be added by OpenSSL's own build system

      // Generator-expression resolution: the toolchain contains $<...:...>
      // options that the flags-resolver subproject should fully resolve before
      // any flag reaches ./Configure. The Release-branch options should appear,
      // the Debug-branch option should NOT, and no raw $< marker should leak.
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-DHFC_GENEX_RELEASE=1"),
        "Toolchain genex option $<$<CONFIG:Release>:-DHFC_GENEX_RELEASE=1> was not resolved/forwarded into OpenSSL Makefile"
      );
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-DHFC_GENEX_DEF_RELEASE"),
        "Toolchain genex compile_definitions $<$<CONFIG:Release>:HFC_GENEX_DEF_RELEASE> was not resolved/forwarded into OpenSSL Makefile"
      );
      BOOST_REQUIRE_MESSAGE(
        !boost::contains(makefile_content, "-DHFC_GENEX_DEBUG_ONLY=1"),
        "Debug-branch genex option leaked into the Release OpenSSL Makefile (config branch not evaluated correctly)"
      );
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-Wl,--gc-sections"),
        "Toolchain genex link option $<$<PLATFORM_ID:Linux>:-Wl,--gc-sections> was not resolved/forwarded into OpenSSL Makefile"
      );
      // $<$<CONFIG:Release>:LINKER:--as-needed> combines a genex with a
      // LINKER: prefix. The resolver emits LINKER:--as-needed, then
      // hunter_get_build_flags translates to the compiler-specific form.
      bool is_clang_for_genex = boost::contains(data.toolchain_name, "clang");
      if (is_clang_for_genex) {
        BOOST_REQUIRE_MESSAGE(
          boost::contains(makefile_content, "-Xlinker --as-needed"),
          "Toolchain genex producing LINKER:--as-needed was not translated for Clang in OpenSSL Makefile. Toolchain: " + data.toolchain_name
        );
      } else {
        BOOST_REQUIRE_MESSAGE(
          boost::contains(makefile_content, "-Wl,--as-needed"),
          "Toolchain genex producing LINKER:--as-needed was not translated for GCC in OpenSSL Makefile. Toolchain: " + data.toolchain_name
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
          "Unresolved generator expression ('$<') leaked into a flag line of the OpenSSL Makefile. Offending line: " + leaking_line
        );
      }

      // $<LINK_LANGUAGE:C/CXX> — OpenSSL uses default HERMETIC_CONFIG_LANGUAGE=C.
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "HFC_LINKLANG_C=0"),
        "$<LINK_LANGUAGE:C> link option missing from OpenSSL LDFLAGS (default HERMETIC_CONFIG_LANGUAGE=C)."
      );
      BOOST_REQUIRE_MESSAGE(
        !boost::contains(makefile_content, "HFC_LINKLANG_CXX=0"),
        "$<LINK_LANGUAGE:CXX> link option leaked into OpenSSL LDFLAGS despite default HERMETIC_CONFIG_LANGUAGE=C."
      );
    }
  }

}
