#define BOOST_TEST_MODULE hfc_source_migration_test
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>

#include <test_project.hpp>
#include <test_variant.hpp>
#include <test_helpers.hpp>
#include <test_isolation_fixture.hpp>

#include <pre/file/string.hpp>
#include <pre/file/hash.hpp>

#include <functional>
#include <sstream>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// HFC source-origin migration matrix
// ─────────────────────────────────────────────────────────────────────────────
//
// HFC relocates the FetchContent download into a *central* source cache and
// keys the on-disk clone directory on:
//
//     <content_name>-<first 8 chars of (GIT_TAG | URL_HASH)>-src
//
// (see FetchContent_MakeHermetic in cmake/HermeticFetchContent.cmake and
//  hfc_provide_dependency_FETCHCONTENT.cmake). The *origin* (the git repository
// URL or the archive URL) is deliberately NOT part of that key.
//
// This test reuses the same build folder + central cache and changes the
// FetchContent_Declare() origin of a single dependency ("samplelib") between two
// configure+build cycles, then runs the produced executable. The executable
// returns the dependency's "variant id" as its exit code, so we can detect
// exactly which origin's source tree was compiled in after the change:
//
//   - exit code == new variant id   -> migration handled cleanly
//   - exit code == old variant id   -> STALE: HFC reused the previous sources
//   - build/configure failure       -> migration broke the build
//
// Each scenario is fully self-contained and offline: it builds local git repos
// and local archives of a trivial CMake library on the fly.
//
// Structure: every scenario is its own focused BOOST_DATA_TEST_CASE_F,
// parameterised over the build drivers, so Boost emits one runnable case per
// (scenario, variant): "<scenario>/_0" = native cmake, "<scenario>/_1" =
// cmake-re. They share no mutable state (each uses the isolation fixture's
// test_env / per-test TIPI_HOME_DIR), so they parallelise cleanly:
//
//     tipi run ./parallel/boost-parallel ./build/test/hfc_source_migration_test
//
// NOTE: the cmake-re ("/_1") cases must be run under `tipi run` so the tipi
// toolchain (tipi-compiler-driver, build cache) is on PATH; without it the
// cmake-re dependency build cannot complete. The native ("/_0") cases need no
// such wrapper.

namespace hfc::test {
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
  using namespace std::string_literals;

  namespace {

    // We run the whole matrix once per build driver (native `cmake` and
    // `cmake-re`) so the report can compare them. cmake-re mirrors the project
    // into a per-build worktree and additionally keeps an L1 install-tree cache
    // keyed on ORIGIN+REVISION, so its reuse semantics differ from native CMake
    // reusing one build folder + one central source cache.
    std::string variant_label(const test_variant& data) {
      return data.is_cmake_re ? "cmake-re" : "cmake";
    }

    // cmake-re needs a stable UUID across the two configures of one scenario so
    // the per-build worktree (and its caches) are reused. Native cmake ignores it.
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

    // Writes a minimal, installable CMake library into `dir`.
    //   - samplelib_variant() returns `variant_id`  (-> process exit code)
    //   - origin_token is embedded in the header as a grep-able marker
    void write_lib_source(const fs::path& dir, int variant_id, const std::string& origin_token) {
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
          << "  FILE samplelibTargets.cmake)\n";  // *Targets.cmake so HFC's target discovery picks it up
      pre::file::from_string((dir / "CMakeLists.txt").generic_string(), cml.str());

      std::stringstream hdr;
      hdr << "#pragma once\n"
          << "// HFC-MIGRATION-ORIGIN-TOKEN: " << origin_token << "\n"
          << "int samplelib_variant();\n";
      pre::file::from_string((dir / "samplelib.h").generic_string(), hdr.str());

      std::stringstream src;
      src << "#include \"samplelib.h\"\n"
          << "int samplelib_variant() { return " << variant_id << "; }\n";
      pre::file::from_string((dir / "samplelib.cpp").generic_string(), src.str());
    }

    // Hand-written, dependency-free autotools scaffold (a fake ./configure +
    // Makefile.rules) committed under test_project_templates. HFC's autotools
    // path only runs `./configure --prefix=... CC=... && make && make install`, so
    // this fake has full fidelity for what we test without needing autoconf.
    fs::path autotools_scaffold_dir() {
      return get_source_tree_dir() / "test" / "test_project_templates" / "autotools_samplelib";
    }

    // Writes a minimal, installable *autotools* library into `dir`: the fake
    // scaffold + a per-variant samplelib.c/.h. HFC builds it with
    // `./configure && make && make install` (HERMETIC_BUILD_SYSTEM autotools).
    void write_autotools_lib_source(const fs::path& dir, int variant_id, const std::string& origin_token) {
      fs::create_directories(dir);
      // copy the ready-made autotools scaffold (configure, Makefile.in, aux ...)
      for(const auto& entry : fs::directory_iterator(autotools_scaffold_dir())) {
        fs::copy(entry.path(), dir / entry.path().filename(),
                 fs::copy_options::overwrite_existing | fs::copy_options::recursive);
      }

      std::stringstream hdr;
      hdr << "#ifndef SAMPLELIB_H\n#define SAMPLELIB_H\n"
          << "/* HFC-MIGRATION-ORIGIN-TOKEN: " << origin_token << " */\n"
          << "#ifdef __cplusplus\nextern \"C\" {\n#endif\n"
          << "int samplelib_variant(void);\n"
          << "#ifdef __cplusplus\n}\n#endif\n#endif\n";
      pre::file::from_string((dir / "samplelib.h").generic_string(), hdr.str());

      std::stringstream src;
      src << "#include \"samplelib.h\"\n"
          << "int samplelib_variant(void) { return " << variant_id << "; }\n";
      pre::file::from_string((dir / "samplelib.c").generic_string(), src.str());
    }

    // Initialises `dir` as a git repo and commits its current contents on `branch`.
    // Returns the resulting commit SHA.
    std::string git_init_and_commit(const fs::path& dir, bp::environment& env, const std::string& message) {
      run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " init -q");
      run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " add -A");
      run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " commit -q -m \"" + message + "\"");
      // Force the branch name to "main" regardless of git's default: git < 2.28
      // (e.g. the CI runner's 2.25) ignores `init.defaultBranch=main` and inits
      // `master`, which would break the moving-branch scenario's GIT_TAG "main".
      run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " branch -M main");
      auto rev = run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " rev-parse HEAD");
      boost::trim(rev.output);
      return rev.output;
    }

    // Commits the current contents of an existing repo as a new commit on the
    // current branch (used to "move a branch forward"). Returns new commit SHA.
    std::string git_commit_all(const fs::path& dir, bp::environment& env, const std::string& message) {
      run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " add -A");
      run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " commit -q -m \"" + message + "\"");
      auto rev = run_cmd(env, bp::start_dir=(dir), bp::shell, git_with_identity() + " rev-parse HEAD");
      boost::trim(rev.output);
      return rev.output;
    }

    // Builds a .tar.gz of `src_dir` (as a single top-level directory, which
    // FetchContent strips on extraction) at `archive_path`. Returns its SHA1.
    std::string make_archive(const fs::path& src_dir, const fs::path& archive_path, bp::environment& env) {
      fs::path cmake_bin = bp::search_path("cmake");
      fs::path parent = src_dir.parent_path();
      std::string dirname = src_dir.filename().generic_string();
      // tar from the parent so the archive contains "<dirname>/..." at the root
      run_cmd(env, bp::start_dir=(parent), bp::shell,
        cmake_bin.generic_string() + " -E tar czf " + archive_path.generic_string() + " " + dirname);
      return pre::file::sha1sum(archive_path.generic_string());
    }

    // Origin description for a single FetchContent_Declare() under test.
    struct origin_t {
      int variant_id = 0;
      std::string token;          // human label, also embedded in the header
      std::string declare_body;   // the GIT_REPOSITORY/GIT_TAG or URL/URL_HASH lines
      bool autotools = false;     // build system of the dependency (cmake vs autotools)
    };

    origin_t git_origin(int variant_id, const std::string& token, const fs::path& repo, const std::string& tag,
                        bool autotools = false) {
      std::stringstream body;
      body << "  GIT_REPOSITORY \"" << ("file://" + repo.generic_string()) << "\"\n"
           << "  GIT_TAG \"" << tag << "\"\n";
      return origin_t{variant_id, token, body.str(), autotools};
    }

    origin_t url_origin(int variant_id, const std::string& token, const fs::path& archive, const std::string& sha1,
                        bool autotools = false) {
      std::stringstream body;
      body << "  URL \"" << ("file://" + archive.generic_string()) << "\"\n"
           << "  URL_HASH SHA1=" << sha1 << "\n";
      return origin_t{variant_id, token, body.str(), autotools};
    }

    // Writes the consumer project's CMakeLists.txt for a given origin.
    void write_consumer(const fs::path& project_path, const origin_t& origin) {
      std::stringstream cml;
      cml << "set(FETCHCONTENT_QUIET OFF CACHE BOOL \"\" FORCE)\n"
          << "cmake_minimum_required(VERSION 3.27.6)\n"
          << "project(HfcSourceMigrationTest VERSION 1.0 LANGUAGES CXX)\n\n"
          << "set(CMAKE_MODULE_PATH\n"
          << "  \"${CMAKE_CURRENT_SOURCE_DIR}/cmake\"\n"
          << "  \"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules\"\n"
          << "  ${CMAKE_MODULE_PATH})\n"
          << "include(HermeticFetchContent)\n\n"
          << "FetchContent_Declare(\n"
          << "  samplelib\n"
          << origin.declare_body
          << ")\n\n";

      if(origin.autotools) {
        // autotools deps don't emit CMake target files, so we declare the
        // imported target via HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION (the
        // same mechanism the iconv sample uses).
        cml << "FetchContent_MakeHermetic(\n"
            << "  samplelib\n"
            << "  HERMETIC_BUILD_SYSTEM autotools\n"
            << "  HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION\n"
            << "    [=[\n"
            << "      add_library(SampleLib::samplelib STATIC IMPORTED)\n"
            << "      set_property(TARGET SampleLib::samplelib PROPERTY IMPORTED_LOCATION \"@HFC_PREFIX_PLACEHOLDER@/lib/libsamplelib.a\")\n"
            << "      set_property(TARGET SampleLib::samplelib PROPERTY INTERFACE_INCLUDE_DIRECTORIES @HFC_PREFIX_PLACEHOLDER@/include)\n"
            << "    ]=])\n\n";
      } else {
        cml << "FetchContent_MakeHermetic(\n"
            << "  samplelib\n"
            << "  HERMETIC_BUILD_SYSTEM cmake)\n\n";
      }

      cml << "HermeticFetchContent_MakeAvailableAtConfigureTime(samplelib)\n\n"
          << "add_executable(MyExample simple_example.cpp)\n"
          << "target_link_libraries(MyExample PRIVATE SampleLib::samplelib)\n";
      pre::file::from_string((project_path / "CMakeLists.txt").generic_string(), cml.str());
    }

    struct step_result {
      bool configured = false;
      bool built = false;
      bool ran = false;
      int  variant_seen = -1;   // exit code of MyExample
      std::string detail;
      std::string configure_output;  // kept so we can locate the source-cache dir
    };

    // Configure + build the consumer in its (reused) build folder, then run the
    // produced executable and capture its exit code. `uuid` keeps the cmake-re
    // worktree stable across the two configures of one scenario.
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
      r.ran = true;
      r.variant_seen = static_cast<int>(run.return_code);
      return r;
    }

    // Locate the central source-cache clone dir for samplelib by scanning the
    // configure output (HFC prints it as the `-S <dir>` of the isolated dep
    // configure, e.g. ".../thirdparty/cache/samplelib-<key>-src"). Works for
    // both native cmake and cmake-re because it reports the real on-disk path.
    fs::path extract_samplelib_src_dir(const std::string& output) {
      boost::regex rx{R"((/[^ '\"\n]*/samplelib-[A-Za-z0-9._]+-src))"};
      auto begin = boost::sregex_iterator(output.begin(), output.end(), rx);
      auto end = boost::sregex_iterator();
      fs::path any_existing;
      for(auto it = begin; it != end; ++it) {
        fs::path p((*it)[1].str());
        if(fs::exists(p / "CMakeLists.txt")) {
          return p;  // the populated source tree
        }
        if(any_existing.empty() && fs::exists(p)) {
          any_existing = p;
        }
      }
      return any_existing;
    }

    // In cmake-re mode the project is mirrored and `build` is a symlink into the
    // mirror; a project-relative path (the source cache) resolves under the
    // mirror's project copy, not under `project_path`. Recover that real source
    // dir from the symlink (native cmake: it's just project_path).
    fs::path resolve_real_source(const fs::path& project_path) {
      fs::path build = project_path / "build";
      if(fs::is_symlink(build)) {
        fs::path mirror = fs::read_symlink(build).parent_path().parent_path();
        return mirror.parent_path() / mirror.stem();
      }
      return project_path;
    }

    // Locate the samplelib source-cache clone on disk (the `<name>-<key>-src`
    // dir under thirdparty/cache). Robust across native cmake (under
    // project_path) and cmake-re (under the mirror copy) -- unlike scraping the
    // configure output, which misses the mirror path and varies by environment.
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

    // One migration scenario: build with `from`, then reconfigure+rebuild the
    // same folder with `to`, and check which sources end up compiled in.
    struct scenario_outcome {
      std::string name;
      bool initial_ok = false;
      step_result after;
      bool migrated_cleanly = false;   // running binary reflects `to`
      bool served_stale = false;       // running binary reflects `from`
    };

    scenario_outcome run_scenario(const std::string& name, const origin_t& from, const origin_t& to,
                                  const test_variant& data, const fs::path& base_dir, const std::string& uuid,
                                  bp::environment& env) {
      scenario_outcome out;
      out.name = name;

      // Each scenario gets an isolated project root so central caches do not
      // cross-contaminate between scenarios.
      fs::path scenario_base = base_dir / name;
      fs::create_directories(scenario_base);
      fs::path project_path = prepare_project_to_be_tested("hfc_source_migration", data.is_cmake_re, scenario_base);
      write_project_tipi_id(project_path);

      std::cout << "\n========== scenario: " << name << " ==========" << std::endl;

      // 1. Build with the initial ("from") origin.
      write_consumer(project_path, from);
      step_result first = configure_build_run(project_path, data, env, uuid);
      out.initial_ok = first.ran && first.variant_seen == from.variant_id;
      if(!out.initial_ok) {
        std::cout << "[" << name << "] initial build with '" << from.token
                  << "' did not yield variant " << from.variant_id
                  << " (configured=" << first.configured << " built=" << first.built
                  << " ran=" << first.ran << " seen=" << first.variant_seen << ")\n"
                  << first.detail << std::endl;
        return out;  // can't meaningfully test migration if baseline is broken
      }
      std::cout << "[" << name << "] baseline OK: '" << from.token << "' -> variant " << first.variant_seen << std::endl;

      // 2. Switch the origin to "to" and reconfigure/rebuild the SAME folder.
      write_consumer(project_path, to);
      out.after = configure_build_run(project_path, data, env, uuid);

      if(out.after.ran && out.after.variant_seen == to.variant_id) {
        out.migrated_cleanly = true;
      } else if(out.after.ran && out.after.variant_seen == from.variant_id) {
        out.served_stale = true;
      }

      std::cout << "[" << name << "] after switch to '" << to.token << "': "
                << "configured=" << out.after.configured
                << " built=" << out.after.built
                << " ran=" << out.after.ran
                << " variant_seen=" << out.after.variant_seen
                << " (expected " << to.variant_id << ", old was " << from.variant_id << ")" << std::endl;
      if(!out.after.detail.empty()) {
        std::cout << out.after.detail << std::endl;
      }

      return out;
    }

    std::string outcome_label(const scenario_outcome& o) {
      if(!o.initial_ok)        return "BASELINE-FAILED";
      if(o.migrated_cleanly)   return "CLEAN";
      if(o.served_stale)       return "STALE (old sources reused)";
      if(!o.after.configured)  return "BROKEN (configure error)";
      if(!o.after.built)       return "BROKEN (build error)";
      if(!o.after.ran)         return "BROKEN (no executable)";
      return "UNEXPECTED (variant " + std::to_string(o.after.variant_seen) + ")";
    }

    // Coarse outcome class used by the exact-expectation assertions. STALE means
    // the binary ran but reflected the OLD sources; BROKEN covers every failure
    // mode (configure/build/run error or an unexpected variant).
    enum class outcome_kind { clean, stale, broken };

    const char* kind_name(outcome_kind k) {
      switch(k) {
        case outcome_kind::clean:  return "CLEAN";
        case outcome_kind::stale:  return "STALE";
        case outcome_kind::broken: return "BROKEN";
      }
      return "?";
    }

    outcome_kind classify(const scenario_outcome& o) {
      if(o.migrated_cleanly) return outcome_kind::clean;
      if(o.served_stale)     return outcome_kind::stale;
      return outcome_kind::broken;
    }

    // The offline source material shared by every scenario: two git repos
    // (repoA has two commits / a movable "main"), and two archives. Variant ids
    // double as the executable exit code so we can tell the origins apart.
    struct source_material {
      fs::path repoA, repoB, arcX, arcY;
      std::string repoA_v1, repoA_v2, repoB_v1, arcX_sha, arcY_sha;
    };

    // Builds the source material with either a cmake or an autotools sample lib.
    // The git-repo / archive layout (and thus HFC's source-cache keying) is
    // identical for both build systems, so the same migration scenarios apply.
    source_material build_source_material(const fs::path& root, bp::environment& env, bool autotools = false) {
      source_material m;
      fs::create_directories(root);

      auto write_lib = [&](const fs::path& dir, int variant, const std::string& token) {
        if(autotools) write_autotools_lib_source(dir, variant, token);
        else          write_lib_source(dir, variant, token);
      };

      m.repoA = root / "repoA";
      write_lib(m.repoA, 11, "repoA@v1");
      m.repoA_v1 = git_init_and_commit(m.repoA, env, "repoA v1 (variant 11)");
      write_lib(m.repoA, 12, "repoA@v2");
      m.repoA_v2 = git_commit_all(m.repoA, env, "repoA v2 (variant 12)");

      m.repoB = root / "repoB";
      write_lib(m.repoB, 21, "repoB@v1");
      m.repoB_v1 = git_init_and_commit(m.repoB, env, "repoB v1 (variant 21)");

      fs::path arcX_src = root / "arcX-src" / "samplelib";
      write_lib(arcX_src, 31, "archiveX");
      m.arcX = root / "archiveX.tar.gz";
      m.arcX_sha = make_archive(arcX_src, m.arcX, env);

      fs::path arcY_src = root / "arcY-src" / "samplelib";
      write_lib(arcY_src, 41, "archiveY");
      m.arcY = root / "archiveY.tar.gz";
      m.arcY_sha = make_archive(arcY_src, m.arcY, env);
      return m;
    }

    // Drives one "should migrate cleanly" scenario end to end: build the offline
    // material (cmake or autotools lib), run from->to in a reused folder, then
    // assert CLEAN. A different revision string yields a different source-cache
    // key, so these are pinned with a hard CHECK on both build drivers.
    void do_clean_migration(const test_variant& data, const fs::path& temp_dir, bp::environment& env,
                            const std::string& name, bool autotools,
                            const std::function<origin_t(const source_material&)>& mk_from,
                            const std::function<origin_t(const source_material&)>& mk_to) {
      source_material m = build_source_material(temp_dir / "sources", env, autotools);
      auto o = run_scenario(name, mk_from(m), mk_to(m), data, temp_dir, make_uuid(), env);

      const std::string lbl = "[" + variant_label(data) + "] " + o.name;
      std::cout << lbl << " -> " << outcome_label(o) << std::endl;
      BOOST_REQUIRE_MESSAGE(o.initial_ok, lbl << " baseline build failed: " << o.after.detail);
      BOOST_CHECK_MESSAGE(o.migrated_cleanly, lbl << " expected CLEAN, got: " << outcome_label(o));
    }

    // Removing the .git folder from the shared source clone between builds.
    // `fresh_build_folder` distinguishes "same build folder reused" (stamp files
    // survive, populate is skipped) from "a brand new run reusing the shared
    // cache" (no stamps -> FetchContent tries to git-clone into a non-empty,
    // non-git dir). The second is the reported breakage.
    scenario_outcome run_dotgit_scenario(const std::string& name, bool fresh_build_folder,
                                         const test_variant& data, const fs::path& temp_dir,
                                         const source_material& m, bp::environment& env, bool autotools = false) {
      scenario_outcome out;
      out.name = name;
      std::string uuid = make_uuid();

      fs::path scenario_base = temp_dir / name;
      fs::create_directories(scenario_base);
      fs::path project_path = prepare_project_to_be_tested("hfc_source_migration", data.is_cmake_re, scenario_base);
      write_project_tipi_id(project_path);

      origin_t git_v1 = git_origin(11, "repoA@" + m.repoA_v1.substr(0,8), m.repoA, m.repoA_v1, autotools);
      write_consumer(project_path, git_v1);
      step_result first = configure_build_run(project_path, data, env, uuid);
      out.initial_ok = first.ran && first.variant_seen == 11;

      fs::path src_dir = find_samplelib_clone(project_path);
      bool removed_git = false;
      if(!src_dir.empty() && fs::exists(src_dir / ".git")) {
        fs::remove_all(src_dir / ".git");
        removed_git = fs::exists(src_dir) && !fs::exists(src_dir / ".git");
      }
      // The whole point of these scenarios is to exercise a clone whose .git was
      // removed; fail loudly if we could not set that precondition up rather than
      // silently degrading to a plain same-origin rebuild.
      BOOST_REQUIRE_MESSAGE(removed_git,
        "[" << variant_label(data) << "] " << name << " could not remove the samplelib clone's .git (clone="
        << (src_dir.empty() ? "<not found>" : src_dir.generic_string()) << ")");

      if(fresh_build_folder) {
        // Drop the build folder (and thus the FetchContent stamp files) while
        // the central thirdparty/cache (the shared clone) survives.
        fs::remove_all(project_path / "build");
        fs::create_directories(project_path / "build");
      }

      std::cout << "[" << variant_label(data) << "] " << name
                << " | baseline variant " << first.variant_seen
                << " | src=" << (src_dir.empty() ? "<not found>" : src_dir.generic_string())
                << " | .git removed=" << removed_git
                << " | fresh build folder=" << fresh_build_folder << std::endl;

      write_consumer(project_path, git_v1);
      out.after = configure_build_run(project_path, data, env, uuid);
      if(out.after.ran && out.after.variant_seen == 11) out.migrated_cleanly = true;
      return out;
    }

    // A branch (GIT_TAG = "main") whose tip advances between two builds of the
    // same folder. The revision *string* ("main") is unchanged, so the source
    // cache key does not change and HFC reuses the old checkout.
    scenario_outcome run_moving_branch_scenario(const test_variant& data, const fs::path& temp_dir,
                                                const source_material& m, bp::environment& env, bool autotools = false) {
      scenario_outcome out;
      out.name = autotools ? "at_git_moving_branch_same_tag" : "git_moving_branch_same_tag";
      std::string uuid = make_uuid();
      fs::path scenario_base = temp_dir / out.name;
      fs::create_directories(scenario_base);
      fs::path project_path = prepare_project_to_be_tested("hfc_source_migration", data.is_cmake_re, scenario_base);
      write_project_tipi_id(project_path);

      // Baseline: main == v1 (variant 11).
      run_cmd(env, bp::start_dir=(m.repoA), bp::shell, git_with_identity() + " reset -q --hard " + m.repoA_v1);
      write_consumer(project_path, git_origin(11, "repoA@main(v1)", m.repoA, "main", autotools));
      step_result first = configure_build_run(project_path, data, env, uuid);
      out.initial_ok = first.ran && first.variant_seen == 11;

      // Move main forward to v2, rebuild the same folder with identical GIT_TAG.
      run_cmd(env, bp::start_dir=(m.repoA), bp::shell, git_with_identity() + " reset -q --hard " + m.repoA_v2);
      write_consumer(project_path, git_origin(12, "repoA@main(v2)", m.repoA, "main", autotools));
      out.after = configure_build_run(project_path, data, env, uuid);
      if(out.after.ran && out.after.variant_seen == 12) out.migrated_cleanly = true;
      else if(out.after.ran && out.after.variant_seen == 11) out.served_stale = true;
      return out;
    }
  }


  // Each scenario below is its OWN focused test case, parameterised over the
  // build drivers via BOOST_DATA_TEST_CASE_F. Boost generates one runnable case
  // per (scenario, variant) - e.g. "git_tag_to_git_tag/_0" (cmake) and
  // "git_tag_to_git_tag/_1" (cmake-re) - so `parallel/boost-parallel` can run
  // every variant of every scenario concurrently as separate processes. This is
  // what lets the matrix scale: add a scenario or a variant, get more parallel
  // units, not a longer single test.
  #define HFC_MIGRATION_VARIANTS boost::unit_test::data::make(hfc::test::test_variants())

  // ─── Clean migrations ──────────────────────────────────────────────────────
  // A different revision string (git commit / archive hash) yields a different
  // source-cache key, so HFC fetches the new origin into a fresh clone dir.
  // Each migration is exercised for BOTH dependency build systems: a cmake
  // sample lib (HERMETIC_BUILD_SYSTEM cmake) and an autotools one
  // (./configure + make, consumed via HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION).
  // `at_*` cases are the autotools mirror of the cmake case above them.

  // Migration-origin builders (the only thing that varies between the cases).
  namespace mig {
    auto from_repoA_v1 = [](bool at){ return [at](const source_material& m){ return git_origin(11, "repoA@" + m.repoA_v1.substr(0,8), m.repoA, m.repoA_v1, at); }; };
    auto to_repoA_v2   = [](bool at){ return [at](const source_material& m){ return git_origin(12, "repoA@" + m.repoA_v2.substr(0,8), m.repoA, m.repoA_v2, at); }; };
    auto to_repoB_v1   = [](bool at){ return [at](const source_material& m){ return git_origin(21, "repoB@" + m.repoB_v1.substr(0,8), m.repoB, m.repoB_v1, at); }; };
    auto archiveX      = [](bool at){ return [at](const source_material& m){ return url_origin(31, "archiveX", m.arcX, m.arcX_sha, at); }; };
    auto archiveY      = [](bool at){ return [at](const source_material& m){ return url_origin(41, "archiveY", m.arcY, m.arcY_sha, at); }; };
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, git_tag_to_git_tag, HFC_MIGRATION_VARIANTS, data) {
    do_clean_migration(data, temp_dir, test_env, "git_tag_to_git_tag", false, mig::from_repoA_v1(false), mig::to_repoA_v2(false));
  }
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, at_git_tag_to_git_tag, HFC_MIGRATION_VARIANTS, data) {
    do_clean_migration(data, temp_dir, test_env, "at_git_tag_to_git_tag", true,
                       mig::from_repoA_v1(true), mig::to_repoA_v2(true));
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, git_repo_to_git_repo, HFC_MIGRATION_VARIANTS, data) {
    do_clean_migration(data, temp_dir, test_env, "git_repo_to_git_repo", false, mig::from_repoA_v1(false), mig::to_repoB_v1(false));
  }
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, at_git_repo_to_git_repo, HFC_MIGRATION_VARIANTS, data) {
    do_clean_migration(data, temp_dir, test_env, "at_git_repo_to_git_repo", true, mig::from_repoA_v1(true), mig::to_repoB_v1(true));
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, git_to_archive, HFC_MIGRATION_VARIANTS, data) {
    do_clean_migration(data, temp_dir, test_env, "git_to_archive", false, mig::from_repoA_v1(false), mig::archiveX(false));
  }
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, at_git_to_archive, HFC_MIGRATION_VARIANTS, data) {
    do_clean_migration(data, temp_dir, test_env, "at_git_to_archive", true, mig::from_repoA_v1(true), mig::archiveX(true));
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, archive_to_git, HFC_MIGRATION_VARIANTS, data) {
    do_clean_migration(data, temp_dir, test_env, "archive_to_git", false, mig::archiveX(false), mig::from_repoA_v1(false));
  }
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, at_archive_to_git, HFC_MIGRATION_VARIANTS, data) {
    do_clean_migration(data, temp_dir, test_env, "at_archive_to_git", true, mig::archiveX(true), mig::from_repoA_v1(true));
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, archive_to_archive, HFC_MIGRATION_VARIANTS, data) {
    do_clean_migration(data, temp_dir, test_env, "archive_to_archive", false, mig::archiveX(false), mig::archiveY(false));
  }
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, at_archive_to_archive, HFC_MIGRATION_VARIANTS, data) {
    do_clean_migration(data, temp_dir, test_env, "at_archive_to_archive", true, mig::archiveX(true), mig::archiveY(true));
  }

  // ─── Same-cache-key edge cases ──────────────────────────────────────────────
  // These scenarios keep the SAME source-cache key across both builds (the
  // revision string is unchanged, or the .git dir is removed in place). Rather
  // than silently warning, each case asserts the EXACT outcome HFC produces today
  // so the whole matrix stays on a positive expectation: the suite never silently
  // passes a broken case, a regression that changes the outcome trips the check,
  // and if HFC starts handling a limitation the exact-match fails and tells us to
  // tighten it to CLEAN. The expected outcomes are calibrated against real runs:
  //   moving branch (same GIT_TAG "main")      -> STALE  (cache key unchanged)
  //   .git removed, build folder reused        -> CLEAN  (stamps survive)
  //   .git removed, fresh build folder         -> CLEAN cmake-re / BROKEN native
  //                                               cmake (both cmake and autotools)
  //
  // TODO: the non-CLEAN expectations below encode real HFC issues, not desired
  // behaviour. When either is fixed, flip its expectation to CLEAN (the exact
  // match will fail until you do, by design):
  //   1. STALE  - moving_branch_same_tag: a GIT_TAG that is a mutable branch
  //      reuses the stale clone because the source-cache key is the GIT_TAG
  //      string. Fix would resolve the ref to a concrete commit before keying.
  //   2. BROKEN - {git,at_git}_dotgit_removed_fresh_build (native cmake):
  //      re-populating a shared clone whose .git was removed fails (clone into a
  //      non-empty, non-git dir). cmake-re recovers through its mirror.

  // Assert a scenario produced exactly `expected`. Any deviation fails the case,
  // including an *improvement* to CLEAN (so the expectation is kept honest).
  static void expect_outcome(const scenario_outcome& o, const test_variant& data,
                             outcome_kind expected, const std::string& note) {
    const std::string lbl = "[" + variant_label(data) + "] " + o.name;
    std::cout << lbl << " -> " << outcome_label(o) << std::endl;
    BOOST_REQUIRE_MESSAGE(o.initial_ok, lbl << " baseline failed: " << o.after.detail);
    const outcome_kind actual = classify(o);
    BOOST_CHECK_MESSAGE(actual == expected,
      lbl << " expected " << kind_name(expected) << " (" << note << "), got " << outcome_label(o)
          << (actual == outcome_kind::clean
                ? " -- HFC now handles this case; tighten the expectation to CLEAN" : ""));
  }

  // A branch (GIT_TAG = "main") whose tip advances between builds.
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, git_moving_branch_same_tag, HFC_MIGRATION_VARIANTS, data) {
    source_material m = build_source_material(temp_dir / "sources", test_env);
    auto o = run_moving_branch_scenario(data, temp_dir, m, test_env, false);
    expect_outcome(o, data, outcome_kind::stale,
      "same GIT_TAG \"main\" => same source cache key; moved branch tip not picked up");
  }
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, at_git_moving_branch_same_tag, HFC_MIGRATION_VARIANTS, data) {
    source_material m = build_source_material(temp_dir / "sources", test_env, true);
    auto o = run_moving_branch_scenario(data, temp_dir, m, test_env, true);
    expect_outcome(o, data, outcome_kind::stale,
      "autotools; same GIT_TAG \"main\" => same source cache key");
  }

  // .git removed from the shared clone, same build folder reused: the stamp
  // files survive so HFC keeps the leftover working tree and rebuilds cleanly.
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, git_dotgit_removed_same_build, HFC_MIGRATION_VARIANTS, data) {
    source_material m = build_source_material(temp_dir / "sources", test_env);
    auto o = run_dotgit_scenario("git_dotgit_removed_same_build", false, data, temp_dir, m, test_env, false);
    expect_outcome(o, data, outcome_kind::clean, ".git removed, build folder reused");
  }
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, at_git_dotgit_removed_same_build, HFC_MIGRATION_VARIANTS, data) {
    source_material m = build_source_material(temp_dir / "sources", test_env, true);
    auto o = run_dotgit_scenario("at_git_dotgit_removed_same_build", false, data, temp_dir, m, test_env, true);
    expect_outcome(o, data, outcome_kind::clean, "autotools; .git removed, build folder reused");
  }

  // .git removed from the shared clone, fresh build folder: no stamp files, so
  // FetchContent re-populates from scratch. For native cmake this fails —
  // FetchContent tries to git-clone into the non-empty, non-git shared clone dir
  // and the configure step errors. cmake-re re-populates through its mirror and
  // recovers, so the native-cmake case is BROKEN in both build systems.
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, git_dotgit_removed_fresh_build, HFC_MIGRATION_VARIANTS, data) {
    source_material m = build_source_material(temp_dir / "sources", test_env);
    auto o = run_dotgit_scenario("git_dotgit_removed_fresh_build", true, data, temp_dir, m, test_env, false);
    expect_outcome(o, data, data.is_cmake_re ? outcome_kind::clean : outcome_kind::broken,
      ".git deleted from shared clone + fresh build folder => native-cmake re-population fails");
  }
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, at_git_dotgit_removed_fresh_build, HFC_MIGRATION_VARIANTS, data) {
    source_material m = build_source_material(temp_dir / "sources", test_env, true);
    auto o = run_dotgit_scenario("at_git_dotgit_removed_fresh_build", true, data, temp_dir, m, test_env, true);
    expect_outcome(o, data, data.is_cmake_re ? outcome_kind::clean : outcome_kind::broken,
      "autotools; .git deleted from shared clone + fresh build folder => native-cmake re-population fails");
  }
}
