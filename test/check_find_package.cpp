#define BOOST_TEST_MODULE check_find_package
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

  // negative test case to the above
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_find_package_neg, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_find_package", data.is_cmake_re, temp_dir);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);
    write_simple_main(test_project_path, {"MathFunctions.h", "MathFunctionscbrt.h", "lib.h"});

    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON"; // we need this for the HFC assertions

    auto result = run_cmd(bp::start_dir=(test_project_path), test_env, bp::shell, get_cmake_configure_command(test_project_path, data, "-DTEST_DATA_DISABLE_FIND_HfcDependencyProvidedLib=ON"));
    BOOST_TEST_INFO("Command output: \n" << result.output);
    BOOST_REQUIRE_NE(result.return_code, 0); // no success please!
    
    // example output 
    // -- [HERMETIC_FETCHCONTENT 2024-11-27T21:06:30] Received find_package() request for package name HfcDependencyProvidedLib
    // -- [HERMETIC_FETCHCONTENT 2024-11-27T21:06:30]  - not in list of hermetic dependencies
    BOOST_REQUIRE(boost::regex_search(result.output, boost::regex{"-- \\[HERMETIC_FETCHCONTENT .+\\] Received find_package\\(\\) request for package name HfcDependencyProvidedLib"}));
    BOOST_REQUIRE(boost::regex_search(result.output, boost::regex{"-- \\[HERMETIC_FETCHCONTENT .+\\]  - not in list of hermetic dependencies"}));
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_find_package, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_find_package", data.is_cmake_re, temp_dir);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);
    write_simple_main(test_project_path, { "MathFunctions.h", "MathFunctionscbrt.h", "lib.h" });

    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON"; // we need this for the HFC assertions
    std::string configure_output = run_command(get_cmake_configure_command(test_project_path, data), test_project_path, test_env);
    BOOST_REQUIRE(boost::contains(configure_output , "Received find_package() request for package name HfcDependencyProvidedLib"));
    BOOST_REQUIRE(boost::regex_search(configure_output, boost::regex{"-- \\[HERMETIC_FETCHCONTENT .+\\] Received find_package\\(\\) request for package name HfcDependencyProvidedLib"}));
    BOOST_REQUIRE(boost::regex_search(configure_output, boost::regex{"loading target cache from .*hermetic_targetcaches/HfcDependencyProvidedLib.cmake"}));
    // Verify HERMETIC_FIND_PACKAGE_HfcDependencyProvidedLib_EXTRA_CODE was executed
    // and that code in it can set variables visible to the consuming project (mathlib).
    BOOST_REQUIRE(boost::contains(configure_output, "[hfc_test] HfcDependencyProvidedLib_VERSION=1.2.3.hfc_test"));
    // Verify <content-name>_HERMETIC_INSTALL_PREFIX is exposed and non-empty, pointing at
    // the HFC install tree for this dependency (cache-emulation path).
    BOOST_REQUIRE(boost::regex_search(configure_output, boost::regex{"\\[hfc_test\\] HfcDependencyProvidedLib_HERMETIC_INSTALL_PREFIX=\\S*HfcDependencyProvidedLib-install"}));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "HfcDependencyProvidedLib-install" / "lib" / "libiconv.a"));

    run_command(get_cmake_build_command(test_project_path, data), test_project_path, test_env);
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctionscbrt.a"));
  }

  // Tests HERMETIC_DEFER_NATIVE_ROOTED_FIND_PACKAGE_FOR: when a consumer dep
  // lists a package in this option, HFC sets <Pkg>_ROOT to the HFC install
  // prefix and forwards find_package() to CMake's native implementation
  // (BYPASS_PROVIDER) instead of serving it from the HFC target cache.
  //
  // The template injects FindHfcDependencyProvidedLib.cmake via
  // HERMETIC_TOOLCHAIN_EXTENSION so that the native find_package() can locate
  // the library using <Pkg>_ROOT -- exactly as a real project's Find module
  // (e.g. FindArrow.cmake) would do.
  //
  // Key assertions:
  //   - cmake output contains the "forwarding to native" log from HFC, proving
  //     the new code path was taken instead of the target-cache emulation path
  //   - mathlib configure and build succeed end-to-end
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_resolve_packages_natively, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_resolve_packages_natively", data.is_cmake_re, temp_dir);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);
    write_simple_main(test_project_path, { "MathFunctions.h", "MathFunctionscbrt.h", "lib.h" });

    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON"; // we need this for the HFC assertions

    std::string configure_output = run_command(get_cmake_configure_command(test_project_path, data), test_project_path, test_env);

    // HFC must have taken the native-resolve path (not the target-cache path)
    BOOST_REQUIRE_MESSAGE(
      boost::regex_search(configure_output, boost::regex{"forwarding to native find_package\\(\\) with HFC install root"}),
      "expected 'forwarding to native find_package()' in cmake output -- HERMETIC_DEFER_NATIVE_ROOTED_FIND_PACKAGE_FOR code path not taken"
    );

    // The native Find module must have located the library via _ROOT
    BOOST_REQUIRE_MESSAGE(
      boost::regex_search(configure_output, boost::regex{"Found HfcDependencyProvidedLib"}),
      "FindHfcDependencyProvidedLib.cmake did not find the library via HfcDependencyProvidedLib_ROOT"
    );
    // Verify HERMETIC_FIND_PACKAGE_HfcDependencyProvidedLib_EXTRA_CODE was executed
    // in the native-forward path and that code in it can set variables visible to mathlib.
    BOOST_REQUIRE(boost::contains(configure_output, "[hfc_test] HfcDependencyProvidedLib_VERSION=1.2.3.hfc_test"));
    // <content-name>_HERMETIC_INSTALL_PREFIX must also be exposed on the native-forward path.
    BOOST_REQUIRE(boost::regex_search(configure_output, boost::regex{"\\[hfc_test\\] HfcDependencyProvidedLib_HERMETIC_INSTALL_PREFIX=\\S*HfcDependencyProvidedLib-install"}));

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "HfcDependencyProvidedLib-install" / "lib" / "libiconv.a"));

    run_command(get_cmake_build_command(test_project_path, data), test_project_path, test_env);
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctions.a"));
  }

  // Tests alias resolution via find_package() in sub-builds: the autotools iconv lib
  // is declared under canonical name "iconv_lib", then aliased as "HfcDependencyProvidedLib".
  // When mathlib's CMakeLists.txt does find_package(HfcDependencyProvidedLib), the HFC
  // provider must resolve the alias and serve iconv_lib's target cache.
  // This exercises the HermeticFetchContent_ResolveContentNameAlias code path inside
  // hfc_provide_dependency_FINDPACKAGE.cmake and the alias forwarding in proxy toolchains.
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_find_package_via_alias, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_find_package_via_alias", data.is_cmake_re, temp_dir);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);
    write_simple_main(test_project_path, { "MathFunctions.h", "MathFunctionscbrt.h", "lib.h" });

    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON";
    std::string configure_output = run_command(get_cmake_configure_command(test_project_path, data), test_project_path, test_env);

    // The provider must receive "HfcDependencyProvidedLib" and resolve it via alias
    BOOST_REQUIRE(boost::regex_search(configure_output, boost::regex{"Received find_package\\(\\) request for package name HfcDependencyProvidedLib"}));
    BOOST_REQUIRE(boost::regex_search(configure_output, boost::regex{"resolved to alias iconv_lib"}));

    // The target cache served must be iconv_lib's (not a separate HfcDependencyProvidedLib content)
    BOOST_REQUIRE(boost::regex_search(configure_output, boost::regex{"loading target cache from .*hermetic_targetcaches/iconv_lib.cmake"}));

    // EXTRA_CODE must still work through alias resolution
    BOOST_REQUIRE(boost::contains(configure_output, "[hfc_test] HfcDependencyProvidedLib_VERSION=1.2.3.alias_test"));
    // The variable is keyed on the content name (HfcDependencyProvidedLib) but its value must be
    // the canonical dependency's HFC install prefix (iconv_lib-install), proving alias resolution
    // feeds the exposed prefix correctly.
    BOOST_REQUIRE(boost::regex_search(configure_output, boost::regex{"\\[hfc_test\\] HfcDependencyProvidedLib_HERMETIC_INSTALL_PREFIX=\\S*iconv_lib-install"}));

    // iconv_lib install path (canonical name, not alias)
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "iconv_lib-install" / "lib" / "libiconv.a"));

    run_command(get_cmake_build_command(test_project_path, data), test_project_path, test_env);
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctionscbrt.a"));
  }

  // Regression test for the interaction between alias resolution, deduplication and
  // HERMETIC_DEFER_NATIVE_ROOTED_FIND_PACKAGE_FOR.
  //
  // The template lists HERMETIC_FIND_PACKAGES "iconv_lib;HfcDependencyProvidedLib" (canonical
  // FIRST, alias SECOND) and requests defer-to-native via the ALIAS name only. The proxy
  // toolchain generator resolves every entry to its canonical name and deduplicates: the
  // canonical "iconv_lib" is registered first and the alias entry (which carries the defer
  // flag) is dropped by the dedup. Unless the defer flag is matched on canonical names, it is
  // lost purely because of list order -- the consumer asked for native-rooted find_package()
  // but HFC silently serves from the target cache instead.
  //
  // Asserts the provider takes the native-forward path ("forwarding to native find_package()"),
  // which only happens if the defer flag survived alias resolution + dedup.
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_find_package_defer_native_via_alias, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_find_package_via_alias", data.is_cmake_re, temp_dir);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);
    write_simple_main(test_project_path, { "MathFunctions.h", "MathFunctionscbrt.h", "lib.h" });

    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON";
    std::string configure_output = run_command(
      get_cmake_configure_command(test_project_path, data, "-DTEST_DATA_DEFER_VIA_ALIAS=ON"),
      test_project_path, test_env);

    // The defer flag was requested via the alias name but must apply to the canonical
    // registration: the provider must take the native-forward path, not target-cache emulation.
    BOOST_REQUIRE_MESSAGE(
      boost::regex_search(configure_output, boost::regex{"forwarding to native find_package\\(\\) with HFC install root"}),
      "HERMETIC_DEFER_NATIVE_ROOTED_FIND_PACKAGE_FOR requested via the alias name was dropped "
      "during alias resolution + dedup -- provider fell back to target-cache emulation");

    // And the native Find module must have located the lib via the canonical _ROOT.
    BOOST_REQUIRE(boost::regex_search(configure_output, boost::regex{"Found iconv_lib"}));

    run_command(get_cmake_build_command(test_project_path, data), test_project_path, test_env);
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample"));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-install" / "lib" / "libMathFunctions.a"));
  }

  // Negative test for HERMETIC_DEFER_NATIVE_ROOTED_FIND_PACKAGE_FOR: verifies that when
  // HFC does not provide the package, the native find_package() does NOT fall back to a
  // system-installed library (e.g. the system iconv), preserving hermeticity.
  //
  // With TEST_DATA_DISABLE_FIND_HfcDependencyProvidedLib=ON both HERMETIC_FIND_PACKAGES
  // and HERMETIC_DEFER_NATIVE_ROOTED_FIND_PACKAGE_FOR are emptied for mathlib, so the HFC
  // provider has no registration for HfcDependencyProvidedLib.  The provider must intercept
  // the request and reject it ("not in list of hermetic dependencies") instead of leaking to
  // the system.  The FindHfcDependencyProvidedLib.cmake module uses NO_DEFAULT_PATH in its
  // find_path/find_library calls, so even if it runs it will not find any system iconv.
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_resolve_packages_natively_neg, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_resolve_packages_natively", data.is_cmake_re, temp_dir);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);

    append_random_testdata_marker_as_toolchain_comment(project_toolchain, data);
    write_simple_main(test_project_path, { "MathFunctions.h", "MathFunctionscbrt.h", "lib.h" });

    test_env["HERMETIC_FETCHCONTENT_LOG_DEBUG"] = "ON";

    auto result = run_cmd(bp::start_dir=(test_project_path), bp::env=test_env, bp::shell,
      get_cmake_configure_command(test_project_path, data, "-DTEST_DATA_DISABLE_FIND_HfcDependencyProvidedLib=ON"));
    BOOST_TEST_INFO("Command output: \n" << result.output);
    BOOST_REQUIRE_NE(result.return_code, 0); // must fail — system iconv must not be used

    // The provider must have been invoked and rejected the request hermetically
    BOOST_REQUIRE(boost::regex_search(result.output, boost::regex{"-- \\[HERMETIC_FETCHCONTENT .+\\] Received find_package\\(\\) request for package name HfcDependencyProvidedLib"}));
    BOOST_REQUIRE(boost::regex_search(result.output, boost::regex{"-- \\[HERMETIC_FETCHCONTENT .+\\]  - not in list of hermetic dependencies"}));
  }
}