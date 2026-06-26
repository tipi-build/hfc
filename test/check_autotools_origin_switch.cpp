#define BOOST_TEST_MODULE check_autotools_origin_switch
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>

#include <test_project.hpp>
#include <test_variant.hpp>
#include <test_helpers.hpp>
#include <test_isolation_fixture.hpp>

#include <pre/file/string.hpp>

#include <sstream>
#include <string>

// Online regression: a *real* autotools dependency whose origin is switched to a
// fork and then reverted to the original, in place -- same build folder and
// source cache dir.
//
// In cmake-re mode PROJECT_BINARY_DIR is a symlink. On the revert the outer
// logic checks the configure marker through the symlink (still pointing at the
// fork's mirror) -> not found -> dep_need_configure = ON. The mirror preparation
// then re-points the symlink back to the original origin's mirror, which still
// holds the first build's configure marker. The (removed) inner
// `if(NOT EXISTS already_configured_file)` guard in hfc_autootols_configure then
// re-checked the marker through that switched symlink, found the stale marker and
// SKIPPED ./configure, so the freshly-repopulated sources had no Makefile:
//
//     make: *** No targets specified and no makefile found.  Stop.
//
// This is the discriminating proof for the fix: it FAILS without the fix and
// PASSES with it. Reproduction requires, with the genuine autotools toolchain:
//   - the revert step (orig -> fork -> orig): the original mirror's stale marker
//     is what the switched symlink resolves to (a lone orig -> fork switch to a
//     fresh mirror does not reproduce);
//   - a stable cmake-re host worktree (one UUID across configures) so the symlink
//     state carries across the origin changes;
//   - TIPI_CACHE_CONSUME_ONLY=ON so the install tree is not re-populated and the
//     revert keeps dep_need_configure ON while the stale marker is reachable.

namespace hfc::test {
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
  using namespace std::string_literals;

  namespace {
    void write_consumer(const fs::path& project_path, const std::string& url, const std::string& tag) {
      std::stringstream cml;
      cml << "set(FETCHCONTENT_QUIET OFF CACHE BOOL \"\" FORCE)\n"
          << "cmake_minimum_required(VERSION 3.27.6)\n"
          << "project(AutotoolsOriginSwitch VERSION 1.0 LANGUAGES CXX)\n"
          << "set(CMAKE_MODULE_PATH \"${CMAKE_CURRENT_SOURCE_DIR}/cmake\""
          << " \"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules\" ${CMAKE_MODULE_PATH})\n"
          << "include(HermeticFetchContent)\n\n"
          << "FetchContent_Declare(Iconv\n"
          << "  GIT_REPOSITORY \"" << url << "\"\n"
          << "  GIT_TAG \"" << tag << "\")\n"
          << "FetchContent_MakeHermetic(Iconv\n"
          << "  HERMETIC_BUILD_SYSTEM autotools\n"
          << "  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION\n"
          << "    [=[\n"
          << "      add_library(Iconv::Iconv STATIC IMPORTED)\n"
          << "      set_property(TARGET Iconv::Iconv PROPERTY IMPORTED_LOCATION \"@HFC_PREFIX_PLACEHOLDER@/lib/libiconv.a\")\n"
          << "      set_property(TARGET Iconv::Iconv PROPERTY INTERFACE_INCLUDE_DIRECTORIES @HFC_PREFIX_PLACEHOLDER@/include)\n"
          << "    ]=])\n"
          << "HermeticFetchContent_MakeAvailableAtConfigureTime(Iconv)\n\n"
          << "add_executable(MyExample simple_example.cpp)\n"
          << "target_link_libraries(MyExample PRIVATE Iconv::Iconv)\n";
      pre::file::from_string((project_path / "CMakeLists.txt").generic_string(), cml.str());
    }
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_autotools_origin_switch,
      boost::unit_test::data::make(hfc::test::test_variants()), data) {
    // A real autotools sample and a same-basename fork, switched in place.
    const std::string orig_url = "https://github.com/tipi-build/unittest-autotools-sample.git";
    const std::string orig_tag = "ad80b024eeda8f4c0a96eedf669dc453ed33a094";
    const std::string fork_url = "https://github.com/nxxm/unittest-autotools-sample.git";
    const std::string fork_tag = "da659594da39c7a3061407ed100bed1b36dbb7bc";

    fs::path project_path = prepare_project_to_be_tested("check_autotools_origin_switch", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, {}, "simple_example.cpp");

    const std::string build_command = get_cmake_build_command(project_path, data);
    const std::string lbl = data.is_cmake_re ? "cmake-re"s : "cmake"s;

    // One stable UUID across all configures: a real user reconfigures the SAME
    // project (same cmake-re host worktree), so the symlink state carries across
    // the origin changes -- which is what surfaces the bug on the revert.
    const std::string uuid = data.is_cmake_re ? boost::uuids::to_string(boost::uuids::random_generator()()) : "";

    // Consume-only: the dependency install tree is not re-populated into the L1
    // cache, so the revert keeps dep_need_configure ON while the stale configure
    // marker is still reachable through the switched symlink.
    test_env["TIPI_CACHE_CONSUME_ONLY"] = "ON";

    auto configure = [&]() {
      std::string configure_command =
        get_cmake_configure_command(project_path, data, "", std::nullopt, std::nullopt, uuid);
      return run_cmd(test_env, bp::start_dir=(project_path), bp::shell, configure_command);
    };
    auto build = [&]() {
      return run_cmd(test_env, bp::start_dir=(project_path), bp::shell, build_command);
    };

    // 1. Baseline at the original origin.
    write_consumer(project_path, orig_url, orig_tag);
    {
      auto cfg = configure();
      BOOST_REQUIRE_MESSAGE(cfg.return_code == 0, "[" << lbl << "] baseline configure failed:\n" << cfg.output);
      auto bld = build();
      BOOST_REQUIRE_MESSAGE(bld.return_code == 0, "[" << lbl << "] baseline build failed:\n" << bld.output);
    }

    // 2. Switch the origin to the fork, in place.
    write_consumer(project_path, fork_url, fork_tag);
    {
      auto cfg = configure();
      BOOST_REQUIRE_MESSAGE(cfg.return_code == 0, "[" << lbl << "] configure to fork failed:\n" << cfg.output);
      auto bld = build();
      BOOST_REQUIRE_MESSAGE(bld.return_code == 0, "[" << lbl << "] build to fork failed:\n" << bld.output);
    }

    // 3. Revert the origin back to the original, in place. This is the trigger:
    //    the original origin's mirror still holds the step-1 configure marker,
    //    which the switched symlink resolves to.
    write_consumer(project_path, orig_url, orig_tag);
    {
      auto cfg = configure();
      BOOST_CHECK_MESSAGE(
        !boost::contains(cfg.output, "No targets specified and no makefile found"),
        "[" << lbl << "] regression: autotools dep ./configure skipped after reverting "
        "origin in place (make found no Makefile)");
      BOOST_REQUIRE_MESSAGE(cfg.return_code == 0,
        "[" << lbl << "] configure failed after reverting origin in place:\n" << cfg.output);
      auto bld = build();
      BOOST_CHECK_MESSAGE(bld.return_code == 0,
        "[" << lbl << "] build failed after reverting origin in place:\n" << bld.output);
    }
  }
}
