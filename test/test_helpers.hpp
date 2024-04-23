#pragma once

#define BOOST_TEST_NO_MAIN
#include <boost/test/included/unit_test.hpp>
#undef BOOST_TEST_NO_MAIN

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>
#include <pre/file/string.hpp>
#include <pre/file/hash.hpp>


namespace hfc::test { 
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
  using namespace std::string_literals;

 struct file_fingerprint {

    fs::path path;
    size_t size;
    std::time_t creation_time;
    std::time_t last_write_time;
    std::string content_hash;

    file_fingerprint(const fs::path& _path)
      : path{_path}
      , size(fs::file_size(path))
      , creation_time{fs::creation_time(path)}
      , last_write_time{fs::last_write_time(path)}
      , content_hash{pre::file::sha1sum(path.generic_string())}
    { /**/ }

    bool is_equal_to(const file_fingerprint& rhs) const {
      return path == rhs.path 
        && size == rhs.size 
        && creation_time == rhs.creation_time
        && last_write_time == rhs.last_write_time
        && content_hash == rhs.content_hash;
    }

    //!\brief check if the file is unchanged
    bool is_unchanged() const {
      auto fingerprint_now = file_fingerprint{path};
      return is_equal_to(fingerprint_now);
    }

    bool has_changed() const {
      return !is_unchanged();
    }
  };

  bool operator==(const file_fingerprint& lhs, const file_fingerprint& rhs) {
    return lhs.is_equal_to(rhs);
  }

  inline bool is_empty_directory(const fs::path& path) {
    return fs::exists(path) && fs::is_directory(path) && fs::is_empty(path);
  }

  inline void write_project_tipi_id(const fs::path& path) {
    // repo identity
    std::string repoid_hostname = "testhost";
    std::string repoid_org = "testorg";
    std::string repoid_reponame = path.generic_string();
    boost::replace_all(repoid_reponame, "/", "_");
    boost::replace_all(repoid_reponame, "\\", "_");
    
    // write an .tipi/id file
    fs::create_directories(path / ".tipi");

    auto dot_tipi_id_path = path / ".tipi" / "id";
    auto dot_tipi_id_content = "{ \"host_name\": \""s + repoid_hostname + "\", \"org_name\": \""s + repoid_org + "\", \"repo_name\": \""s + repoid_reponame + "\" }"s;
    pre::file::from_string(dot_tipi_id_path.generic_string(), dot_tipi_id_content);
  }

  //!\brief regex replace helper
  std::string regex_replace_all(const std::string& content, const std::string& exp, const std::string& replace, bool& matched, bool throw_if_not_found = false) {
    boost::regex rx{exp, boost::regex_constants::perl };
    matched = boost::regex_search(content, rx);

    if(matched) {
      std::string result = boost::regex_replace(content, rx, replace);
      return result;
    }

    if(throw_if_not_found) {
      throw std::runtime_error("Regex "s + exp + " did not match in string: "s + content);
    }

    return content;
  }

  //!\brief regex replace that throws if no match found
  std::string regex_replace_all(const std::string& content, const std::string& exp, const std::string& replace) {
    bool matched = false;
    return regex_replace_all(content, exp, replace, matched, true /* throw if no match found */);
  }

  struct run_cmd_result_t {
    std::string output;
    size_t return_code;
  };

  /// @brief Wraps boost::process::system() and returns 
  /// @tparam ...Param 
  /// @param ...cmd 
  /// @return run_cmd_result_t containing the return code and console output as string
  template <class... Param> 
  inline run_cmd_result_t run_cmd(Param &&... cmd) {
    run_cmd_result_t result{};
  
    std::future<std::string> out;
    result.return_code = bp::system(cmd..., (bp::std_err & bp::std_out) > out, bp::std_in < stdin);
    
    // trim end newlines/spaces
    result.output = out.get();
    boost::algorithm::trim_right(result.output);

    return result;
  }

  inline std::string run_command(std::string command, fs::path test_path, bp::environment test_environment = boost::this_process::environment()){
    bp::ipstream is;
    std::cout << "Running command: '" << command << "'" << std::endl; 
    
    auto result = run_cmd(test_environment, bp::start_dir=(test_path), bp::shell, command);
  
    BOOST_TEST_MESSAGE(result.output);

    if(result.return_code != 0) {
      std::cout << "Command output:\n" << result.output << std::endl;
      std::cout << "Command return_code: " << result.return_code << std::endl;
      throw std::runtime_error("Command returned non-zero code");
    }

    return result.output;
  }

  inline void git_commit_change(std::string message, fs::path test_path, bp::environment test_environment = boost::this_process::environment()){
    fs::path git_bin =  bp::search_path ("git");
    std::string git_cmd = git_bin.generic_string() + " commit -a -m \""+message+"\"";
    run_command(git_cmd, test_path, test_environment);
  }

}