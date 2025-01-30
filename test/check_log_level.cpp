#define BOOST_TEST_MODULE goldilock_provisioning_test
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

  BOOST_DATA_TEST_CASE(HERMETIC_FETCHCONTENT_LOG_DEBUG_undefined, boost::unit_test::data::make(hfc::test::test_variants()), data){
    bp::environment test_env = boost::this_process::environment();
    test_env.erase("HERMETIC_FETCHCONTENT_LOG_DEBUG");
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE(boost::contains(output,"[HERMETIC_FETCHCONTENT] 🟢 Repository"));
    BOOST_REQUIRE(boost::contains(output,"project-cmake-simple-d4593cd9-src at d4593cd90243a7bdd3507a918d735353f51621b1 and clean"));
    BOOST_REQUIRE(boost::contains(output,"-- Configuring done"));
    BOOST_REQUIRE(boost::contains(output,"-- Generating done"));
    BOOST_REQUIRE(boost::contains(output,"-- Generating done"));

    BOOST_REQUIRE(!boost::contains(output,"Generated proxy toolchain at:"));
    BOOST_REQUIRE(!boost::contains(output,"Using goldilock found at"));
  }

  BOOST_DATA_TEST_CASE(HERMETIC_FETCHCONTENT_LOG_DEBUG_off, boost::unit_test::data::make(hfc::test::test_variants()), data){
    bp::environment test_env = boost::this_process::environment();
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"]="Off";
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE(boost::contains(output,"[HERMETIC_FETCHCONTENT] 🟢 Repository"));
    BOOST_REQUIRE(boost::contains(output,"project-cmake-simple-d4593cd9-src at d4593cd90243a7bdd3507a918d735353f51621b1 and clean"));
    BOOST_REQUIRE(boost::contains(output,"-- Configuring done"));
    BOOST_REQUIRE(boost::contains(output,"-- Generating done"));
    BOOST_REQUIRE(boost::contains(output,"-- Generating done"));

    BOOST_REQUIRE(!boost::contains(output,"Generated proxy toolchain at:"));
    BOOST_REQUIRE(!boost::contains(output,"Using goldilock found at"));
  }

  BOOST_DATA_TEST_CASE(HERMETIC_FETCHCONTENT_LOG_DEBUG_on, boost::unit_test::data::make(hfc::test::test_variants()), data){
    bp::environment test_env = boost::this_process::environment();
    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"]="oN";
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");

    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    auto output = run_command(cmake_configure_command, template_path, test_env);

    BOOST_REQUIRE(boost::contains(output,"[HERMETIC_FETCHCONTENT] 🟢 Repository"));
    BOOST_REQUIRE(boost::contains(output,"project-cmake-simple-d4593cd9-src at d4593cd90243a7bdd3507a918d735353f51621b1 and clean"));
    BOOST_REQUIRE(boost::contains(output,"-- Configuring done"));
    BOOST_REQUIRE(boost::contains(output,"-- Generating done"));
    BOOST_REQUIRE(boost::contains(output,"-- Generating done"));

    BOOST_REQUIRE(boost::contains(output,"Generated proxy toolchain at:"));
    BOOST_REQUIRE(boost::contains(output,"Using goldilock found at"));
  }

  BOOST_DATA_TEST_CASE(HERMETIC_FETCHCONTENT_LOG_DEBUG_cmake_cache, boost::unit_test::data::make(hfc::test::test_variants()), data){
    bp::environment test_env = boost::this_process::environment();
    test_env.erase("HERMETIC_FETCHCONTENT_LOG_DEBUG");
    fs::path template_path = prepare_project_to_be_tested("check_without_install", data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    write_simple_main(template_path,{}, "simple_main.cpp");

    {
      std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
      auto output = run_command(cmake_configure_command, template_path, test_env);

      BOOST_REQUIRE(boost::contains(output,"[HERMETIC_FETCHCONTENT] 🟢 Repository"));
      BOOST_REQUIRE(boost::contains(output,"project-cmake-simple-d4593cd9-src at d4593cd90243a7bdd3507a918d735353f51621b1 and clean"));
      BOOST_REQUIRE(boost::contains(output,"-- Configuring done"));
      BOOST_REQUIRE(boost::contains(output,"-- Generating done"));
      BOOST_REQUIRE(boost::contains(output,"-- Generating done"));

      BOOST_REQUIRE(!boost::contains(output,"Generated proxy toolchain at:"));
      BOOST_REQUIRE(!boost::contains(output,"Using goldilock found at"));
    }

    {
      std::string cmake_configure_command = get_cmake_configure_command(template_path, data)
        + " -DHERMETIC_FETCHCONTENT_LOG_DEBUG=oN";
      auto output = run_command(cmake_configure_command, template_path, test_env);

      BOOST_REQUIRE(boost::contains(output,"[HERMETIC_FETCHCONTENT] 🟢 Repository"));
      BOOST_REQUIRE(boost::contains(output,"project-cmake-simple-d4593cd9-src at d4593cd90243a7bdd3507a918d735353f51621b1 and clean"));
      BOOST_REQUIRE(boost::contains(output,"-- Configuring done"));
      BOOST_REQUIRE(boost::contains(output,"-- Generating done"));
      BOOST_REQUIRE(boost::contains(output,"-- Generating done"));

      BOOST_REQUIRE(boost::contains(output,"Generated proxy toolchain at:"));
      BOOST_REQUIRE(boost::contains(output,"Using goldilock found at"));
    }

    {
      std::string cmake_configure_command = get_cmake_configure_command(template_path, data)
        + " -DHERMETIC_FETCHCONTENT_LOG_DEBUG=OFF";
      auto output = run_command(cmake_configure_command, template_path, test_env);

      BOOST_REQUIRE(boost::contains(output,"[HERMETIC_FETCHCONTENT] 🟢 Repository"));
      BOOST_REQUIRE(boost::contains(output,"project-cmake-simple-d4593cd9-src at d4593cd90243a7bdd3507a918d735353f51621b1 and clean"));
      BOOST_REQUIRE(boost::contains(output,"-- Configuring done"));
      BOOST_REQUIRE(boost::contains(output,"-- Generating done"));
      BOOST_REQUIRE(boost::contains(output,"-- Generating done"));

      BOOST_REQUIRE(!boost::contains(output,"Generated proxy toolchain at:"));
      BOOST_REQUIRE(!boost::contains(output,"Using goldilock found at"));
    }

  }
}