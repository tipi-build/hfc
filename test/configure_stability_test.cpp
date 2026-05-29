#define BOOST_TEST_MODULE configure_stability_test
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>

#include <test_project.hpp>
#include <test_variant.hpp>

#include <pre/file/string.hpp>
#include <pre/file/hash.hpp>

#include <test_helpers.hpp>
#include <test_isolation_fixture.hpp>


namespace hfc::test {
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
  namespace utf = boost::unit_test;
  using namespace std::string_literals;


  inline size_t count_files_starts_and_ends_with(const fs::path& path, const std::string& prefix_str, const std::string& suffix_str) {
    size_t result = 0;

    for (fs::directory_entry& dir_entry : fs::directory_iterator(path)) {

      if(fs::is_regular_file(dir_entry)) {
        auto filename = dir_entry.path().filename().generic_string();

        if(boost::starts_with(filename, prefix_str) && boost::ends_with(filename, suffix_str)) {
          result++;
        }
      }
    }

    std::cout << "Found " << result << " starting with '" << prefix_str << "' and ending with '" << suffix_str << "' files in folder: " << path << std::endl;

    return result;
  }

  inline size_t count_configure_done_files(const fs::path& path) {
    return count_files_starts_and_ends_with(path, "hfc.", ".configure.done");
  }

  inline size_t count_install_done_files(const fs::path& path) {   
    return count_files_starts_and_ends_with(path, "hfc.", ".install.done");
  }


  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, configure_stability_autotools_dependency_AtConfigureTime, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path project_path = prepare_project_to_be_tested("configure_stability_autotools_dependency_AtConfigureTime", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, { "lib.h" } /* includes */, "simple_example.cpp" /* destination */);

    std::string cmake_configure_command = get_cmake_configure_command(project_path, data);
    std::string cmake_build_command = get_cmake_build_command(project_path, data);

    /* check we don't have leftovers... */
    {
      BOOST_REQUIRE(is_empty_directory(project_path / "build"));
    }

    auto iconv_build_dir = project_path / "build" / "_deps" / "iconv-build" / "src";
    auto iconv_install_dir = project_path / "build" / "_deps" / "Iconv-install";

    // configure twice
    std::cout << "⚗️ [Configure 1]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    /* checks */
    {
      //
      // the Iconv lib is a "AtConfigureTime" library, so it should be fully configured by now
      //

      // these files are configure artifacts and should be present
      BOOST_REQUIRE(fs::exists(iconv_build_dir / "Makefile"));    
      BOOST_REQUIRE(fs::exists(iconv_build_dir / "config.log"));
      BOOST_REQUIRE(fs::exists(iconv_build_dir / "config.status"));

      // we should have in build time artifacts
      BOOST_REQUIRE(fs::exists(iconv_build_dir / ".libs" / "libiconv.a"));
      BOOST_REQUIRE(fs::exists(iconv_build_dir / ".libs" / "libiconv.la"));
      BOOST_REQUIRE(fs::exists(iconv_build_dir / "hello"));

      // we should have installed
      BOOST_REQUIRE(fs::exists(iconv_install_dir / "lib" / "libiconv.a"));
      BOOST_REQUIRE(fs::exists(iconv_install_dir / "lib" / "libiconv.la"));
      BOOST_REQUIRE(fs::exists(iconv_install_dir / "include" / "lib.h"));
      BOOST_REQUIRE(fs::exists(iconv_install_dir / "bin" / "hello"));

      BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
      BOOST_REQUIRE(count_install_done_files(iconv_install_dir) == 1);

      //
      // the project binary should not be there just yet
      //
      BOOST_REQUIRE(!fs::exists(project_path / "build" / "MyExample" ));     
    }

    // gather some info to check that the configure didn't reexecute
    auto path_iconv_config_log = iconv_build_dir / "config.log";
    auto path_iconv_config_status = iconv_build_dir / "config.status";
    auto path_iconv_makefile = iconv_build_dir / "Makefile";
    auto path_iconv_installed_libiconv = iconv_install_dir / "lib" / "libiconv.a";


    file_fingerprint ffingerprint_c1_config_log{path_iconv_config_log};
    file_fingerprint ffingerprint_c1_config_status{path_iconv_config_status};
    file_fingerprint ffingerprint_c1_makefile{path_iconv_makefile};
    file_fingerprint ffingerprint_c1_installed_libiconv{path_iconv_installed_libiconv};

    std::cout << "⚗️ [Configure 2 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libiconv.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
    BOOST_REQUIRE(count_install_done_files(iconv_install_dir) == 1);

    // build
    std::cout << "⚗️ [Build 1]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libiconv.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
    BOOST_REQUIRE(count_install_done_files(iconv_install_dir) == 1);
   
    auto path_project_binary = project_path / "build" / "MyExample";
    BOOST_REQUIRE(fs::exists(path_project_binary));

    //
    // configure and build once more
    // expectation: no input was modified, so everything shall stay as-is
    //

    file_fingerprint ffingerprint_b1_project_binary{path_project_binary};
    std::cout << "👷 [Configure 3 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);
    std::cout << "👷 [Build 2]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libiconv.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_project_binary.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
    BOOST_REQUIRE(count_install_done_files(iconv_install_dir) == 1);

    //
    // force rebuilding by changing the main file content
    // expectation: configured library remains unchanged
    //

    write_simple_main(project_path, { "lib.h", "iostream" } /* includes */, "simple_example.cpp" /* destination */);

    std::cout << "👷 [Build 3]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libiconv.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_project_binary.has_changed());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
    BOOST_REQUIRE(count_install_done_files(iconv_install_dir) == 1);

    file_fingerprint ffingerprint_b3_project_binary{path_project_binary};

    //
    // now, modify the toolchain file to trigger a full rebuild
    //

    fs::path project_toolchain = get_project_toolchain_path(project_path);
    file_fingerprint toolchain_fingerprint{project_toolchain};

    append_to_toolchain(project_toolchain, "\nadd_compile_definitions(\"HFC_CHANGE=abc\")");
    BOOST_REQUIRE(toolchain_fingerprint.has_changed());

    std::cout << "👷 [Configure 4 / toolchain changed]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_config_log.has_changed());
    BOOST_REQUIRE(ffingerprint_c1_config_status.has_changed());
    BOOST_REQUIRE(ffingerprint_c1_makefile.has_changed());
    BOOST_REQUIRE(ffingerprint_c1_installed_libiconv.has_changed());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
    BOOST_REQUIRE(count_install_done_files(iconv_install_dir) == 1);

    // note: cmake-re will mirror to a different location so this will be changed rightfully in that case
    if(data.is_cmake_re) {
      BOOST_REQUIRE(!fs::exists(path_project_binary));
    } else {
      BOOST_REQUIRE(ffingerprint_b3_project_binary.is_unchanged()); // !! not built yet !!
    }

    file_fingerprint ffingerprint_c4_config_log{path_iconv_config_log};
    file_fingerprint ffingerprint_c4_config_status{path_iconv_config_status};
    file_fingerprint ffingerprint_c4_makefile{path_iconv_makefile};
    file_fingerprint ffingerprint_b4_installed_libiconv{path_iconv_installed_libiconv};

    // build
    std::cout << "👷 [Build 4 / toolchain changed]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);


    BOOST_REQUIRE(ffingerprint_c4_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_installed_libiconv.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b3_project_binary.has_changed());  // now it should be "different" (works in dependently of is_cmake_re bc. the path stored in not resolved)
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
    BOOST_REQUIRE(count_install_done_files(iconv_install_dir) == 1);

    file_fingerprint ffingerprint_b4_project_binary{path_project_binary};

    std::cout << "👷 [Configure 5 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);
    std::cout << "👷 [Build 5]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c4_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_installed_libiconv.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
    BOOST_REQUIRE(count_install_done_files(iconv_install_dir) == 1);

    //
    // change the dependency config and expect a rebuild
    //

    fs::path project_cmakelists_path = project_path / "CMakeLists.txt";
    file_fingerprint ffingerprint_c5_cmakelists{project_cmakelists_path};
    fs::copy(project_path / "CMakeLists.config_mod.txt", project_cmakelists_path, fs::copy_options::overwrite_existing);
    file_fingerprint ffingerprint_c6_cmakelists{project_cmakelists_path};
    BOOST_REQUIRE(ffingerprint_c5_cmakelists.content_hash != ffingerprint_c6_cmakelists.content_hash);  // explicitely check the contents differ!

    std::cout << "👷 [Configure 6 / project CmakeLists.txt changed]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c4_config_log.has_changed());
    BOOST_REQUIRE(ffingerprint_c4_config_status.has_changed());
    BOOST_REQUIRE(ffingerprint_c4_makefile.has_changed());
    BOOST_REQUIRE(ffingerprint_b4_installed_libiconv.has_changed());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
    BOOST_REQUIRE(count_install_done_files(iconv_install_dir) == 1);

    file_fingerprint ffingerprint_c5_config_log{path_iconv_config_log};
    file_fingerprint ffingerprint_c5_config_status{path_iconv_config_status};
    file_fingerprint ffingerprint_c5_makefile{path_iconv_makefile};
    file_fingerprint ffingerprint_c5_installed_libiconv{path_iconv_installed_libiconv};

    std::cout << "👷 [Build 6 / project CmakeLists.txt changed]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c5_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_installed_libiconv.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.has_changed());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
    BOOST_REQUIRE(count_install_done_files(iconv_install_dir) == 1);  

  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, configure_stability_autotools_dependency_AtBuildTime, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path project_path = prepare_project_to_be_tested("configure_stability_autotools_dependency_AtBuildTime", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, { "lib.h" } /* includes */, "simple_example.cpp" /* destination */);

    std::string cmake_configure_command = get_cmake_configure_command(project_path, data);
    std::string cmake_build_command = get_cmake_build_command(project_path, data);

    /* check we don't have leftovers... */
    {
      BOOST_REQUIRE(is_empty_directory(project_path / "build"));
    }

    auto iconv_build_dir = project_path / "build" / "_deps" / "iconv-build" / "src";
    auto iconv_install_dir = project_path / "build" / "_deps" / "Iconv-install";

    // configure twice
    std::cout << "⚗️ [Configure 1]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    /* checks */
    {
      //
      // the Iconv lib is a "AtBuildTime" library, so it should be only configured not built
      //

      // these files are configure artifacts and should be present
      BOOST_REQUIRE(fs::exists(iconv_build_dir / "Makefile"));    
      BOOST_REQUIRE(fs::exists(iconv_build_dir / "config.log"));
      BOOST_REQUIRE(fs::exists(iconv_build_dir / "config.status"));
      BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);

      // we should NOT have build time artifacts
      BOOST_REQUIRE(!fs::exists(iconv_build_dir / ".libs" / "libiconv.a"));
      BOOST_REQUIRE(!fs::exists(iconv_build_dir / ".libs" / "libiconv.la"));
      BOOST_REQUIRE(!fs::exists(iconv_build_dir / "hello"));

      // nothing should be installed
      BOOST_REQUIRE(!fs::exists(iconv_install_dir / "bin"));   
      BOOST_REQUIRE(!fs::exists(iconv_install_dir / "lib"));   
      BOOST_REQUIRE(is_empty_directory(iconv_install_dir / "include")); // exists for technical purposes but shall be empty

      //
      // the project binary should not be there just yet
      //
      BOOST_REQUIRE(!fs::exists(project_path / "build" / "MyExample" ));     
    }

    // gather some info to check that the configure didn't reexecute
    auto path_iconv_config_log = iconv_build_dir / "config.log";
    auto path_iconv_config_status = iconv_build_dir / "config.status";
    auto path_iconv_makefile = iconv_build_dir / "Makefile";
    auto path_iconv_installed_libiconv = iconv_install_dir / "lib" / "libiconv.a";


    file_fingerprint ffingerprint_c1_config_log{path_iconv_config_log};
    file_fingerprint ffingerprint_c1_config_status{path_iconv_config_status};
    file_fingerprint ffingerprint_c1_makefile{path_iconv_makefile};

    std::cout << "⚗️ [Configure 2 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_makefile.is_unchanged());
    BOOST_REQUIRE(!fs::exists(path_iconv_installed_libiconv));
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);

    // build
    std::cout << "⚗️ [Build 1]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_makefile.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
   
    auto path_project_binary = project_path / "build" / "MyExample";
    BOOST_REQUIRE(fs::exists(path_project_binary));
   
    BOOST_REQUIRE(fs::exists(path_iconv_installed_libiconv));
    file_fingerprint ffingerprint_b1_installed_libiconv{path_iconv_installed_libiconv};

    //
    // configure and build once more
    // expectation: no input was modified, so everything shall stay as-is
    //

    file_fingerprint ffingerprint_b1_project_binary{path_project_binary};
    std::cout << "👷 [Configure 3 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);
    std::cout << "👷 [Build 2]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_installed_libiconv.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_project_binary.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);

    //
    // force rebuilding by changing the main file content
    // expectation: configured library remains unchanged
    //

    write_simple_main(project_path, { "lib.h", "iostream" } /* includes */, "simple_example.cpp" /* destination */);

    std::cout << "👷 [Build 3]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_installed_libiconv.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_project_binary.has_changed());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);

    file_fingerprint ffingerprint_b3_project_binary{path_project_binary};

    //
    // now, modify the toolchain file to trigger a full rebuild
    //

    fs::path project_toolchain = get_project_toolchain_path(project_path);
    file_fingerprint toolchain_fingerprint{project_toolchain};

    append_to_toolchain(project_toolchain, "\nadd_compile_definitions(\"HFC_CHANGE=abc\")");
    BOOST_REQUIRE(toolchain_fingerprint.has_changed());

    std::cout << "👷 [Configure 4 / toolchain changed]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_config_log.has_changed());
    BOOST_REQUIRE(ffingerprint_c1_config_status.has_changed());
    BOOST_REQUIRE(ffingerprint_c1_makefile.has_changed());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);

    // note: cmake-re will mirror to a different location so this will be changed rightfully in that case
    if(data.is_cmake_re) {
      BOOST_REQUIRE(!fs::exists(path_project_binary));
    } else {
      BOOST_REQUIRE(ffingerprint_b3_project_binary.is_unchanged()); // !! not built yet !!
    }

    file_fingerprint ffingerprint_c4_config_log{path_iconv_config_log};
    file_fingerprint ffingerprint_c4_config_status{path_iconv_config_status};
    file_fingerprint ffingerprint_c4_makefile{path_iconv_makefile};
    // build
    std::cout << "👷 [Build 4 / toolchain changed]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c4_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b3_project_binary.has_changed());  // now it should be "different" (works in dependently of is_cmake_re bc. the path stored in not resolved)
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);

    file_fingerprint ffingerprint_b4_installed_libiconv{path_iconv_installed_libiconv};
    file_fingerprint ffingerprint_b4_project_binary{path_project_binary};

    std::cout << "👷 [Configure 5 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);
    std::cout << "👷 [Build 5]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c4_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_installed_libiconv.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);

    //
    // change the dependency config and expect a rebuild
    //

    fs::path project_cmakelists_path = project_path / "CMakeLists.txt";
    file_fingerprint ffingerprint_c5_cmakelists{project_cmakelists_path};
    fs::copy(project_path / "CMakeLists.config_mod.txt", project_cmakelists_path, fs::copy_options::overwrite_existing);
    file_fingerprint ffingerprint_c6_cmakelists{project_cmakelists_path};
    BOOST_REQUIRE(ffingerprint_c5_cmakelists.content_hash != ffingerprint_c6_cmakelists.content_hash);  // explicitely check the contents differ!

    std::cout << "👷 [Configure 6 / project CmakeLists.txt changed]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c4_config_log.has_changed());
    BOOST_REQUIRE(ffingerprint_c4_config_status.has_changed());
    BOOST_REQUIRE(ffingerprint_c4_makefile.has_changed());
    BOOST_REQUIRE(ffingerprint_b4_installed_libiconv.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);

    file_fingerprint ffingerprint_c5_config_log{path_iconv_config_log};
    file_fingerprint ffingerprint_c5_config_status{path_iconv_config_status};
    file_fingerprint ffingerprint_c5_makefile{path_iconv_makefile};

    std::cout << "👷 [Build 6 / project CmakeLists.txt changed]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c5_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_installed_libiconv.has_changed());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.has_changed());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
  }


  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, configure_stability_cmake_dependency_AtConfigureTime, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path project_path = prepare_project_to_be_tested("configure_stability_cmake_dependency_AtConfigureTime", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, { "MathFunctions.h" } /* includes */, "simple_example.cpp" /* destination */);

    std::string cmake_configure_command = get_cmake_configure_command(project_path, data);
    std::string cmake_build_command = get_cmake_build_command(project_path, data);

    /* check we don't have leftovers... */
    {
      BOOST_REQUIRE(is_empty_directory(project_path / "build"));
    }

    auto mathlib_build_dir = project_path / "build" / "_deps" / "mathlib-build";
    auto mathlib_install_dir = project_path / "build" / "_deps" / "mathlib-install";

    // configure twice
    std::cout << "⚗️ [Configure 1]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    /* checks */
    {
      //
      // the mathlib is a "AtConfigureTime" library, so it should be fully configured and installed by now
      //

      // these files are configure artifacts and should be present
      BOOST_REQUIRE(fs::exists(mathlib_build_dir / "build.ninja"));    
      BOOST_REQUIRE(fs::exists(mathlib_build_dir / "CMakeCache.txt"));
      BOOST_REQUIRE(fs::exists(mathlib_build_dir / "CMakeFiles" / "CMakeConfigureLog.yaml"));

      // we should have in build time artifacts
      BOOST_REQUIRE(fs::exists(mathlib_build_dir / "libMathFunctions.a"));
      BOOST_REQUIRE(fs::exists(mathlib_build_dir / "libMathFunctionscbrt.a"));

      // we should have installed
      BOOST_REQUIRE(fs::exists(mathlib_install_dir / "lib" / "libMathFunctions.a"));
      BOOST_REQUIRE(fs::exists(mathlib_install_dir / "lib" / "libMathFunctionscbrt.a"));
      BOOST_REQUIRE(fs::exists(mathlib_install_dir / "include" / "MathFunctions.h"));
      BOOST_REQUIRE(fs::exists(mathlib_install_dir / "include" / "MathFunctionscbrt.h"));

      BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

      //
      // the project binary should not be there just yet
      //
      BOOST_REQUIRE(!fs::exists(project_path / "build" / "MyExample" ));     
    }

    // gather some info to check that the configure didn't reexecute
    auto path_mathlib_CMakeConfigureLog = mathlib_build_dir / "CMakeFiles" / "CMakeConfigureLog.yaml";
    auto path_mathlib_ninjafile = mathlib_build_dir / "build.ninja";
    auto path_mathlib_installed_libMathFunctions = mathlib_install_dir / "lib" / "libMathFunctions.a";
    auto path_mathlib_installed_libMathFunctionscbrt = mathlib_install_dir / "lib" / "libMathFunctionscbrt.a";

    file_fingerprint ffingerprint_c1_CMakeConfigureLog{path_mathlib_CMakeConfigureLog};
    file_fingerprint ffingerprint_c1_path_mathlib_ninjafile{path_mathlib_ninjafile};
    file_fingerprint ffingerprint_c1_installed_libMathFunctions{path_mathlib_installed_libMathFunctions};
    file_fingerprint ffingerprint_c1_installed_libMathFunctionscbrt{path_mathlib_installed_libMathFunctionscbrt};

    std::cout << "⚗️ [Configure 2 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_path_mathlib_ninjafile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libMathFunctionscbrt.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    // build
    std::cout << "⚗️ [Build 1]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_path_mathlib_ninjafile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libMathFunctionscbrt.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);
   
    auto path_project_binary = project_path / "build" / "MyExample";
    BOOST_REQUIRE(fs::exists(path_project_binary));

    //
    // configure and build once more
    // expectation: no input was modified, so everything shall stay as-is
    //

    file_fingerprint ffingerprint_b1_project_binary{path_project_binary};
    std::cout << "👷 [Configure 3 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);
    std::cout << "👷 [Build 2]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_path_mathlib_ninjafile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libMathFunctionscbrt.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_project_binary.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    //
    // force rebuilding by changing the main file content
    // expectation: configured library remains unchanged
    //

    write_simple_main(project_path, { "MathFunctions.h", "iostream" } /* includes */, "simple_example.cpp" /* destination */);

    std::cout << "👷 [Build 3]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_path_mathlib_ninjafile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libMathFunctionscbrt.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_project_binary.has_changed());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    file_fingerprint ffingerprint_b3_project_binary{path_project_binary};

    //
    // now, modify the toolchain file to trigger a full rebuild
    //

    fs::path project_toolchain = get_project_toolchain_path(project_path);
    file_fingerprint toolchain_fingerprint{project_toolchain};

    append_to_toolchain(project_toolchain, "\nadd_compile_definitions(\"HFC_CHANGE=abc\")");
    BOOST_REQUIRE(toolchain_fingerprint.has_changed());

    std::cout << "👷 [Configure 4 / toolchain changed]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.has_changed());
    BOOST_REQUIRE(ffingerprint_c1_path_mathlib_ninjafile.has_changed());
    BOOST_REQUIRE(ffingerprint_c1_installed_libMathFunctionscbrt.has_changed());
    BOOST_REQUIRE(ffingerprint_c1_installed_libMathFunctions.has_changed());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    // note: cmake-re will mirror to a different location so this will be changed rightfully in that case
    if(data.is_cmake_re) {
      BOOST_REQUIRE(!fs::exists(path_project_binary));
    } else {
      BOOST_REQUIRE(ffingerprint_b3_project_binary.is_unchanged()); // !! not built yet !!
    }

    file_fingerprint ffingerprint_c4_config_log{path_mathlib_CMakeConfigureLog};
    file_fingerprint ffingerprint_c4_config_status{path_mathlib_ninjafile};
    file_fingerprint ffingerprint_c4_installed_libMathFunctionscbrt{path_mathlib_installed_libMathFunctionscbrt};
    file_fingerprint ffingerprint_b4_installed_libMathFunctions{path_mathlib_installed_libMathFunctions};

    // build
    std::cout << "👷 [Build 4 / toolchain changed]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);


    BOOST_REQUIRE(ffingerprint_c4_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_installed_libMathFunctionscbrt.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b3_project_binary.has_changed());  // now it should be "different" (works in dependently of is_cmake_re bc. the path stored in not resolved)
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    file_fingerprint ffingerprint_b4_project_binary{path_project_binary};

    std::cout << "👷 [Configure 5 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);
    std::cout << "👷 [Build 5]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c4_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_installed_libMathFunctionscbrt.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    //
    // change the dependency config and expect a rebuild
    //

    fs::path project_cmakelists_path = project_path / "CMakeLists.txt";
    file_fingerprint ffingerprint_c5_cmakelists{project_cmakelists_path};
    fs::copy(project_path / "CMakeLists.config_mod.txt", project_cmakelists_path, fs::copy_options::overwrite_existing);
    file_fingerprint ffingerprint_c6_cmakelists{project_cmakelists_path};
    BOOST_REQUIRE(ffingerprint_c5_cmakelists.content_hash != ffingerprint_c6_cmakelists.content_hash);  // explicitely check the contents differ!

    std::cout << "👷 [Configure 6 / project CmakeLists.txt changed]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c4_config_log.has_changed());
    BOOST_REQUIRE(ffingerprint_c4_config_status.has_changed());
    BOOST_REQUIRE(ffingerprint_c4_installed_libMathFunctionscbrt.has_changed());
    BOOST_REQUIRE(ffingerprint_b4_installed_libMathFunctions.has_changed());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    file_fingerprint ffingerprint_c5_config_log{path_mathlib_CMakeConfigureLog};
    file_fingerprint ffingerprint_c5_installed_libMathFunctions{path_mathlib_installed_libMathFunctions};
    file_fingerprint ffingerprint_c5_libMathFunctionscbrt{path_mathlib_installed_libMathFunctionscbrt};

    std::cout << "👷 [Build 6 / project CmakeLists.txt changed]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c5_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_libMathFunctionscbrt.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.has_changed());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);
  }

  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, configure_stability_cmake_dependency_AtBuildTime, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path project_path = prepare_project_to_be_tested("configure_stability_cmake_dependency_AtBuildTime", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, { "MathFunctions.h" } /* includes */, "simple_example.cpp" /* destination */);

    std::string cmake_configure_command = get_cmake_configure_command(project_path, data);
    std::string cmake_build_command = get_cmake_build_command(project_path, data);

    /* check we don't have leftovers... */
    {
      BOOST_REQUIRE(is_empty_directory(project_path / "build"));
    }

    auto mathlib_build_dir = project_path / "build" / "_deps" / "mathlib-build";
    auto mathlib_install_dir = project_path / "build" / "_deps" / "mathlib-install";

    // configure twice
    std::cout << "⚗️ [Configure 1]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    /* checks */
    {
      //
      // the Iconv lib is a "AtBuildTime" library, so it should be only configured not built
      //

      // these files are configure artifacts and should be present
      BOOST_REQUIRE(fs::exists(mathlib_build_dir / "build.ninja"));    
      BOOST_REQUIRE(fs::exists(mathlib_build_dir / "CMakeCache.txt"));
      BOOST_REQUIRE(fs::exists(mathlib_build_dir / "CMakeFiles" / "CMakeConfigureLog.yaml"));
      BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

      // we should have in build time artifacts
      BOOST_REQUIRE(!fs::exists(mathlib_build_dir / "libMathFunctions.a"));
      BOOST_REQUIRE(!fs::exists(mathlib_build_dir / "libMathFunctionscbrt.a"));

      // nothing should be installed
      BOOST_REQUIRE(!fs::exists(mathlib_install_dir / "bin"));   
      BOOST_REQUIRE(!fs::exists(mathlib_install_dir / "lib"));   
      BOOST_REQUIRE(is_empty_directory(mathlib_install_dir / "include")); // exists for technical purposes but shall be empty

      //
      // the project binary should not be there just yet
      //
      BOOST_REQUIRE(!fs::exists(project_path / "build" / "MyExample" ));     
    }

    // gather some info to check that the configure didn't reexecute
    auto path_mathlib_CMakeConfigureLog = mathlib_build_dir / "CMakeFiles" / "CMakeConfigureLog.yaml";
    auto path_mathlib_ninjafile = mathlib_build_dir / "build.ninja";
    auto path_mathlib_installed_libMathFunctions = mathlib_install_dir / "lib" / "libMathFunctions.a";
    auto path_mathlib_installed_libMathFunctionscbrt = mathlib_install_dir / "lib" / "libMathFunctionscbrt.a";

    file_fingerprint ffingerprint_c1_CMakeConfigureLog{path_mathlib_CMakeConfigureLog};
    file_fingerprint ffingerprint_c1_path_mathlib_ninjafile{path_mathlib_ninjafile};

    std::cout << "⚗️ [Configure 2 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_path_mathlib_ninjafile.is_unchanged());
    BOOST_REQUIRE(!fs::exists(path_mathlib_installed_libMathFunctions));
    BOOST_REQUIRE(!fs::exists(path_mathlib_installed_libMathFunctionscbrt));
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    // build
    std::cout << "⚗️ [Build 1]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_path_mathlib_ninjafile.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);
   
    auto path_project_binary = project_path / "build" / "MyExample";
    BOOST_REQUIRE(fs::exists(path_project_binary));
   
    BOOST_REQUIRE(fs::exists(path_mathlib_installed_libMathFunctions));
    BOOST_REQUIRE(fs::exists(path_mathlib_installed_libMathFunctionscbrt));
    file_fingerprint ffingerprint_b1_installed_libMathFunctions{path_mathlib_installed_libMathFunctions};
    file_fingerprint ffingerprint_b1_installed_libMathFunctionscbrt{path_mathlib_installed_libMathFunctionscbrt};

    //
    // configure and build once more
    // expectation: no input was modified, so everything shall stay as-is
    //

    file_fingerprint ffingerprint_b1_project_binary{path_project_binary};
    std::cout << "👷 [Configure 3 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);
    std::cout << "👷 [Build 2]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_path_mathlib_ninjafile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_installed_libMathFunctionscbrt.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_project_binary.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    //
    // force rebuilding by changing the main file content
    // expectation: configured library remains unchanged
    //

    write_simple_main(project_path, { "MathFunctions.h", "iostream" } /* includes */, "simple_example.cpp" /* destination */);

    std::cout << "👷 [Build 3]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_path_mathlib_ninjafile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_project_binary.has_changed());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    file_fingerprint ffingerprint_b3_project_binary{path_project_binary};

    //
    // now, modify the toolchain file to trigger a full rebuild
    //

    fs::path project_toolchain = get_project_toolchain_path(project_path);
    file_fingerprint toolchain_fingerprint{project_toolchain};

    append_to_toolchain(project_toolchain, "\nadd_compile_definitions(\"HFC_CHANGE=abcd\")");
    BOOST_REQUIRE(toolchain_fingerprint.has_changed());

    std::cout << "👷 [Configure 4 / toolchain changed]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.has_changed());
    BOOST_REQUIRE(ffingerprint_c1_path_mathlib_ninjafile.has_changed());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    // note: cmake-re will mirror to a different location so this will be changed rightfully in that case
    if(data.is_cmake_re) {
      BOOST_REQUIRE(!fs::exists(path_project_binary));
    } else {
      BOOST_REQUIRE(ffingerprint_b3_project_binary.is_unchanged()); // !! not built yet !!
    }

    file_fingerprint ffingerprint_c4_config_log{path_mathlib_CMakeConfigureLog};

    // build
    std::cout << "👷 [Build 4 / toolchain changed]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c4_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b3_project_binary.has_changed());  // now it should be "different" (works in dependently of is_cmake_re bc. the path stored in not resolved)
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    file_fingerprint ffingerprint_b4_installed_libMathFunctions{path_mathlib_installed_libMathFunctions};
    file_fingerprint ffingerprint_b4_installed_libMathFunctionscbrt{path_mathlib_installed_libMathFunctionscbrt};
    file_fingerprint ffingerprint_b4_project_binary{path_project_binary};

    std::cout << "👷 [Configure 5 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);
    std::cout << "👷 [Build 5]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c4_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_installed_libMathFunctionscbrt.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    //
    // change the dependency config and expect a rebuild
    //

    fs::path project_cmakelists_path = project_path / "CMakeLists.txt";
    file_fingerprint ffingerprint_c5_cmakelists{project_cmakelists_path};
    fs::copy(project_path / "CMakeLists.config_mod.txt", project_cmakelists_path, fs::copy_options::overwrite_existing);
    file_fingerprint ffingerprint_c6_cmakelists{project_cmakelists_path};
    BOOST_REQUIRE(ffingerprint_c5_cmakelists.content_hash != ffingerprint_c6_cmakelists.content_hash);  // explicitely check the contents differ!

    std::cout << "👷 [Configure 6 / project CmakeLists.txt changed]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c4_config_log.has_changed());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);
    BOOST_REQUIRE(ffingerprint_b4_project_binary.is_unchanged());

    file_fingerprint ffingerprint_c5_config_log{path_mathlib_CMakeConfigureLog};

    std::cout << "👷 [Build 6 / project CmakeLists.txt changed]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c5_config_log.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    BOOST_REQUIRE(ffingerprint_b4_installed_libMathFunctions.has_changed());
    BOOST_REQUIRE(ffingerprint_b4_installed_libMathFunctionscbrt.has_changed());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.has_changed());
   
  }


  // Returns two test_variant instances for the hidden-input test:
  //  _0 → AtBuildTime  (default, no extra -D flags)
  //  _1 → AtConfigureTime (-DHFC_BUILD_AT_CONFIGURE_TIME=ON)
  inline std::vector<test_variant> cmake_hidden_input_variants() {
    auto cmake_path = bp::search_path("cmake");
    if(cmake_path.string().empty()) {
      throw std::runtime_error("cmake not found on PATH");
    }
    return {
      test_variant{ cmake_path, false },                                                     // AtBuildTime
      test_variant{ cmake_path, false, "", "-DHFC_BUILD_AT_CONFIGURE_TIME=ON" },             // AtConfigureTime
    };
  }

  // Tests that a "hidden input" read inside the toolchain (e.g. the SHA256 of
  // a sanitizer ignorelist file) is captured by the isolated toolchain
  // fingerprint and causes dependencies to be rebuilt when that file changes.
  // Exercised for both AtBuildTime and AtConfigureTime using the same template
  // switched via -DHFC_BUILD_AT_CONFIGURE_TIME.
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, configure_stability_cmake_dependency_toolchain_hidden_input,
      boost::unit_test::data::make(hfc::test::cmake_hidden_input_variants()), data){

    fs::path project_path = prepare_project_to_be_tested("configure_stability_cmake_dependency_toolchain_hidden_input", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, { "MathFunctions.h" } /* includes */, "simple_example.cpp" /* destination */);

    // create the hidden input file that the toolchain will hash
    fs::path hidden_input_file = project_path / "toolchain" / "hidden_input.txt";
    pre::file::from_string(hidden_input_file.generic_string(), "initial content v1");

    fs::path toolchain_with_hash = get_project_toolchain_path(project_path, "linux-toolchain-file-hash.cmake");
    std::string cmake_configure_command = get_cmake_configure_command(project_path, data, "", toolchain_with_hash);
    std::string cmake_build_command     = get_cmake_build_command(project_path, data);

    BOOST_REQUIRE(is_empty_directory(project_path / "build"));

    auto mathlib_build_dir   = project_path / "build" / "_deps" / "mathlib-build";
    auto mathlib_install_dir = project_path / "build" / "_deps" / "mathlib-install";

    auto path_mathlib_CMakeConfigureLog = mathlib_build_dir / "CMakeFiles" / "CMakeConfigureLog.yaml";
    auto path_mathlib_ninjafile         = mathlib_build_dir / "build.ninja";

    // helper: extract the toolchain fingerprint hash from HFC's status output
    // format: "-- [HERMETIC_FETCHCONTENT]  - toolchain fingerprint: <hex>"
    auto extract_toolchain_fingerprint = [](const std::string& cmake_output) -> std::string {
      boost::smatch m;
      if(boost::regex_search(cmake_output, m, boost::regex{" - toolchain fingerprint: ([0-9a-f]+)"})) {
        return m[1];
      }
      return "";
    };

    // Configure 1
    std::cout << "⚗️ [Configure 1]" << std::endl;
    std::string output_c1 = run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(fs::exists(path_mathlib_CMakeConfigureLog));
    BOOST_REQUIRE(fs::exists(path_mathlib_ninjafile));
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    std::string fingerprint_c1 = extract_toolchain_fingerprint(output_c1);
    BOOST_REQUIRE_MESSAGE(!fingerprint_c1.empty(), "toolchain fingerprint not found in configure output");

    // Build 1
    std::cout << "⚗️ [Build 1]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(fs::exists(mathlib_install_dir / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(mathlib_install_dir / "lib" / "libMathFunctionscbrt.a"));
    BOOST_REQUIRE(fs::exists(project_path / "build" / "MyExample"));

    file_fingerprint ffingerprint_c1_CMakeConfigureLog{path_mathlib_CMakeConfigureLog};
    file_fingerprint ffingerprint_c1_ninjafile{path_mathlib_ninjafile};
    file_fingerprint ffingerprint_b1_installed_libMathFunctions{mathlib_install_dir / "lib" / "libMathFunctions.a"};
    file_fingerprint ffingerprint_b1_project_binary{project_path / "build" / "MyExample"};

    // Configure 2 / no changes
    std::cout << "⚗️ [Configure 2 / no changes]" << std::endl;
    std::string output_c2 = run_command(cmake_configure_command, project_path, test_env);

    std::string fingerprint_c2 = extract_toolchain_fingerprint(output_c2);
    BOOST_REQUIRE_MESSAGE(!fingerprint_c2.empty(), "toolchain fingerprint not found in configure output");
    BOOST_REQUIRE_MESSAGE(fingerprint_c1 == fingerprint_c2,
      "toolchain fingerprint changed without any input change: " << fingerprint_c1 << " -> " << fingerprint_c2);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_ninjafile.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    // Build 2 / no changes
    std::cout << "⚗️ [Build 2 / no changes]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b1_project_binary.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    // Change hidden input
    // Simulates e.g. updating a sanitizer ignorelist: the toolchain reads the
    // file and adds its SHA256 as a compile definition, so the fingerprint of
    // the toolchain changes and the dependency must be fully reconfigured.
    file_fingerprint hidden_input_fingerprint_before{hidden_input_file};
    pre::file::from_string(hidden_input_file.generic_string(), "modified content v2 - triggers rebuild");
    BOOST_REQUIRE(hidden_input_fingerprint_before.has_changed());

    // Configure 3 / hidden input changed
    std::cout << "👷 [Configure 3 / hidden input changed]" << std::endl;
    std::string output_c3 = run_command(cmake_configure_command, project_path, test_env);

    // verify the fingerprint itself changed - this is the direct evidence that
    // the hidden file input is what drove the rebuild, not some unrelated cause
    std::string fingerprint_c3 = extract_toolchain_fingerprint(output_c3);
    BOOST_REQUIRE_MESSAGE(!fingerprint_c3.empty(), "toolchain fingerprint not found in configure output");
    BOOST_REQUIRE_MESSAGE(fingerprint_c1 != fingerprint_c3,
      "toolchain fingerprint did not change after hidden input modification: " << fingerprint_c3);

    // the changed fingerprint must have triggered a full dependency reconfigure
    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.has_changed());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    file_fingerprint ffingerprint_c3_CMakeConfigureLog{path_mathlib_CMakeConfigureLog};

    // Build 3 / hidden input changed
    std::cout << "👷 [Build 3 / hidden input changed]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(ffingerprint_b1_project_binary.has_changed());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    file_fingerprint ffingerprint_b3_installed_libMathFunctions{mathlib_install_dir / "lib" / "libMathFunctions.a"};
    file_fingerprint ffingerprint_b3_project_binary{project_path / "build" / "MyExample"};

    // Configure 4 + Build 4 / no changes
    // everything must be stable now that the hidden input has settled
    std::cout << "👷 [Configure 4 / no changes]" << std::endl;
    std::string output_c4 = run_command(cmake_configure_command, project_path, test_env);
    std::cout << "👷 [Build 4 / no changes]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    std::string fingerprint_c4 = extract_toolchain_fingerprint(output_c4);
    BOOST_REQUIRE_MESSAGE(!fingerprint_c4.empty(), "toolchain fingerprint not found in configure output");
    BOOST_REQUIRE_MESSAGE(fingerprint_c3 == fingerprint_c4,
      "toolchain fingerprint changed between consecutive no-change configures: " << fingerprint_c3 << " -> " << fingerprint_c4);

    BOOST_REQUIRE(ffingerprint_c3_CMakeConfigureLog.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b3_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b3_project_binary.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);
  }

  // Verifies that changing the parent toolchain causes a transitive proxy
  // toolchain invalidation through a HERMETIC_FIND_PACKAGES dependency chain:
  //
  //  parent toolchain changes
  //    -> HfcDependencyProvidedLib proxy toolchain changes
  //         (live_toolchain_fingerprint baked into HERMETIC_FETCHCONTENT_ROOT_PROJECT_TOOLCHAIN_FINGERPRINT)
  //      -> HFC_HfcDependencyProvidedLib_FINGERPRINT changes
  //        -> mathlib proxy toolchain changes
  //             (HERMETIC_FETCHCONTENT_DEPENDENCY_FINGERPRINTS entry updated)
  //          -> mathlib is reconfigured
  //
  // Uses the check_find_package template which already has the two-dependency
  // structure: HfcDependencyProvidedLib (autotools) consumed by mathlib via
  // HERMETIC_FIND_PACKAGES.
  BOOST_FIXTURE_TEST_CASE(configure_stability_transitive_invalidation_via_find_packages, test_isolation_fixture) {
    auto cmake_path = bp::search_path("cmake");
    if(cmake_path.string().empty()) {
      throw std::runtime_error("cmake not found on PATH");
    }
    test_variant data{cmake_path, false};

    fs::path project_path = prepare_project_to_be_tested("check_find_package", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, { "MathFunctions.h", "MathFunctionscbrt.h", "lib.h" });

    std::string cmake_configure_command = get_cmake_configure_command(project_path, data);

    auto hfc_dep_proxy  = project_path / "build" / "_deps" / "HfcDependencyProvidedLib-toolchain" / "hfc_hermetic_proxy_toolchain.cmake";
    auto mathlib_proxy  = project_path / "build" / "_deps" / "mathlib-toolchain"                 / "hfc_hermetic_proxy_toolchain.cmake";
    auto mathlib_build_dir       = project_path / "build" / "_deps" / "mathlib-build";
    auto path_mathlib_config_log = mathlib_build_dir / "CMakeFiles" / "CMakeConfigureLog.yaml";

    // Configure 1
    std::cout << "⚗️ [Configure 1]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(fs::exists(hfc_dep_proxy));
    BOOST_REQUIRE(fs::exists(mathlib_proxy));
    BOOST_REQUIRE(fs::exists(path_mathlib_config_log));
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    auto extract_dep_fingerprint = [&](const fs::path& proxy) -> std::string {
      std::string content = pre::file::to_string(proxy.generic_string());
      boost::smatch m;
      if(boost::regex_search(content, m, boost::regex{"HfcDependencyProvidedLib=([0-9a-fA-F.]+)"})) {
        return m[1].str();
      }
      return {};
    };

    // mathlib's proxy must actually carry the dependency fingerprint entry. The
    // change-detection assertions below also pass via ROOT_PROJECT_TOOLCHAIN_FINGERPRINT,
    // so this is the only assertion that fails if DEPENDENCY_FINGERPRINTS is emitted empty.
    std::string dep_fingerprint_before = extract_dep_fingerprint(mathlib_proxy);
    BOOST_REQUIRE_MESSAGE(!dep_fingerprint_before.empty(),
      "mathlib proxy toolchain missing HfcDependencyProvidedLib=<hash> in HERMETIC_FETCHCONTENT_DEPENDENCY_FINGERPRINTS");

    file_fingerprint fp_hfc_dep_proxy{hfc_dep_proxy};
    file_fingerprint fp_mathlib_proxy{mathlib_proxy};
    file_fingerprint fp_mathlib_config_log{path_mathlib_config_log};

    // Configure 2 / no changes
    std::cout << "⚗️ [Configure 2 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE_MESSAGE(fp_hfc_dep_proxy.is_unchanged(),
      "HfcDependencyProvidedLib proxy toolchain changed without any input change");
    BOOST_REQUIRE_MESSAGE(fp_mathlib_proxy.is_unchanged(),
      "mathlib proxy toolchain changed without any input change");
    BOOST_REQUIRE(fp_mathlib_config_log.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    // Modify parent toolchain
    // Appending to the toolchain changes the live_toolchain_fingerprint baked
    // into HfcDependencyProvidedLib's proxy toolchain, which changes its SHA256
    // hash, which changes HFC_HfcDependencyProvidedLib_FINGERPRINT, which
    // appears in mathlib's HERMETIC_FETCHCONTENT_DEPENDENCY_FINGERPRINTS.
    fs::path project_toolchain = get_project_toolchain_path(project_path);
    append_to_toolchain(project_toolchain, "\nadd_compile_definitions(\"HFC_TRANSITIVE_CHANGE=1\")");

    // Configure 3 / toolchain changed
    std::cout << "⚗️ [Configure 3 / toolchain changed]" << std::endl;
    run_command(cmake_configure_command, project_path, test_env);

    // First hop: HfcDependencyProvidedLib's proxy toolchain must reflect the
    // new live_toolchain_fingerprint
    BOOST_REQUIRE_MESSAGE(fp_hfc_dep_proxy.has_changed(),
      "HfcDependencyProvidedLib proxy toolchain did not change after parent toolchain modification");

    // Second hop: mathlib's proxy toolchain must reflect the updated
    // HFC_HfcDependencyProvidedLib_FINGERPRINT via DEPENDENCY_FINGERPRINTS
    BOOST_REQUIRE_MESSAGE(fp_mathlib_proxy.has_changed(),
      "mathlib proxy toolchain did not change -- HERMETIC_FETCHCONTENT_DEPENDENCY_FINGERPRINTS transitive invalidation failed");

    std::string dep_fingerprint_after = extract_dep_fingerprint(mathlib_proxy);
    BOOST_REQUIRE_MESSAGE(!dep_fingerprint_after.empty(),
      "mathlib proxy toolchain missing HfcDependencyProvidedLib=<hash> after toolchain change");
    BOOST_REQUIRE_MESSAGE(dep_fingerprint_after != dep_fingerprint_before,
      "HfcDependencyProvidedLib fingerprint in mathlib's DEPENDENCY_FINGERPRINTS did not change "
      "after the dependency was invalidated");

    // Consequence: mathlib was reconfigured because its proxy toolchain changed
    BOOST_REQUIRE_MESSAGE(fp_mathlib_config_log.has_changed(),
      "mathlib was not reconfigured after transitive dependency fingerprint change");
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);
  }


  // Transitive invalidation with a stimulus that perturbs ONLY the dependency, leaving the
  // parent toolchain untouched. The previous test changes the parent toolchain, which reaches
  // mathlib's proxy both through DEPENDENCY_FINGERPRINTS and directly through
  // ROOT_PROJECT_TOOLCHAIN_FINGERPRINT -- so it cannot prove which path caused the change.
  // Here the HfcDependencyProvidedLib declaration lists ENV{HFC_DEP_PERTURB} as a per-dependency
  // fingerprint variable, so changing that env var alters ONLY HfcDependencyProvidedLib's
  // fingerprint. mathlib's ROOT_PROJECT_TOOLCHAIN_FINGERPRINT is unchanged, so the only way
  // mathlib's proxy can change is through the DEPENDENCY_FINGERPRINTS entry.
  BOOST_FIXTURE_TEST_CASE(configure_stability_transitive_invalidation_via_dependency_fingerprint_only, test_isolation_fixture) {
    auto cmake_path = bp::search_path("cmake");
    if(cmake_path.string().empty()) {
      throw std::runtime_error("cmake not found on PATH");
    }
    test_variant data{cmake_path, false};

    fs::path project_path = prepare_project_to_be_tested("check_find_package", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, { "MathFunctions.h", "MathFunctionscbrt.h", "lib.h" });

    std::string cmake_configure_command = get_cmake_configure_command(project_path, data);

    auto hfc_dep_proxy  = project_path / "build" / "_deps" / "HfcDependencyProvidedLib-toolchain" / "hfc_hermetic_proxy_toolchain.cmake";
    auto mathlib_proxy  = project_path / "build" / "_deps" / "mathlib-toolchain"                 / "hfc_hermetic_proxy_toolchain.cmake";
    auto mathlib_build_dir       = project_path / "build" / "_deps" / "mathlib-build";
    auto path_mathlib_config_log = mathlib_build_dir / "CMakeFiles" / "CMakeConfigureLog.yaml";

    auto extract_dep_fingerprint = [&](const fs::path& proxy) -> std::string {
      std::string content = pre::file::to_string(proxy.generic_string());
      boost::smatch m;
      if(boost::regex_search(content, m, boost::regex{"HfcDependencyProvidedLib=([0-9a-fA-F.]+)"})) {
        return m[1].str();
      }
      return {};
    };

    auto extract_root_toolchain_fingerprint = [&](const fs::path& proxy) -> std::string {
      std::string content = pre::file::to_string(proxy.generic_string());
      boost::smatch m;
      if(boost::regex_search(content, m, boost::regex{"HERMETIC_FETCHCONTENT_ROOT_PROJECT_TOOLCHAIN_FINGERPRINT \"([0-9a-fA-F]+)\""})) {
        return m[1].str();
      }
      return {};
    };

    // Configure 1
    std::cout << "⚗️ [Configure 1]" << std::endl;
    test_env["HFC_DEP_PERTURB"] = "before";
    run_command(cmake_configure_command, project_path, test_env);

    BOOST_REQUIRE(fs::exists(hfc_dep_proxy));
    BOOST_REQUIRE(fs::exists(mathlib_proxy));
    BOOST_REQUIRE(fs::exists(path_mathlib_config_log));

    std::string dep_fingerprint_before = extract_dep_fingerprint(mathlib_proxy);
    std::string mathlib_root_tc_before = extract_root_toolchain_fingerprint(mathlib_proxy);
    BOOST_REQUIRE_MESSAGE(!dep_fingerprint_before.empty(),
      "mathlib proxy toolchain missing HfcDependencyProvidedLib=<hash> in HERMETIC_FETCHCONTENT_DEPENDENCY_FINGERPRINTS");
    BOOST_REQUIRE_MESSAGE(!mathlib_root_tc_before.empty(),
      "mathlib proxy toolchain missing HERMETIC_FETCHCONTENT_ROOT_PROJECT_TOOLCHAIN_FINGERPRINT");

    file_fingerprint fp_mathlib_proxy{mathlib_proxy};
    file_fingerprint fp_mathlib_config_log{path_mathlib_config_log};

    // Configure 2 / perturb ONLY the dependency's fingerprint via its env input
    std::cout << "⚗️ [Configure 2 / dependency fingerprint perturbed]" << std::endl;
    test_env["HFC_DEP_PERTURB"] = "after";
    run_command(cmake_configure_command, project_path, test_env);

    // The parent toolchain is untouched, so mathlib's ROOT_PROJECT_TOOLCHAIN_FINGERPRINT must
    // be identical -- this is what isolates the change to the DEPENDENCY_FINGERPRINTS path.
    BOOST_REQUIRE_MESSAGE(extract_root_toolchain_fingerprint(mathlib_proxy) == mathlib_root_tc_before,
      "mathlib ROOT_PROJECT_TOOLCHAIN_FINGERPRINT changed -- stimulus leaked into the parent toolchain, "
      "test no longer isolates the DEPENDENCY_FINGERPRINTS path");

    // The dependency fingerprint entry must change...
    std::string dep_fingerprint_after = extract_dep_fingerprint(mathlib_proxy);
    BOOST_REQUIRE_MESSAGE(dep_fingerprint_after != dep_fingerprint_before,
      "HfcDependencyProvidedLib fingerprint in mathlib's DEPENDENCY_FINGERPRINTS did not change "
      "after perturbing only the dependency's fingerprint input");

    // ...and mathlib's proxy + reconfigure must follow.
    BOOST_REQUIRE_MESSAGE(fp_mathlib_proxy.has_changed(),
      "mathlib proxy toolchain did not change after the dependency's fingerprint changed");
    BOOST_REQUIRE_MESSAGE(fp_mathlib_config_log.has_changed(),
      "mathlib was not reconfigured after the dependency's fingerprint changed");
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);
  }


  // Generator expressions in add_compile_definitions() / add_compile_options()
  // are hashed as literal strings, not as evaluated values.  When the GENEX
  // references state that is NOT a CMAKE_* cache variable (here a user-defined
  // cache variable accessed via $CACHE{...}), changing that state between
  // configure runs produces an identical fingerprint even though the effective
  // compilation flags differ.
  //
  // The GENEX $<$<BOOL:$CACHE{HFC_TEST_GENEX_FLAG}>:HFC_GENEX_FLAG_ACTIVE>
  // is stored as a constant string regardless of the flag value.  HFC should
  // detect the change and reconfigure the dependency -- but currently does not.
  BOOST_DATA_TEST_CASE_F(test_isolation_fixture, configure_stability_cmake_dependency_genex_blind_spot,
      boost::unit_test::data::make(hfc::test::cmake_hidden_input_variants()), data){

    fs::path project_path = prepare_project_to_be_tested("configure_stability_cmake_dependency_genex_blind_spot", data.is_cmake_re, temp_dir);
    write_project_tipi_id(project_path);
    write_simple_main(project_path, { "MathFunctions.h" }, "simple_example.cpp");

    // Use the dedicated genex-blindspot toolchain which contains two GENEXs:
    //  GENEX 1: $<$<BOOL:$CACHE{HFC_TEST_GENEX_FLAG}>:HFC_GENEX_FLAG_ACTIVE>
    //           references a forwarded non-CMAKE_ cache variable
    //  GENEX 2: $<$<CONFIG:Release>:HFC_TEST_RELEASE_MODE>
    //           pure toolchain-local expression, no forwarding needed
    fs::path toolchain_with_genex = get_project_toolchain_path(project_path, "linux-toolchain-genex-blindspot.cmake");

    std::string cmake_configure_flag_off = get_cmake_configure_command(project_path, data, "-DHFC_TEST_GENEX_FLAG=OFF", toolchain_with_genex);
    std::string cmake_configure_flag_on  = get_cmake_configure_command(project_path, data, "-DHFC_TEST_GENEX_FLAG=ON",  toolchain_with_genex);
    std::string cmake_build_command      = get_cmake_build_command(project_path, data);

    BOOST_REQUIRE(is_empty_directory(project_path / "build"));

    auto mathlib_build_dir   = project_path / "build" / "_deps" / "mathlib-build";
    auto mathlib_install_dir = project_path / "build" / "_deps" / "mathlib-install";
    auto path_mathlib_CMakeConfigureLog = mathlib_build_dir / "CMakeFiles" / "CMakeConfigureLog.yaml";

    auto extract_toolchain_fingerprint = [](const std::string& cmake_output) -> std::string {
      boost::smatch m;
      if(boost::regex_search(cmake_output, m, boost::regex{" - toolchain fingerprint: ([0-9a-f]+)"})) {
        return m[1];
      }
      return "";
    };

    // Extracts the generation-time evaluated GENEX values logged by
    // compute_augmented_toolchain_fingerprint_isolated after reading
    // _hfc_genex_evals.txt from the mini-project.
    auto extract_genex_evals = [](const std::string& cmake_output) -> std::string {
      boost::smatch m;
      if(boost::regex_search(cmake_output, m, boost::regex{" - toolchain genex evals: ([^\n]+)"})) {
        return m[1];
      }
      return "";
    };

    // Configure 1 / GENEX flag = OFF
    std::cout << "⚗️ [Configure 1 / HFC_TEST_GENEX_FLAG=OFF]" << std::endl;
    std::string output_c1 = run_command(cmake_configure_flag_off, project_path, test_env);

    BOOST_REQUIRE(fs::exists(path_mathlib_CMakeConfigureLog));
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    std::string fingerprint_c1 = extract_toolchain_fingerprint(output_c1);
    BOOST_REQUIRE_MESSAGE(!fingerprint_c1.empty(), "toolchain fingerprint not found in configure output");

    // GENEX 2 (config-based) is always present; GENEX 1 (forwarded flag) is absent when OFF
    std::string genex_evals_c1 = extract_genex_evals(output_c1);
    BOOST_REQUIRE_MESSAGE(!genex_evals_c1.empty(), "genex evals not found in configure output -- _hfc_genex_evals.txt was not produced");
    BOOST_REQUIRE_MESSAGE(genex_evals_c1.find("HFC_TEST_RELEASE_MODE") != std::string::npos,
      "GENEX 2 (HFC_TEST_RELEASE_MODE) not found in genex evals: " << genex_evals_c1);
    BOOST_REQUIRE_MESSAGE(genex_evals_c1.find("HFC_GENEX_FLAG_ACTIVE") == std::string::npos,
      "GENEX 1 (HFC_GENEX_FLAG_ACTIVE) should be absent when flag=OFF, but found in: " << genex_evals_c1);

    // Build 1
    std::cout << "⚗️ [Build 1]" << std::endl;
    run_command(cmake_build_command, project_path, test_env);

    BOOST_REQUIRE(fs::exists(mathlib_install_dir / "lib" / "libMathFunctions.a"));
    BOOST_REQUIRE(fs::exists(project_path / "build" / "MyExample"));

    file_fingerprint ffingerprint_c1_CMakeConfigureLog{path_mathlib_CMakeConfigureLog};

    // Configure 2 / no changes
    std::cout << "⚗️ [Configure 2 / no changes]" << std::endl;
    std::string output_c2 = run_command(cmake_configure_flag_off, project_path, test_env);

    std::string fingerprint_c2 = extract_toolchain_fingerprint(output_c2);
    BOOST_REQUIRE_MESSAGE(fingerprint_c1 == fingerprint_c2,
      "fingerprint should be stable when nothing changes: " << fingerprint_c1 << " -> " << fingerprint_c2);
    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    // Configure 3 / GENEX flag toggled to ON
    std::cout << "⚗️ [Configure 3 / HFC_TEST_GENEX_FLAG=ON]" << std::endl;
    std::string output_c3 = run_command(cmake_configure_flag_on, project_path, test_env);

    std::string fingerprint_c3 = extract_toolchain_fingerprint(output_c3);
    BOOST_REQUIRE_MESSAGE(!fingerprint_c3.empty(), "toolchain fingerprint not found in configure output");

    // Both GENEXs must appear in the evaluated output:
    //  GENEX 1 (forwarded flag=ON)  -> HFC_GENEX_FLAG_ACTIVE
    //  GENEX 2 (pure config-based)  -> HFC_TEST_RELEASE_MODE
    std::string genex_evals_c3 = extract_genex_evals(output_c3);
    BOOST_REQUIRE_MESSAGE(!genex_evals_c3.empty(), "genex evals not found in configure output -- _hfc_genex_evals.txt was not produced");
    BOOST_REQUIRE_MESSAGE(genex_evals_c3.find("HFC_GENEX_FLAG_ACTIVE") != std::string::npos,
      "GENEX 1 (HFC_GENEX_FLAG_ACTIVE) not found in genex evals after flag=ON: " << genex_evals_c3);
    BOOST_REQUIRE_MESSAGE(genex_evals_c3.find("HFC_TEST_RELEASE_MODE") != std::string::npos,
      "GENEX 2 (HFC_TEST_RELEASE_MODE) not found in genex evals: " << genex_evals_c3);

    // The fingerprint must have changed because GENEX 1's evaluation changed
    BOOST_REQUIRE_MESSAGE(fingerprint_c1 != fingerprint_c3,
      "GENEX blind spot: fingerprint did not change after toggling $CACHE{HFC_TEST_GENEX_FLAG} OFF -> ON: " << fingerprint_c3);

    BOOST_REQUIRE_MESSAGE(ffingerprint_c1_CMakeConfigureLog.has_changed(),
      "GENEX blind spot: dependency was not reconfigured after GENEX-accessible state change");

    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);
  }

}
