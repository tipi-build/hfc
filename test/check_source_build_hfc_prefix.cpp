#define BOOST_TEST_MODULE check_source_build_hfc_prefix
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

  BOOST_DATA_TEST_CASE(check_source_build_hfc_prefix_works, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path test_project_path = prepare_project_to_be_tested("check_source_build_hfc_prefix", data.is_cmake_re);
    fs::path project_toolchain = get_project_toolchain_path(test_project_path);


    write_simple_main(test_project_path,{"file_only_in_source.hpp","file_only_in_build.hpp"});  
    std::string cmake_configure_command = get_cmake_configure_command(test_project_path, data);
    run_command(cmake_configure_command, test_project_path);
    std::string cmake_build_command = get_cmake_build_command(test_project_path, data);
    run_command(cmake_build_command, test_project_path);

    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "MyExample" ));
    BOOST_REQUIRE(fs::exists(test_project_path / "build" / "_deps" / "mathlib-build" / "file_only_in_build.hpp"));
    BOOST_REQUIRE(!fs::exists(test_project_path / "build" / "_deps" / "mathlib-build" / "file_only_in_source.hpp"));
    if (!data.is_cmake_re){
      BOOST_REQUIRE(fs::exists(test_project_path / "thirdparty" / "cache" / "mathlib-c35bc46a-src" / "file_only_in_source.hpp" ));
      BOOST_REQUIRE(!fs::exists(test_project_path / "thirdparty" / "cache" / "mathlib-c35bc46a-src" / "file_only_in_build.hpp" ));   
    } else if(data.is_cmake_re){
      // the read simlink is /usr/local/share/.tipi/vK.w/876340f-check_source_build_hfc_prefix.b/1e0a9df/bin
      fs::path build_folder_mirror = fs::read_symlink(test_project_path / "build").parent_path().parent_path();
      fs::path test_project_path_mirror = build_folder_mirror.parent_path() / build_folder_mirror.stem();
      BOOST_REQUIRE(fs::exists(test_project_path_mirror / "thirdparty" / "cache" / "mathlib-c35bc46a-src" / "file_only_in_source.hpp" ));
      BOOST_REQUIRE(!fs::exists(test_project_path_mirror / "thirdparty" / "cache" / "mathlib-c35bc46a-src" / "file_only_in_build.hpp" ));   
    } 
  }

}