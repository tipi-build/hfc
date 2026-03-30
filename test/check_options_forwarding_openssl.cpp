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
    fs::path test_project_path = prepare_project_to_be_tested("check_options_forwarding_openssl", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

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
      
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-DTIPI_TEAM=1"),
        "add_compile_definitions 'TIPI_TEAM=1' not found in OpenSSL Makefile"
      );
      
      BOOST_REQUIRE_MESSAGE(
        boost::contains(makefile_content, "-DTIPI_TEAM_ZURICH"),
        "add_compile_definitions 'TIPI_TEAM_ZURICH' not found in OpenSSL Makefile"
      );
    }
  }

}
