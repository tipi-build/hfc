#define BOOST_TEST_MODULE check_source_tree_removal
#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>

#include <find_source_tree.hpp>
#include <test_variant.hpp>

#include <pre/file/string.hpp>


namespace hfc::test { 
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
  using namespace std::literals;

  BOOST_DATA_TEST_CASE(check_source_tree_removal_for_cmake_deps, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path template_path = prepare_project_to_be_tested("check_compiler_forwarding",data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    generate_toolchain_with_cmake_re_data((template_path / "toolchain" / "toolchain.cmake"),data.enable_cmake_re);
  
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    run_command(cmake_configure_command, template_path);
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "mathlib-src" / "CMakeLists.txt" ));

    std::string cmake_build_command = get_cmake_build_command(template_path, data);
    run_command(cmake_build_command, template_path);
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "mathlib-src" / "CMakeLists.txt" ));

    run_command(cmake_configure_command, template_path);
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "mathlib-src" / "CMakeLists.txt" ));
    auto output = run_command(cmake_build_command, template_path);
    BOOST_REQUIRE(boost::contains(output, "ninja: no work to do."));
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "mathlib-src" / "CMakeLists.txt" ));

    std::string content = pre::file::to_string((template_path / "CMakeLists.txt").generic_string());
    content += "\n message(\"Display a message\")";
    pre::file::from_string((template_path / "CMakeLists.txt").generic_string(), content);

    content = pre::file::to_string((template_path / "simple_example.cpp").generic_string());
    content += "#include <vector>\n";
    pre::file::from_string((template_path / "simple_example.cpp").generic_string(), content);

    run_command(cmake_configure_command, template_path);
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "mathlib-src" / "CMakeLists.txt" ));
    output = run_command(cmake_build_command, template_path);
    BOOST_REQUIRE(!boost::contains(output, "ninja: no work to do."));
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "mathlib-src" / "CMakeLists.txt" ));
  }

  BOOST_DATA_TEST_CASE(check_after_source_tree_removal_commit_change_and_config_change_for_cmake_deps, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path template_path = prepare_project_to_be_tested("check_compiler_forwarding",data.is_cmake_re);
    write_simple_main(template_path,{"MathFunctions.h", "MathFunctionscbrt.h"});
    generate_toolchain_with_cmake_re_data((template_path / "toolchain" / "toolchain.cmake"),data.enable_cmake_re);
  
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    run_command(cmake_configure_command, template_path);
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "mathlib-src" / "CMakeLists.txt" ));
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "mathlib-subbuild" / "CMakeLists.txt" ));

    std::string cmake_build_command = get_cmake_build_command(template_path, data);
    run_command(cmake_build_command, template_path);
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "mathlib-src" / "CMakeLists.txt" ));
    BOOST_REQUIRE(!fs::exists(template_path / "build"  / "_deps" / "mathlib-subbuild" / "CMakeLists.txt" ));

    std::string content = pre::file::to_string((template_path / "CMakeLists.txt").generic_string());
    boost::algorithm::replace_all(content,"ecc756a4c3f1811cdfd637bd6d8f4e3feb6aff92","20f983e688fd77961445608d44bc90df15f38e76");
    pre::file::from_string((template_path / "CMakeLists.txt").generic_string(), content);

    run_command(cmake_configure_command, template_path);
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "mathlib-src" / "CMakeLists.txt" ));
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" /"mathlib-subbuild" / "CMakeLists.txt" ));

    run_command(cmake_build_command, template_path);
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "mathlib-src" / "CMakeLists.txt" ));
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" /  "mathlib-subbuild" / "CMakeLists.txt" ));

    content = pre::file::to_string((template_path / "CMakeLists.txt").generic_string());
    std::string add_hermetic_toolchain= "HERMETIC_BUILD_SYSTEM cmake \n HERMETIC_TOOLCHAIN_EXTENSION \"set(A_VAR ON)\"";
    boost::algorithm::replace_all(content,"HERMETIC_BUILD_SYSTEM cmake",add_hermetic_toolchain);
    pre::file::from_string((template_path / "CMakeLists.txt").generic_string(), content);

    run_command(cmake_configure_command, template_path);
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "mathlib-src" / "CMakeLists.txt" ));
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" /"mathlib-subbuild" / "CMakeLists.txt" ));

    run_command(cmake_build_command, template_path);
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "mathlib-src" / "CMakeLists.txt" ));
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "mathlib-subbuild" / "CMakeLists.txt" ));

  }

  BOOST_DATA_TEST_CASE(check_after_source_tree_removal_commit_change_and_config_change_for_autotools_deps, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path template_path = prepare_project_to_be_tested("check_compiler_forwarding_autotools",data.is_cmake_re);
    write_simple_main(template_path,{"lib.h"});
    generate_toolchain_with_cmake_re_data((template_path / "toolchain" / "toolchain.cmake"),data.enable_cmake_re);
  
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    run_command(cmake_configure_command, template_path);
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "iconv-src" / "src" / "configure" ));
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "iconv-subbuild" / "CMakeLists.txt" ));

    std::string cmake_build_command = get_cmake_build_command(template_path, data);
    run_command(cmake_build_command, template_path);
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "iconv-src" / "src" / "configure" ));
    BOOST_REQUIRE(!fs::exists(template_path / "build"  /"_deps" / "iconv-subbuild" / "CMakeLists.txt" ));

    std::string content = pre::file::to_string((template_path / "CMakeLists.txt").generic_string());
    boost::algorithm::replace_all(content,"ad80b024eeda8f4c0a96eedf669dc453ed33a094","1e7e45ac0ae46905afc4070d65c417e303ecb180");
    pre::file::from_string((template_path / "CMakeLists.txt").generic_string(), content);

    run_command(cmake_configure_command, template_path);
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "iconv-src" / "src" / "configure" ));
    BOOST_REQUIRE(fs::exists(template_path / "build" /"_deps" / "iconv-subbuild" / "CMakeLists.txt" ));

    run_command(cmake_build_command, template_path);
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "iconv-src" / "src" / "configure" ));
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" /  "iconv-subbuild" / "CMakeLists.txt" ));


    content = pre::file::to_string((template_path / "CMakeLists.txt").generic_string());
    std::string add_hermetic_toolchain= "TIPI_TEAM_ZURICH \n TIPI_TEAM_LOCATION=ZURICH";
    boost::algorithm::replace_all(content,"TIPI_TEAM_ZURICH",add_hermetic_toolchain);
    pre::file::from_string((template_path / "CMakeLists.txt").generic_string(), content);

    run_command(cmake_configure_command, template_path);
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "iconv-src" / "src" / "configure" ));
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" /"iconv-subbuild" / "CMakeLists.txt" ));

    run_command(cmake_build_command, template_path);
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "iconv-src" / "src" / "configure" ));
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "iconv-subbuild" / "CMakeLists.txt" ));
  }

  BOOST_DATA_TEST_CASE(check_source_tree_removal_for_autotools_deps, boost::unit_test::data::make(hfc::test::test_variants()), data){
    fs::path template_path = prepare_project_to_be_tested("check_compiler_forwarding_autotools",data.is_cmake_re);
    write_simple_main(template_path,{"lib.h"});

    generate_toolchain_with_cmake_re_data((template_path / "toolchain" / "toolchain.cmake"),data.enable_cmake_re);
    std::cout<<template_path.string()<<std::endl;
    std::string cmake_configure_command = get_cmake_configure_command(template_path, data);
    run_command(cmake_configure_command, template_path);
    BOOST_REQUIRE(fs::exists(template_path / "build" / "_deps" / "iconv-src" / "src" / "configure" ));

    std::string cmake_build_command = get_cmake_build_command(template_path, data);
    run_command(cmake_build_command, template_path);
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "iconv-src" / "src" / "configure" ));

    run_command(cmake_configure_command, template_path);
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "iconv-src" / "src" / "configure" ));
    auto output = run_command(cmake_build_command, template_path);
    BOOST_REQUIRE(boost::contains(output, "ninja: no work to do."));
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "iconv-src" / "src" / "configure" ));

    std::string content = pre::file::to_string((template_path / "CMakeLists.txt").generic_string());
    content += "\n message(\"Display a message\")";
    pre::file::from_string((template_path / "CMakeLists.txt").generic_string(), content);

    content = pre::file::to_string((template_path / "simple_example.cpp").generic_string());
    content += "#include <vector>\n";
    pre::file::from_string((template_path / "simple_example.cpp").generic_string(), content);

    run_command(cmake_configure_command, template_path);
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "iconv-src" / "src" / "configure" ));
    output = run_command(cmake_build_command, template_path);
    BOOST_REQUIRE(!boost::contains(output, "ninja: no work to do."));
    BOOST_REQUIRE(!fs::exists(template_path / "build" / "_deps" / "iconv-src" / "src" / "configure" ));
  }
}