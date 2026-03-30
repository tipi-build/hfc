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

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_options_forwarding_openssl, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_options_forwarding_openssl", data.is_cmake_re, temp_dir);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    // Add compile options, definitions, and link options to the toolchain file
    append_to_toolchain(project_toolchain, R"(
# Toolchain-level flags that should be propagated
add_compile_options(-Wno-deprecated-declarations)
add_compile_definitions(TOOLCHAIN_OPENSSL_DEF=1 TOOLCHAIN_OPENSSL_STRING)
add_link_options(-Wl,--build-id)
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

    fs::path openssl_makefile = test_project_path / "build" / "_deps" / "OpenSSL-build" / "src" / "Makefile";
    if(fs::exists(openssl_makefile)) {
      std::string makefile_content = pre::file::to_string(openssl_makefile.generic_string());

      // Check that toolchain-level flags ARE forwarded
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-Wno-deprecated"),
        "Toolchain compile option -Wno-deprecated not found in OpenSSL Makefile (but should be forwarded from toolchain)"
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
    }
  }

}
