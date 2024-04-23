#pragma once

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
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

}