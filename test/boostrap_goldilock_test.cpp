#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>

#include <test_project.hpp>
#include <test_helpers.hpp>
#include <pre/file/string.hpp>

int main(int argc, char** argv) {
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
  using namespace std::string_literals;
  using namespace hfc::test;

  //
  fs::path test_project_path = prepare_project_to_be_tested("boostrap_goldilock", false);
  fs::path project_toolchain = get_project_toolchain_path(test_project_path);

  std::string cmake_configure_command = "cmake -GNinja -DCMAKE_TOOLCHAIN_FILE="s + project_toolchain.generic_string() + " -S "s + (test_project_path.generic_string()) + " -B "s + ((test_project_path / "build").generic_string()); 
  run_command(cmake_configure_command, test_project_path);
  return 0;
}