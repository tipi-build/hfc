#define BOOST_TEST_MODULE check_openssl_build_system
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

  using namespace std::literals;
  using namespace hfc::test;

  BOOST_DATA_TEST_CASE(build_hermetic_openssl, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path template_path = prepare_project_to_be_tested("check_openssl_build_system", data.is_cmake_re);
    write_simple_main(template_path,{"openssl/x509.h"});

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    run_command(cmake_configure_command, template_path);

    std::string cmake_build_command = get_cmake_build_command(template_path, data);
    run_command(cmake_build_command, template_path);

    BOOST_REQUIRE(fs::exists(template_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "OpenSSL-install" / "lib" / "libssl.a"  ));
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "OpenSSL-install" / "lib" / "libcrypto.a"  ));

    std::string ninja_output = run_command(cmake_build_command, template_path);
    BOOST_REQUIRE(boost::contains(ninja_output, "ninja: no work to do"));
  }

  BOOST_DATA_TEST_CASE(check_force_system_openssl, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_openssl_build_system",data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    write_simple_main(test_project_path,{"openssl/x509.h"});
    append_to_toolchain(project_toolchain, "set(FORCE_SYSTEM_OpenSSL ON CACHE BOOL \"\")");

    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path);

    std::string cmake_build_command = get_cmake_build_command(test_project_path, data);
    run_command(cmake_build_command, test_project_path);

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(!fs::exists(test_project_path / "thirdparty" / "cache" / "openssl-src"));
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "OpenSSL-install" / "lib" / "libssl.a"  ));
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "OpenSSL-install" / "lib" / "libcrypto.a"  ));

    std::string ninja_output = run_command(cmake_build_command, test_project_path);
    BOOST_REQUIRE(boost::contains(ninja_output, "ninja: no work to do"));
  }
}