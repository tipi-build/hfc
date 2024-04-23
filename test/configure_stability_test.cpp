#define BOOST_TEST_MODULE configure_stability_test
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>

#include <test_project.hpp>
#include <test_variant.hpp>

#include <pre/file/string.hpp>
#include <pre/file/hash.hpp>

#include <test_helpers.hpp>


namespace hfc::test { 
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
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


  BOOST_DATA_TEST_CASE(configure_stability_autotools_dependency_AtConfigureTime, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path project_path = prepare_project_to_be_tested("configure_stability_autotools_dependency_AtConfigureTime", data.is_cmake_re);
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
    run_command(cmake_configure_command, project_path);

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
    run_command(cmake_configure_command, project_path);

    BOOST_REQUIRE(ffingerprint_c1_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libiconv.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
    BOOST_REQUIRE(count_install_done_files(iconv_install_dir) == 1);

    // build
    std::cout << "⚗️ [Build 1]" << std::endl;
    run_command(cmake_build_command, project_path);

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
    run_command(cmake_configure_command, project_path);
    std::cout << "👷 [Build 2]" << std::endl;
    run_command(cmake_build_command, project_path);

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
    run_command(cmake_build_command, project_path);

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

    append_to_toolchain(project_toolchain, "\n# something different");
    BOOST_REQUIRE(toolchain_fingerprint.has_changed());

    std::cout << "👷 [Configure 4 / toolchain changed]" << std::endl;
    run_command(cmake_configure_command, project_path);

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
    run_command(cmake_build_command, project_path);


    BOOST_REQUIRE(ffingerprint_c4_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_installed_libiconv.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b3_project_binary.has_changed());  // now it should be "different" (works in dependently of is_cmake_re bc. the path stored in not resolved)
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
    BOOST_REQUIRE(count_install_done_files(iconv_install_dir) == 1);

    file_fingerprint ffingerprint_b4_project_binary{path_project_binary};

    std::cout << "👷 [Configure 5 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path);
    std::cout << "👷 [Build 5]" << std::endl;
    run_command(cmake_build_command, project_path);

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
    run_command(cmake_configure_command, project_path);

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
    run_command(cmake_build_command, project_path);

    BOOST_REQUIRE(ffingerprint_c5_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_installed_libiconv.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.has_changed());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
    BOOST_REQUIRE(count_install_done_files(iconv_install_dir) == 1);   
    
  }

  BOOST_DATA_TEST_CASE(configure_stability_autotools_dependency_AtBuildTime, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path project_path = prepare_project_to_be_tested("configure_stability_autotools_dependency_AtBuildTime", data.is_cmake_re);
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
    run_command(cmake_configure_command, project_path);

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
    run_command(cmake_configure_command, project_path);

    BOOST_REQUIRE(ffingerprint_c1_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_makefile.is_unchanged());
    BOOST_REQUIRE(!fs::exists(path_iconv_installed_libiconv));
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);

    // build
    std::cout << "⚗️ [Build 1]" << std::endl;
    run_command(cmake_build_command, project_path);

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
    run_command(cmake_configure_command, project_path);
    std::cout << "👷 [Build 2]" << std::endl;
    run_command(cmake_build_command, project_path);

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
    run_command(cmake_build_command, project_path);

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

    append_to_toolchain(project_toolchain, "\n# something different");
    BOOST_REQUIRE(toolchain_fingerprint.has_changed());

    std::cout << "👷 [Configure 4 / toolchain changed]" << std::endl;
    run_command(cmake_configure_command, project_path);

    BOOST_REQUIRE(ffingerprint_c1_config_log.has_changed());
    BOOST_REQUIRE(ffingerprint_c1_config_status.has_changed());
    BOOST_REQUIRE(ffingerprint_c1_makefile.has_changed());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);

    BOOST_REQUIRE(!fs::exists(path_iconv_installed_libiconv));    

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
    run_command(cmake_build_command, project_path);

    BOOST_REQUIRE(ffingerprint_c4_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b3_project_binary.has_changed());  // now it should be "different" (works in dependently of is_cmake_re bc. the path stored in not resolved)
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);

    file_fingerprint ffingerprint_b4_installed_libiconv{path_iconv_installed_libiconv};
    file_fingerprint ffingerprint_b4_project_binary{path_project_binary};

    std::cout << "👷 [Configure 5 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path);
    std::cout << "👷 [Build 5]" << std::endl;
    run_command(cmake_build_command, project_path);

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
    run_command(cmake_configure_command, project_path);

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
    run_command(cmake_build_command, project_path);

    BOOST_REQUIRE(ffingerprint_c5_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_makefile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_installed_libiconv.has_changed());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.has_changed());
    BOOST_REQUIRE(count_configure_done_files(iconv_build_dir.parent_path()) == 1);
  }


  BOOST_DATA_TEST_CASE(configure_stability_cmake_dependency_AtConfigureTime, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path project_path = prepare_project_to_be_tested("configure_stability_cmake_dependency_AtConfigureTime", data.is_cmake_re);
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
    run_command(cmake_configure_command, project_path);

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
    run_command(cmake_configure_command, project_path);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_path_mathlib_ninjafile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libMathFunctionscbrt.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    // build
    std::cout << "⚗️ [Build 1]" << std::endl;
    run_command(cmake_build_command, project_path);

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
    run_command(cmake_configure_command, project_path);
    std::cout << "👷 [Build 2]" << std::endl;
    run_command(cmake_build_command, project_path);

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
    run_command(cmake_build_command, project_path);

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

    append_to_toolchain(project_toolchain, "\n# something different");
    BOOST_REQUIRE(toolchain_fingerprint.has_changed());

    std::cout << "👷 [Configure 4 / toolchain changed]" << std::endl;
    run_command(cmake_configure_command, project_path);

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
    run_command(cmake_build_command, project_path);


    BOOST_REQUIRE(ffingerprint_c4_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_installed_libMathFunctionscbrt.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b3_project_binary.has_changed());  // now it should be "different" (works in dependently of is_cmake_re bc. the path stored in not resolved)
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    file_fingerprint ffingerprint_b4_project_binary{path_project_binary};

    std::cout << "👷 [Configure 5 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path);
    std::cout << "👷 [Build 5]" << std::endl;
    run_command(cmake_build_command, project_path);

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
    run_command(cmake_configure_command, project_path);

    BOOST_REQUIRE(ffingerprint_c4_config_log.has_changed());
    BOOST_REQUIRE(ffingerprint_c4_config_status.has_changed());
    BOOST_REQUIRE(ffingerprint_c4_installed_libMathFunctionscbrt.has_changed());
    BOOST_REQUIRE(ffingerprint_b4_installed_libMathFunctions.has_changed());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    file_fingerprint ffingerprint_c5_config_log{path_mathlib_CMakeConfigureLog};
    file_fingerprint ffingerprint_c5_config_status{path_mathlib_ninjafile};
    file_fingerprint ffingerprint_c5_installed_libMathFunctions{path_mathlib_installed_libMathFunctions};
    file_fingerprint ffingerprint_c5_libMathFunctionscbrt{path_mathlib_installed_libMathFunctionscbrt};

    std::cout << "👷 [Build 6 / project CmakeLists.txt changed]" << std::endl;
    run_command(cmake_build_command, project_path);

    BOOST_REQUIRE(ffingerprint_c5_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_config_status.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_libMathFunctionscbrt.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_installed_libMathFunctions.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.has_changed());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);
  }

   BOOST_DATA_TEST_CASE(configure_stability_cmake_dependency_AtBuildTime, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path project_path = prepare_project_to_be_tested("configure_stability_cmake_dependency_AtBuildTime", data.is_cmake_re);
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
    run_command(cmake_configure_command, project_path);

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
    run_command(cmake_configure_command, project_path);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c1_path_mathlib_ninjafile.is_unchanged());
    BOOST_REQUIRE(!fs::exists(path_mathlib_installed_libMathFunctions));
    BOOST_REQUIRE(!fs::exists(path_mathlib_installed_libMathFunctionscbrt));
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    // build
    std::cout << "⚗️ [Build 1]" << std::endl;
    run_command(cmake_build_command, project_path);

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
    run_command(cmake_configure_command, project_path);
    std::cout << "👷 [Build 2]" << std::endl;
    run_command(cmake_build_command, project_path);

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
    run_command(cmake_build_command, project_path);

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

    append_to_toolchain(project_toolchain, "\n# something different");
    BOOST_REQUIRE(toolchain_fingerprint.has_changed());

    std::cout << "👷 [Configure 4 / toolchain changed]" << std::endl;
    run_command(cmake_configure_command, project_path);

    BOOST_REQUIRE(ffingerprint_c1_CMakeConfigureLog.has_changed());
    BOOST_REQUIRE(ffingerprint_c1_path_mathlib_ninjafile.has_changed());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);
    BOOST_REQUIRE(!fs::exists(path_mathlib_installed_libMathFunctions));
    BOOST_REQUIRE(!fs::exists(path_mathlib_installed_libMathFunctionscbrt));

    // note: cmake-re will mirror to a different location so this will be changed rightfully in that case
    if(data.is_cmake_re) {
      BOOST_REQUIRE(!fs::exists(path_project_binary));
    } else {
      BOOST_REQUIRE(ffingerprint_b3_project_binary.is_unchanged()); // !! not built yet !!
    }

    file_fingerprint ffingerprint_c4_config_log{path_mathlib_CMakeConfigureLog};
    file_fingerprint ffingerprint_c4_ninjafile{path_mathlib_ninjafile};
    
    // build
    std::cout << "👷 [Build 4 / toolchain changed]" << std::endl;
    run_command(cmake_build_command, project_path);

    BOOST_REQUIRE(ffingerprint_c4_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_ninjafile.is_unchanged());
    BOOST_REQUIRE(ffingerprint_b3_project_binary.has_changed());  // now it should be "different" (works in dependently of is_cmake_re bc. the path stored in not resolved)
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    file_fingerprint ffingerprint_b4_installed_libMathFunctions{path_mathlib_installed_libMathFunctions};
    file_fingerprint ffingerprint_b4_installed_libMathFunctionscbrt{path_mathlib_installed_libMathFunctionscbrt};
    file_fingerprint ffingerprint_b4_project_binary{path_project_binary};

    std::cout << "👷 [Configure 5 / no changes]" << std::endl;
    run_command(cmake_configure_command, project_path);
    std::cout << "👷 [Build 5]" << std::endl;
    run_command(cmake_build_command, project_path);

    BOOST_REQUIRE(ffingerprint_c4_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c4_ninjafile.is_unchanged());
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
    run_command(cmake_configure_command, project_path);

    BOOST_REQUIRE(ffingerprint_c4_config_log.has_changed());
    BOOST_REQUIRE(ffingerprint_c4_ninjafile.has_changed());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);
    BOOST_REQUIRE(!fs::exists(path_mathlib_installed_libMathFunctions));  
    BOOST_REQUIRE(!fs::exists(path_mathlib_installed_libMathFunctionscbrt));
    BOOST_REQUIRE(ffingerprint_b4_project_binary.is_unchanged());
    

    file_fingerprint ffingerprint_c5_config_log{path_mathlib_CMakeConfigureLog};
    file_fingerprint ffingerprint_c5_config_status{path_mathlib_ninjafile};

    std::cout << "👷 [Build 6 / project CmakeLists.txt changed]" << std::endl;
    run_command(cmake_build_command, project_path);

    BOOST_REQUIRE(ffingerprint_c5_config_log.is_unchanged());
    BOOST_REQUIRE(ffingerprint_c5_config_status.is_unchanged());
    BOOST_REQUIRE(count_configure_done_files(mathlib_build_dir) == 1);

    BOOST_REQUIRE(ffingerprint_b4_installed_libMathFunctions.has_changed());
    BOOST_REQUIRE(ffingerprint_b4_installed_libMathFunctionscbrt.has_changed());
    BOOST_REQUIRE(ffingerprint_b4_project_binary.has_changed());
    
  }

}
