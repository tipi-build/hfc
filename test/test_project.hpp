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
    return get_project_toolchain_dir(project_folder) / "linux-toolchain.cmake";
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
    fs::copy((source_tree / "test" / "test_project_templates" / "toolchain"), project_toolchain_dir, fs::copy_options::recursive);

    if(fs::exists(project_path / "build")){
      fs::remove_all(project_path / "build");
    }

    if(is_cmake_re){
      remove_template_from_mirror(name_of_the_template);
      
      // copy the goldilock if it's available in the env
      const char* hfc_tools_dir_env_value = std::getenv("HFC_TEST_SHARED_TOOLS_DIR");
     
      if(hfc_tools_dir_env_value != NULL) {
        auto hfc_tools_dir_env_value_str = std::string(hfc_tools_dir_env_value);

        fs::path hfc_tools_dir_dest = project_path / "toolchain" / ".hfc_tools_dir";
        fs::create_directories(hfc_tools_dir_dest);
        fs::copy(hfc_tools_dir_env_value_str, hfc_tools_dir_dest, fs::copy_options::recursive);
      }
    }

    fs::create_directories(project_path / "build");
    
    // copy HFC sources over there
    if(fs::exists(project_path / "cmake")){
      fs::remove_all(project_path / "cmake");
    }
    
    fs::create_directory(project_path / "cmake");
    fs::copy(source_tree / "cmake",  project_path / "cmake", fs::copy_options::recursive);

    std::cout << "Project setup from template in: "<< project_path.string() << std::endl;
    return project_path;
  }


  inline void write_simple_main(const fs::path& template_path, const std::vector<std::string>& includes, const std::string& name_of_file = "simple_example.cpp", const std::string& additional_main_body_code = ""){
    std::string main;
    std::stringstream main_ss{};

    for (auto include : includes){
      main_ss << "#include <" << include << ">\n";
    }

    main_ss << "int main(){\n"
            << "  " << additional_main_body_code << "\n"
            << "  return 0;\n"
            << "}";


    pre::file::from_string(((template_path / name_of_file).string()), main_ss.str());
  }


  inline fs::path get_cmake_or_cmake_re(){
    fs::path cmake_bin = bp::search_path("cmake-re");
    
    if(cmake_bin.empty()){
      cmake_bin = bp::search_path("cmake");
    }

    return cmake_bin;
  }

    inline std::string get_cmake_configure_command(
    const fs::path& test_project_path, 
    const test_variant& data, 
    const std::string& additional_cmake_variables = "", 
    const std::optional<fs::path>& toolchain_file = std::nullopt, 
    const std::optional<std::string>& cmake_build_type = std::nullopt) 
  {
    std::stringstream cmd;
    const std::string SPACE = " ";
    
    cmd << data.cmake_bin.generic_string();
    cmd << SPACE << "-GNinja";

    if(additional_cmake_variables != "") {
      cmd << SPACE << additional_cmake_variables;
    }

    fs::path default_project_toolchain_path = get_project_toolchain_path(test_project_path);

    cmd << SPACE << "-DCMAKE_TOOLCHAIN_FILE=" << toolchain_file.value_or(default_project_toolchain_path).generic_string();
    cmd << SPACE << "-DCMAKE_BUILD_TYPE=" << cmake_build_type.value_or("Release");

    if(data.command_line_arguments != "") {
      cmd << SPACE << data.command_line_arguments;
    }

    if(data.configure_time_command_line_arguments != "") { 
      cmd << SPACE << data.configure_time_command_line_arguments;
    }

    cmd << SPACE << "-S " << test_project_path.generic_string();
    cmd << SPACE << "-B " << (test_project_path / "build").generic_string();
    return cmd.str();
  }

  inline std::string get_cmake_build_command(const fs::path& test_project_path, const test_variant& data, const std::string& additional_ninja_flags = "") { 
    std::stringstream cmd;
    const std::string SPACE = " ";

    cmd << data.cmake_bin.generic_string();

    if(data.command_line_arguments != "") {
      cmd << SPACE << data.command_line_arguments;
    }

    cmd << SPACE << "--build";
    cmd << SPACE << (test_project_path / "build").generic_string();

    if(data.build_time_command_line_arguments != "") { 
      cmd << SPACE << data.build_time_command_line_arguments;
    }

    // forward flags to the underlying (ninja) build system
    if(additional_ninja_flags != "") {
      cmd << SPACE << " -- " << additional_ninja_flags;
    }

    return cmd.str();
  }

  //!\brief append some string as comment to the toolchain file
  inline void append_random_testdata_marker_as_toolchain_comment(fs::path toolchain_path, const test_variant& test_variant_data){
    std::ofstream outfile;
    outfile.open(toolchain_path.generic_string(), std::ios_base::app | std::ios::binary); // append instead of overwrite
    outfile << "\n\n" << "# " << test_variant_data.command_line_arguments << " " << test_variant_data.configure_time_command_line_arguments << " " << test_variant_data.build_time_command_line_arguments << "\n";
    outfile << std::flush;
    outfile.close();
  }

    //!\brief append some string as comment to the toolchain file
  inline void append_to_toolchain(fs::path toolchain_path, const std::string& to_append){
    std::ofstream outfile;
    outfile.open(toolchain_path.generic_string(), std::ios_base::app | std::ios::binary); // append instead of overwrite
    outfile << "\n" << to_append << "\n" << std::flush;
    outfile.close();
  }
}