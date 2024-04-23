#pragma once

#include <stdexcept>
#include <system_error>
#include <fstream>
#include <string>
#include <boost/filesystem.hpp>

namespace pre::file {

  using namespace std::string_literals;

  //! Simply read the path and return it's content as string as-is without formatting (binary one-to-one)
  inline std::string to_string(const std::string &path, std::error_condition &ec);

  //! Simply read the path and return it's content as string as-is without formatting (binary one-to-one ).
  //! Throws on error
  inline std::string to_string(const std::string &path);

  //! Simply truncates path and write in it's place the new content
  inline void from_string(const std::string &path, const std::string &content, std::error_condition &ec);

  //! Simply truncates path and content in it's place the new content
  //! Throws on error
  inline void from_string(const std::string &path, const std::string &content);

  inline std::string to_string(const std::string &path, std::error_condition &ec) {

    std::ifstream ifs(path, std::ios::in | std::ios::binary);
    if (!ifs.is_open()) {
      ec = std::make_error_condition(std::errc::bad_file_descriptor);
      return ""s;
    }

    ifs.seekg(0, ifs.end);
    if (ifs.bad()) {
      ec = std::make_error_condition(std::errc::io_error);
      return ""s;
    }

    std::string content(ifs.tellg(), char{});

    ifs.seekg(0, ifs.beg);
    ifs.read(content.data(), content.size());
    if (ifs.bad()) {
      ec = std::make_error_condition(std::errc::io_error);
      return content;
    } else {
      ec.clear();
    }

    return content;
  }

  inline std::string to_string(const std::string &path) {
    std::error_condition ec{};
    auto ret = to_string(path, ec);

    if (ec) {
      throw std::runtime_error("Reading File "s + path + " error "s + ec.message());
    }

    return ret;
  }

  inline void from_string(const std::string &path, const std::string &content, std::error_condition &ec) {

    boost::filesystem::create_directories(boost::filesystem::path(path).parent_path());

    std::fstream ofs(path, std::ios::out | std::ios::trunc | std::ios::in | std::ios::binary);
    if (!ofs.is_open()) {
      ec = std::make_error_condition(std::errc::bad_file_descriptor);
      return;
    }

    ofs.write(content.data(), content.size());
    if (ofs.bad()) {
      ec = std::make_error_condition(std::errc::io_error);
      return;
    } else {
      ec.clear();
    }
  }

  inline void from_string(const std::string &path, const std::string &content) {
    std::error_condition ec{};
    from_string(path, content, ec);

    if (ec) {
      throw std::runtime_error("Writing File "s + path + " error "s + ec.message());
    }
  }

}
