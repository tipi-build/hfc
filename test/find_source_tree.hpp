#pragma once 

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/process.hpp>

#include <vector>
#include <sstream>

#include <pre/file/string.hpp>
#include <test_variant.hpp>

namespace hfc::test {

  namespace fs = boost::filesystem;
  namespace bp = boost::process;


  inline fs::path get_source_tree_dir(){
    return fs::current_path();
  }

  inline void copy_hermetic_fetchContent_tree(fs::path template_path, fs::path source_tree){
    if(fs::exists(template_path / "cmake")){
      fs::remove_all(template_path / "cmake");
    }
    fs::create_directory(template_path / "cmake");
    fs::copy(source_tree / "cmake",  template_path / "cmake", fs::copy_options::recursive);
  }


  inline void remove_template_from_mirror(std::string name_of_the_template){
    std::vector<fs::path> dir_to_explore{};
    if(fs::exists("/usr/local/share/.tipi")){
      for (const auto& entry : fs::directory_iterator{"/usr/local/share/.tipi"}){
        if(entry.path().extension().string() == ".w"){
          dir_to_explore.push_back(entry.path());
        }
      }
    }

    for(auto dir : dir_to_explore){
      for (const auto& entry : fs::directory_iterator{dir.string()}){ 
        if(boost::contains(entry.path().filename().string(),name_of_the_template) ){
          fs::remove_all(entry.path());
        }
      }  
    }
  }
  
  inline fs::path get_project_toolchain_dir(const fs::path& project_folder) {
    return project_folder / "toolchain";
  }
  
  inline fs::path get_project_toolchain_path(const fs::path& project_folder) {
    return get_project_toolchain_dir(project_folder) / "toolchain.cmake";
  }

  inline fs::path prepare_project_to_be_tested(std::string name_of_the_template, bool is_cmake_re){
    fs::path source_tree = get_source_tree_dir();    
    fs::path project_path = fs::temp_directory_path() / fs::unique_path() / name_of_the_template;
    fs::create_directories(project_path);    

    {
      fs::path template_path = source_tree / "test" / "test_project_templates" / name_of_the_template;    
      fs::copy(template_path, project_path);
    }

    fs::path project_toolchain_dir = get_project_toolchain_dir(project_path);

    if(fs::exists(project_toolchain_dir)){
      fs::remove_all(project_toolchain_dir);
    }

    fs::create_directories(project_toolchain_dir);
    fs::copy((source_tree / "test" / "test_project_templates" / "toolchain"), project_toolchain_dir);

    if(fs::exists(project_path / "build")){
      fs::remove_all(project_path / "build");
    }

    if(is_cmake_re){
      remove_template_from_mirror(name_of_the_template);
    }

    fs::create_directories (project_path / "build");
    copy_hermetic_fetchContent_tree(project_path, source_tree);

    std::cout << "Project setup from template in: "<< project_path.string() << std::endl;
    return project_path;
  }


  inline void write_simple_main(fs::path template_path,std::vector<std::string> includes, std::string name_of_file = "simple_example.cpp" ){
    std::string main;
    std::stringstream main_ss{};

    for (auto include : includes){
      main_ss << "#include <"<<include<<">\n";
    }

    main_ss << "int main(){return 0;}\n";

    pre::file::from_string(((template_path / name_of_file).string()), main_ss.str());
  }


  inline fs::path get_cmake_or_cmake_re(){

    fs::path cmake_bin = bp::search_path ("cmake-re");
    if(cmake_bin.empty()){
      cmake_bin = bp::search_path ("cmake");
    }

    return cmake_bin;
  }

  inline void generate_toolchain_with_cmake_re_data(fs::path toolchain_path, std::string enable_cmake_re_data){
    using namespace std::literals;
    std::string content{};
    if (fs::exists(toolchain_path)){
      content = pre::file::to_string(toolchain_path.generic_string());
    }
    content += "\n#"s+enable_cmake_re_data;
    pre::file::from_string(toolchain_path.generic_string(),content);
  
  }

  inline std::string get_cmake_configure_command(fs::path test_project_path, test_variant data, std::string additional_cmake_variables = ""){
    std::string verbose_cmake = "";
    if(data.is_cmake_re) { verbose_cmake = "-vv"; }
    return (data.cmake_bin.generic_string() + " -GNinja " + additional_cmake_variables + " -DCMAKE_TOOLCHAIN_FILE=toolchain/toolchain.cmake -DCMAKE_BUILD_TYPE=Release " + data.enable_cmake_re + " " +verbose_cmake+" -S "+test_project_path.string()+" -B "+(test_project_path / "build").string());  
  }

  inline std::string get_cmake_build_command(fs::path test_project_path, test_variant data, std::string additional_flags_ninja = ""){
    std::string cmake_build_command = data.cmake_bin.generic_string() + " --build build/";
    if(!additional_flags_ninja.empty()){
      cmake_build_command += " -- " + additional_flags_ninja;
    }
    if(data.is_cmake_re) {
      cmake_build_command = data.cmake_bin.generic_string() + " --build " + (test_project_path / "build").string();
    }
    return cmake_build_command;
  }

  inline std::string run_command(std::string command, fs::path test_path, bp::environment test_environment = boost::this_process::environment()){
    bp::ipstream is;
    std::cout << "Running command: '" << command << "'" << std::endl; 
    bp::child c(test_environment, bp::start_dir=(test_path), bp::shell, command,  bp::std_out > is, bp::std_err > stderr, bp::std_in < stdin);

    std::string output_command;
    std::string line;
    while (c.running() && std::getline(is, line)) {
      std::cout << line << std::endl;
      output_command += line;
    }
    //std::cout<<"Output for command : "<<command<<" is \n"<< output_command<<std::endl;
    c.wait();
    BOOST_REQUIRE(c.exit_code() == 0);
    return output_command;
  }

  inline void git_commit_change(std::string message, fs::path test_path, bp::environment test_environment = boost::this_process::environment()){
    fs::path git_bin =  bp::search_path ("git");
    std::string git_cmd = git_bin.generic_string() + " commit -a -m \""+message+"\"";
    run_command(git_cmd, test_path, test_environment);
  }
}