#define BOOST_TEST_MODULE check_source_cache_reuse
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>

#include <test_project.hpp>
#include <test_variant.hpp>
#include <test_helpers.hpp>
#include <test_isolation_fixture.hpp>

#include <pre/file/string.hpp>
#include <pre/file/hash.hpp>

#include <sstream>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Central source cache reuse: a dependency must be downloaded exactly once
// ─────────────────────────────────────────────────────────────────────────────
//
// HFC's whole point is that the central source cache (thirdparty/cache) is
// populated once and shared: a *new build tree* against a warm cache must not
// re-download — and above all must never wipe — the cached sources
// (tipi-build/specs-cmake-re#96). Under CMake 4 / CMP0168 NEW, FetchContent's
// direct population keeps its stamps per-build-tree in
// ${CMAKE_BINARY_DIR}/CMakeFiles/fc-stamp and its git clone step rm -rf's the
// SOURCE_DIR when they are missing — so without HFC's CMP0168 pin
// (hfc_policy_helpers.cmake) a second build tree nukes and re-clones the
// shared cache, and offline work is impossible.
//
// These scenarios lock that property in the strictest possible way: after the
// first build tree has populated the cache, the *origin is renamed away*. Any
// attempt to contact it — clone, fetch, archive download — then fails loudly
// instead of silently re-downloading. The consumer also opts into
// `cmake_policy(SET CMP0168 NEW)` (when the policy exists) to model a modern
// `cmake_minimum_required(VERSION 3.30+)` consumer, which is exactly the
// configuration that broke without the pin.
//
// Scenarios (each parameterised over native cmake `_0` and cmake-re `_1`):
//  - git_second_build_tree_reuses_cache_without_origin: warm clean clone →
//    fresh build tree works offline-from-origin, HEAD untouched, no clone ran.
//  - url_second_build_tree_reuses_cache_without_origin: same for URL/archive
//    content (whose reuse rests on HFC's populate marker, not on a git check).
//  - git_branch_tag_follows_branch_without_reclone: a branch GIT_TAG is
//    followed via in-place fetch+checkout — never a re-clone — and keeps
//    working from the local clone when the origin is unreachable (native
//    cmake only: branch moves do not change cmake-re's ORIGIN+REVISION key).
//  - git_local_modifications_survive_new_build_tree: local (uncommitted)
//    changes in the cached clone survive a new build tree and are what gets
//    built (the debugging workflow of hfc commit a56de21d). Native cmake only:
//    cmake-re's install-tree cache is keyed on ORIGIN+REVISION, which a local
//    source edit deliberately does not change.

namespace hfc::test {
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
  using namespace std::string_literals;

  namespace {

    std::string make_uuid() {
      return boost::uuids::to_string(boost::uuids::random_generator()());
    }

    // git, run with a deterministic identity so it works on bare CI accounts.
    std::string git_with_identity() {
      fs::path git_bin = bp::search_path("git");
      return git_bin.generic_string() +
        " -c user.email=hfc-test@example.com -c user.name=hfc-test -c init.defaultBranch=main"s +
        " -c commit.gpgsign=false -c advice.detachedHead=false"s;
    }

    // Writes a minimal, installable CMake library into `dir`;
    // samplelib_variant() returns `variant_id` (-> consumer exit code).
    void write_lib_source(const fs::path& dir, int variant_id) {
      fs::create_directories(dir);

      std::stringstream cml;
      cml << "cmake_minimum_required(VERSION 3.20)\n"
          << "project(samplelib VERSION 1.0 LANGUAGES CXX)\n"
          << "add_library(samplelib STATIC samplelib.cpp)\n"
          << "target_include_directories(samplelib PUBLIC\n"
          << "  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>\n"
          << "  $<INSTALL_INTERFACE:include>)\n"
          << "install(TARGETS samplelib EXPORT samplelibTargets ARCHIVE DESTINATION lib)\n"
          << "install(FILES samplelib.h DESTINATION include)\n"
          << "install(EXPORT samplelibTargets\n"
          << "  NAMESPACE SampleLib::\n"
          << "  DESTINATION lib/cmake/samplelib\n"
          << "  FILE samplelibTargets.cmake)\n";
      pre::file::from_string((dir / "CMakeLists.txt").generic_string(), cml.str());

      std::stringstream hdr;
      hdr << "#pragma once\n"
          << "int samplelib_variant();\n";
      pre::file::from_string((dir / "samplelib.h").generic_string(), hdr.str());

      std::stringstream src;
      src << "#include \"samplelib.h\"\n"
          << "int samplelib_variant() { return " << variant_id << "; }\n";
      pre::file::from_string((dir / "samplelib.cpp").generic_string(), src.str());
    }

    // Initialises `dir` as a git repo and commits its contents. Returns the SHA.
    std::string git_init_and_commit(const fs::path& dir, bp::environment& env, const std::string& message) {
      run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " init -q");
      run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " add -A");
      run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " commit -q -m \"" + message + "\"");
      run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " branch -M main");
      auto rev = run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " rev-parse HEAD");
      boost::trim(rev.output);
      return rev.output;
    }

    std::string git_head(const fs::path& repo, bp::environment& env) {
      auto rev = run_cmd(env, bp::start_dir=(repo), bp::shell, git_with_identity() + " rev-parse HEAD");
      boost::trim(rev.output);
      return rev.output;
    }

    // Commits the current contents of an existing repo as a new commit on the
    // current branch (moves the branch forward). Returns the new commit SHA.
    std::string git_commit_all(const fs::path& dir, bp::environment& env, const std::string& message) {
      run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " add -A");
      run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " commit -q -m \"" + message + "\"");
      auto rev = run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " rev-parse HEAD");
      boost::trim(rev.output);
      return rev.output;
    }

    // Builds a .tar.gz of `src_dir` (single top-level dir, which FetchContent
    // strips on extraction) at `archive_path`. Returns its SHA1.
    std::string make_archive(const fs::path& src_dir, const fs::path& archive_path, bp::environment& env) {
      fs::path cmake_bin = bp::search_path("cmake");
      fs::path parent = src_dir.parent_path();
      std::string dirname = src_dir.filename().generic_string();
      run_cmd(env, bp::start_dir=(parent), bp::shell,
        cmake_bin.generic_string() + " -E tar czf " + archive_path.generic_string() + " " + dirname);
      return pre::file::sha1sum(archive_path.generic_string());
    }

    // Writes the consumer project. The consumer flips CMP0168 to NEW when the
    // policy exists (CMake >= 3.30) — the configuration of issue #96 — so this
    // test guards HFC's populate-side pin on every CMake that has the policy.
    void write_consumer(const fs::path& project_path, const std::string& declare_body) {
      std::stringstream cml;
      cml << "set(FETCHCONTENT_QUIET OFF CACHE BOOL \"\" FORCE)\n"
          << "cmake_minimum_required(VERSION 3.27.6)\n"
          << "project(CheckSourceCacheReuse VERSION 1.0 LANGUAGES CXX)\n\n"
          << "if(POLICY CMP0168)\n"
          << "  cmake_policy(SET CMP0168 NEW) # modern consumer, see tipi-build/specs-cmake-re#96\n"
          << "endif()\n\n"
          << "set(CMAKE_MODULE_PATH\n"
          << "  \"${CMAKE_CURRENT_SOURCE_DIR}/cmake\"\n"
          << "  \"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules\"\n"
          << "  ${CMAKE_MODULE_PATH})\n"
          << "include(HermeticFetchContent)\n\n"
          << "FetchContent_Declare(\n"
          << "  samplelib\n"
          << declare_body
          << ")\n\n"
          << "FetchContent_MakeHermetic(\n"
          << "  samplelib\n"
          << "  HERMETIC_BUILD_SYSTEM cmake)\n\n"
          << "HermeticFetchContent_MakeAvailableAtConfigureTime(samplelib)\n\n"
          << "add_executable(MyExample simple_example.cpp)\n"
          << "target_link_libraries(MyExample PRIVATE SampleLib::samplelib)\n";
      pre::file::from_string((project_path / "CMakeLists.txt").generic_string(), cml.str());

      pre::file::from_string((project_path / "simple_example.cpp").generic_string(),
        "#include \"samplelib.h\"\nint main() { return samplelib_variant(); }\n");
    }

    struct step_result {
      bool configured = false;
      bool built = false;
      int  variant_seen = -1;   // exit code of MyExample
      std::string configure_output;
      std::string detail;
    };

    step_result configure_build_run(const fs::path& project_path, const test_variant& data,
                                    bp::environment& env, const std::string& uuid) {
      step_result r;

      std::string configure_command = get_cmake_configure_command(
        project_path, data, "", std::nullopt, std::nullopt, uuid);
      auto c = run_cmd(env, bp::start_dir=(project_path), bp::shell, configure_command);
      r.configure_output = c.output;
      r.configured = (c.return_code == 0);
      if(!r.configured) {
        r.detail = "configure failed:\n" + c.output;
        return r;
      }

      std::string build_command = get_cmake_build_command(project_path, data);
      auto b = run_cmd(env, bp::start_dir=(project_path), bp::shell, build_command);
      r.built = (b.return_code == 0);
      if(!r.built) {
        r.detail = "build failed:\n" + b.output;
        return r;
      }

      fs::path exe = project_path / "build" / "MyExample";
      if(!fs::exists(exe)) {
        r.detail = "executable not produced at " + exe.generic_string();
        return r;
      }

      auto run = run_cmd(env, bp::start_dir=(project_path), bp::shell, exe.generic_string());
      r.variant_seen = static_cast<int>(run.return_code);
      return r;
    }

    // In cmake-re mode the project is mirrored and `build` is a symlink into
    // the mirror; the source cache resolves under the mirror's project copy.
    fs::path resolve_real_source(const fs::path& project_path) {
      fs::path build = project_path / "build";
      if(fs::is_symlink(build)) {
        fs::path mirror = fs::read_symlink(build).parent_path().parent_path();
        return mirror.parent_path() / mirror.stem();
      }
      return project_path;
    }

    // Locates the samplelib clone (`samplelib-<key>-src`) in the central cache,
    // for both native cmake (under project_path) and cmake-re (under the mirror).
    fs::path find_samplelib_clone(const fs::path& project_path) {
      for(const fs::path& root : {project_path, resolve_real_source(project_path)}) {
        fs::path cache = root / "thirdparty" / "cache";
        if(!fs::exists(cache)) continue;
        for(fs::directory_iterator it(cache), end; it != end; ++it) {
          const std::string n = it->path().filename().generic_string();
          if(boost::starts_with(n, "samplelib-") && boost::ends_with(n, "-src")) {
            return it->path();
          }
        }
      }
      return {};
    }

    // True when `output` shows a git clone of the samplelib content.
    bool cloned_samplelib(const std::string& output) {
      // matches e.g. "Cloning into '/…/thirdparty/cache/samplelib-abcd1234-src'"
      static const boost::regex rx{R"(Cloning into '[^']*samplelib-[^']*')"};
      return boost::regex_search(output, rx);
    }

    // Retires the current build tree so the next configure starts from a fresh
    // one (the second-build-tree of issue #96). For cmake-re `build` is a
    // symlink into the mirror worktree: renaming it away makes cmake-re
    // re-create it against the same (uuid-stable) mirror, i.e. a warm
    // reconfigure — the strongest equivalent scenario it supports.
    void retire_build_tree(const fs::path& project_path) {
      fs::path retired;
      for(int i = 1;; ++i) {
        retired = project_path / ("build_retired_" + std::to_string(i));
        if(!fs::exists(retired)) break;
      }
      fs::rename(project_path / "build", retired);
      fs::create_directories(project_path / "build");
    }

    // Renames the dependency origin away: any later attempt to contact it
    // (clone / fetch / archive download) fails loudly instead of silently
    // re-downloading.
    fs::path retire_origin(const fs::path& origin) {
      fs::path retired = origin.parent_path() / (origin.filename().generic_string() + ".retired");
      fs::rename(origin, retired);
      return retired;
    }

  } // namespace

  // A warm, clean git clone in the central cache must let a brand new build
  // tree configure and build with the origin unreachable, without touching the
  // cached clone (single download, offline-capable — AC0 of issue #96).
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, git_second_build_tree_reuses_cache_without_origin,
                         boost::unit_test::data::make(hfc::test::test_variants()), data) {
    fs::path project_path = prepare_project_to_be_tested("check_source_cache_reuse", data.is_cmake_re, temp_dir);
    std::string uuid = make_uuid();

    fs::path origin = temp_dir / "samplelib_origin";
    write_lib_source(origin, 1);
    std::string sha = git_init_and_commit(origin, test_env, "samplelib v1");

    write_consumer(project_path,
      "  GIT_REPOSITORY \"file://" + origin.generic_string() + "\"\n"
      "  GIT_TAG \"" + sha + "\"\n");

    // first build tree: populates the cache (and must be the only download)
    auto first = configure_build_run(project_path, data, test_env, uuid);
    BOOST_REQUIRE_MESSAGE(first.configured && first.built, first.detail);
    BOOST_REQUIRE_EQUAL(first.variant_seen, 1);
    BOOST_REQUIRE_MESSAGE(cloned_samplelib(first.configure_output),
                          "expected the first configure to clone samplelib into the cache");

    fs::path clone = find_samplelib_clone(project_path);
    BOOST_REQUIRE_MESSAGE(!clone.empty(), "samplelib clone not found in the central source cache");
    std::string head_before = git_head(clone, test_env);
    BOOST_REQUIRE_EQUAL(head_before, sha);
    file_fingerprint git_head_file{clone / ".git" / "HEAD"};

    // make any re-download attempt fail loudly, then configure a fresh tree
    retire_origin(origin);
    retire_build_tree(project_path);

    auto second = configure_build_run(project_path, data, test_env, uuid);
    BOOST_REQUIRE_MESSAGE(second.configured && second.built,
                          "second build tree must not need the origin:\n" + second.detail);
    BOOST_REQUIRE_EQUAL(second.variant_seen, 1);
    BOOST_REQUIRE_MESSAGE(!cloned_samplelib(second.configure_output),
                          "second build tree re-cloned samplelib instead of reusing the cache");

    BOOST_REQUIRE_EQUAL(git_head(clone, test_env), head_before);
    BOOST_REQUIRE_MESSAGE(git_head_file.is_unchanged(),
                          "the cached clone's .git/HEAD was rewritten — the cache was re-cloned");
  }

  // Same property for URL/archive content, whose reuse rests entirely on the
  // populate stamps living next to the cache (there is no HFC-side git check).
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, url_second_build_tree_reuses_cache_without_origin,
                         boost::unit_test::data::make(hfc::test::test_variants()), data) {
    fs::path project_path = prepare_project_to_be_tested("check_source_cache_reuse", data.is_cmake_re, temp_dir);
    std::string uuid = make_uuid();

    fs::path origin_tree = temp_dir / "samplelib_tree";
    write_lib_source(origin_tree, 1);
    fs::path archive = temp_dir / "samplelib.tar.gz";
    std::string sha1 = make_archive(origin_tree, archive, test_env);

    write_consumer(project_path,
      "  URL \"file://" + archive.generic_string() + "\"\n"
      "  URL_HASH SHA1=" + sha1 + "\n");

    auto first = configure_build_run(project_path, data, test_env, uuid);
    BOOST_REQUIRE_MESSAGE(first.configured && first.built, first.detail);
    BOOST_REQUIRE_EQUAL(first.variant_seen, 1);

    fs::path clone = find_samplelib_clone(project_path);
    BOOST_REQUIRE_MESSAGE(!clone.empty(), "samplelib sources not found in the central source cache");
    file_fingerprint cached_header{clone / "samplelib.h"};

    retire_origin(archive);
    retire_build_tree(project_path);

    auto second = configure_build_run(project_path, data, test_env, uuid);
    BOOST_REQUIRE_MESSAGE(second.configured && second.built,
                          "second build tree must not need the archive origin:\n" + second.detail);
    BOOST_REQUIRE_EQUAL(second.variant_seen, 1);
    BOOST_REQUIRE_MESSAGE(cached_header.is_unchanged(),
                          "the cached extracted sources were replaced — the archive was re-downloaded/re-extracted");
  }

  // A branch GIT_TAG follows the branch without ever re-cloning the cache:
  // new build trees fetch + check out the moved branch when the origin is
  // reachable, and fall back to the local clone state when it is not.
  // Native cmake only: cmake-re's install-tree cache keys the revision on the
  // declared GIT_TAG, which does not change when a branch moves — a separate,
  // pre-existing concern from cache reuse.
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, git_branch_tag_follows_branch_without_reclone,
                         boost::unit_test::data::make(hfc::test::test_variants()), data) {
    if(data.is_cmake_re) {
      std::cout << "(skipped for cmake-re: branch moves do not change the ORIGIN+REVISION cache key)" << std::endl;
      return;
    }

    fs::path project_path = prepare_project_to_be_tested("check_source_cache_reuse", data.is_cmake_re, temp_dir);
    std::string uuid = make_uuid();

    fs::path origin = temp_dir / "samplelib_origin";
    write_lib_source(origin, 1);
    git_init_and_commit(origin, test_env, "samplelib v1");

    write_consumer(project_path,
      "  GIT_REPOSITORY \"file://" + origin.generic_string() + "\"\n"
      "  GIT_TAG \"main\"\n");

    auto first = configure_build_run(project_path, data, test_env, uuid);
    BOOST_REQUIRE_MESSAGE(first.configured && first.built, first.detail);
    BOOST_REQUIRE_EQUAL(first.variant_seen, 1);
    fs::path clone = find_samplelib_clone(project_path);
    BOOST_REQUIRE_MESSAGE(!clone.empty(), "samplelib clone not found in the central source cache");

    // fresh build tree, branch unmoved: reuse, no re-clone
    retire_build_tree(project_path);
    auto second = configure_build_run(project_path, data, test_env, uuid);
    BOOST_REQUIRE_MESSAGE(second.configured && second.built, second.detail);
    BOOST_REQUIRE_EQUAL(second.variant_seen, 1);
    BOOST_REQUIRE_MESSAGE(!cloned_samplelib(second.configure_output),
                          "unmoved branch caused a re-clone instead of reusing the cache");

    // move the branch: a fresh build tree must follow it, still without re-cloning
    write_lib_source(origin, 2);
    git_commit_all(origin, test_env, "samplelib v2");
    retire_build_tree(project_path);
    auto third = configure_build_run(project_path, data, test_env, uuid);
    BOOST_REQUIRE_MESSAGE(third.configured && third.built, third.detail);
    BOOST_REQUIRE_MESSAGE(!cloned_samplelib(third.configure_output),
                          "moved branch caused a re-clone instead of an in-place update");
    BOOST_REQUIRE_EQUAL(third.variant_seen, 2);

    // origin gone: keep working from the local clone state
    retire_origin(origin);
    retire_build_tree(project_path);
    auto fourth = configure_build_run(project_path, data, test_env, uuid);
    BOOST_REQUIRE_MESSAGE(fourth.configured && fourth.built,
                          "unreachable origin must not break a warm branch-tagged cache:\n" + fourth.detail);
    BOOST_REQUIRE_EQUAL(fourth.variant_seen, 2);
    BOOST_REQUIRE_MESSAGE(!cloned_samplelib(fourth.configure_output),
                          "unreachable origin caused a re-clone attempt");
  }

  // Local (uncommitted) modifications in the cached clone must survive a new
  // build tree and are what actually gets compiled — the debugging workflow of
  // hfc commit a56de21d. Native cmake only: cmake-re's install-tree cache is
  // keyed on ORIGIN+REVISION, which a local source edit deliberately keeps.
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, git_local_modifications_survive_new_build_tree,
                         boost::unit_test::data::make(hfc::test::test_variants()), data) {
    if(data.is_cmake_re) {
      std::cout << "(skipped for cmake-re: local source edits do not change the ORIGIN+REVISION cache key)" << std::endl;
      return;
    }

    fs::path project_path = prepare_project_to_be_tested("check_source_cache_reuse", data.is_cmake_re, temp_dir);
    std::string uuid = make_uuid();

    fs::path origin = temp_dir / "samplelib_origin";
    write_lib_source(origin, 1);
    std::string sha = git_init_and_commit(origin, test_env, "samplelib v1");

    write_consumer(project_path,
      "  GIT_REPOSITORY \"file://" + origin.generic_string() + "\"\n"
      "  GIT_TAG \"" + sha + "\"\n");

    auto first = configure_build_run(project_path, data, test_env, uuid);
    BOOST_REQUIRE_MESSAGE(first.configured && first.built, first.detail);
    BOOST_REQUIRE_EQUAL(first.variant_seen, 1);

    fs::path clone = find_samplelib_clone(project_path);
    BOOST_REQUIRE_MESSAGE(!clone.empty(), "samplelib clone not found in the central source cache");

    // local debugging edit in the cached clone (repo becomes dirty)
    pre::file::from_string((clone / "samplelib.cpp").generic_string(),
      "#include \"samplelib.h\"\nint samplelib_variant() { return 42; }\n");

    retire_build_tree(project_path);

    auto second = configure_build_run(project_path, data, test_env, uuid);
    BOOST_REQUIRE_MESSAGE(second.configured && second.built,
                          "new build tree must build with the locally modified cache:\n" + second.detail);
    BOOST_REQUIRE_MESSAGE(!cloned_samplelib(second.configure_output),
                          "the locally modified clone was re-cloned — local changes were destroyed");
    BOOST_REQUIRE_MESSAGE(boost::contains(pre::file::to_string((clone / "samplelib.cpp").generic_string()), "return 42;"),
                          "the local modification was wiped from the cached clone");
    BOOST_REQUIRE_EQUAL(second.variant_seen, 42);
  }

} // namespace hfc::test
