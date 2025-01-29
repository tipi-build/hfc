#define BOOST_TEST_MODULE check_change_FETCHCONTENT_BASE_DIR
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
  using namespace std::string_literals;

  BOOST_DATA_TEST_CASE(FETCHCONTENT_BASE_DIR_works, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_change_base_dir", data.is_cmake_re);
    write_simple_main(test_project_path,{"MathFunctions.h", "MathFunctionscbrt.h","lib.h"});

    fs::path overridden_FETCHCONTENT_BASE_DIR = test_project_path / "change_base";
    fs::create_directories(overridden_FETCHCONTENT_BASE_DIR);

    run_command(
      get_cmake_configure_command(test_project_path, data)
      + " -DFETCHCONTENT_BASE_DIR="s + overridden_FETCHCONTENT_BASE_DIR.generic_string(), test_project_path);

    if (!data.is_cmake_re) {
      BOOST_REQUIRE(fs::exists(test_project_path / "thirdparty" / "cache" / "mathlib-ecc756a4-src" / "CMakeLists.txt"));
      BOOST_REQUIRE(fs::exists(test_project_path / "thirdparty" / "cache" / "Iconv-ad80b024-src" / "configure"));
    }
    BOOST_REQUIRE(fs::exists(overridden_FETCHCONTENT_BASE_DIR / "mathlib-build" / "CMakeCache.txt"));
    BOOST_REQUIRE(fs::exists(overridden_FETCHCONTENT_BASE_DIR / "Iconv-adapter" / "CMakeLists.txt"));
    run_command(get_cmake_build_command(test_project_path, data), test_project_path);
    BOOST_REQUIRE(fs::exists(overridden_FETCHCONTENT_BASE_DIR / "mathlib-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(overridden_FETCHCONTENT_BASE_DIR / "Iconv-install" / "lib" / "libiconv.a"));
  }

  BOOST_DATA_TEST_CASE(HERMETIC_FETCHCONTENT_INSTALL_DIR_works, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_change_base_dir", data.is_cmake_re);
    write_simple_main(test_project_path,{"MathFunctions.h", "MathFunctionscbrt.h","lib.h"});

    fs::path overriden_HERMETIC_FETCHCONTENT_INSTALL_DIR = test_project_path / "test-installdir";
    fs::create_directories(overriden_HERMETIC_FETCHCONTENT_INSTALL_DIR);

    run_command(
      get_cmake_configure_command(test_project_path, data)
      + " -DHERMETIC_FETCHCONTENT_INSTALL_DIR="s + overriden_HERMETIC_FETCHCONTENT_INSTALL_DIR.generic_string(), test_project_path);

    if (!data.is_cmake_re) {
      BOOST_REQUIRE(fs::exists(test_project_path / "thirdparty" / "cache" / "mathlib-ecc756a4-src" / "CMakeLists.txt"));
      BOOST_REQUIRE(fs::exists(test_project_path / "thirdparty" / "cache" / "Iconv-ad80b024-src" / "configure"));
    }
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-build" / "CMakeCache.txt"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "Iconv-adapter" / "CMakeLists.txt"));
    run_command(get_cmake_build_command(test_project_path, data), test_project_path);
    BOOST_REQUIRE(fs::exists(overriden_HERMETIC_FETCHCONTENT_INSTALL_DIR / "mathlib-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(overriden_HERMETIC_FETCHCONTENT_INSTALL_DIR / "Iconv-install" / "lib" / "libiconv.a"));
  }

  BOOST_DATA_TEST_CASE(FETCHCONTENT_BASE_DIR_and_HERMETIC_FETCHCONTENT_INSTALL_DIR_combined, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_change_base_dir", data.is_cmake_re);
    write_simple_main(test_project_path,{"MathFunctions.h", "MathFunctionscbrt.h","lib.h"});

    fs::path overriden_HERMETIC_FETCHCONTENT_INSTALL_DIR = test_project_path / "test-installdir";
    fs::create_directories(overriden_HERMETIC_FETCHCONTENT_INSTALL_DIR);

    fs::path overridden_FETCHCONTENT_BASE_DIR = test_project_path / "change_base";
    fs::create_directories(overridden_FETCHCONTENT_BASE_DIR);

    run_command(
      get_cmake_configure_command(test_project_path, data)
      + " -DHERMETIC_FETCHCONTENT_INSTALL_DIR="s + overriden_HERMETIC_FETCHCONTENT_INSTALL_DIR.generic_string()
      + " -DFETCHCONTENT_BASE_DIR="s + overridden_FETCHCONTENT_BASE_DIR.generic_string(), test_project_path);

    if (!data.is_cmake_re) {
      BOOST_REQUIRE(fs::exists(test_project_path / "thirdparty" / "cache" / "mathlib-ecc756a4-src" / "CMakeLists.txt"));
      BOOST_REQUIRE(fs::exists(test_project_path / "thirdparty" / "cache" / "Iconv-ad80b024-src" / "configure"));
    }
    BOOST_REQUIRE(fs::exists(overridden_FETCHCONTENT_BASE_DIR / "mathlib-build" / "CMakeCache.txt"));
    BOOST_REQUIRE(fs::exists(overridden_FETCHCONTENT_BASE_DIR / "Iconv-adapter" / "CMakeLists.txt"));

    run_command(get_cmake_build_command(test_project_path, data), test_project_path);

    BOOST_REQUIRE(fs::exists(overriden_HERMETIC_FETCHCONTENT_INSTALL_DIR / "mathlib-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(overriden_HERMETIC_FETCHCONTENT_INSTALL_DIR / "Iconv-install" / "lib" / "libiconv.a"));
  }
}
