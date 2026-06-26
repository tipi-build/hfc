#define BOOST_TEST_MODULE check_autotools_tag_update
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

// Online test: a *real* autotools dependency whose GIT_TAG is bumped to a newer
// commit of the SAME repository, in place (same build folder and source cache
// dir). This is the common "update the pinned commit" case.
//
// It exists to confirm the stale-build bug (and its fix) are real for the
// genuine autotools toolchain (libtool / config.status), not artifacts of the
// hand-written fake-configure scaffold used by the offline migration matrix.
//
// We use the two commits of unittest-autotools-sample that check_version_update
// also uses. They differ only in src/lib.h, so the compiled library is identical
// and the discriminator is the *installed header* (copied by `make install`):
// after bumping v1 -> v2 the installed lib.h must be v2's. If the stale build is
// reused, it stays v1's -> detected.

namespace hfc::test {
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
  using namespace std::string_literals;

  namespace {
    void write_consumer(const fs::path& project_path, const std::string& tag) {
      std::stringstream cml;
      cml << "set(FETCHCONTENT_QUIET OFF CACHE BOOL \"\" FORCE)\n"
          << "cmake_minimum_required(VERSION 3.27.6)\n"
          << "project(AutotoolsTagUpdate VERSION 1.0 LANGUAGES CXX)\n"
          << "set(CMAKE_MODULE_PATH \"${CMAKE_CURRENT_SOURCE_DIR}/cmake\""
          << " \"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules\" ${CMAKE_MODULE_PATH})\n"
          << "include(HermeticFetchContent)\n\n"
          << "FetchContent_Declare(Iconv\n"
          << "  GIT_REPOSITORY \"https://github.com/tipi-build/unittest-autotools-sample.git\"\n"
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

    // The header copied by `make install`. Its content reflects which commit's
    // sources were actually built/installed (the two commits differ only here),
    // so it is the staleness discriminator. Reachable through the build symlink
    // for both native cmake and cmake-re.
    fs::path installed_header(const fs::path& project_path) {
      return project_path / "build" / "_deps" / "Iconv-install" / "include" / "lib.h";
    }
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, check_autotools_tag_update,
      boost::unit_test::data::make(hfc::test::test_variants()), data) {
    // Two commits of the SAME repo that differ only in src/lib.h.
    const std::string v1 = "ad80b024eeda8f4c0a96eedf669dc453ed33a094";
    const std::string v2 = "30258ed5d3227cd37500af8cded3cdc241c288a2";

    fs::path project_path = prepare_project_to_be_tested("check_autotools_tag_update", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, {}, "simple_example.cpp");

    // Stable UUID so cmake-re reuses the same build worktree across both
    // configures (i.e. genuinely in place, no fresh folder masking the bug).
    std::string uuid = data.is_cmake_re ? boost::uuids::to_string(boost::uuids::random_generator()()) : "";
    std::string configure_command = get_cmake_configure_command(project_path, data, "", std::nullopt, std::nullopt, uuid);
    std::string build_command = get_cmake_build_command(project_path, data);

    // 1. Build at v1.
    write_consumer(project_path, v1);
    run_command(configure_command, project_path, test_env);
    run_command(build_command, project_path, test_env);
    BOOST_REQUIRE_MESSAGE(fs::exists(installed_header(project_path)),
      "installed lib.h missing after v1 build: " << installed_header(project_path).generic_string());
    std::string header_v1 = pre::file::to_string(installed_header(project_path).generic_string());
    BOOST_REQUIRE(!header_v1.empty());

    // 2. Bump the tag to v2, in place (same build folder + same cache dir).
    write_consumer(project_path, v2);
    run_command(configure_command, project_path, test_env);
    run_command(build_command, project_path, test_env);
    BOOST_REQUIRE(fs::exists(installed_header(project_path)));
    std::string header_v2 = pre::file::to_string(installed_header(project_path).generic_string());
    BOOST_REQUIRE(!header_v2.empty());

    // The two commits' headers genuinely differ upstream; after the in-place bump
    // the installed header must be v2's. If the stale build/install is reused it
    // stays identical to v1's -> the stale-build bug.
    BOOST_CHECK_MESSAGE(header_v2 != header_v1,
      "STALE: installed lib.h unchanged after bumping GIT_TAG v1->v2 in place "
      "(old autotools build reused). v1/v2 headers:\n--- v1 ---\n" << header_v1
      << "\n--- v2 ---\n" << header_v2);
  }
}
