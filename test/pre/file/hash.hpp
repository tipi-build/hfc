#pragma once

#include <stdexcept>
#include <system_error>
#include <fstream>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/uuid/detail/sha1.hpp>

namespace pre::file {

  using namespace std::string_literals;

  //! Calculate the SHA1 sum for a file on disk
  inline std::string sha1sum(const std::string &path, std::error_condition &ec) {

    std::ifstream ifs(path, std::ios::in | std::ios::binary);
    if (!ifs.is_open()) {
      ec = std::make_error_condition(std::errc::bad_file_descriptor);
      return ""s;
    }

    boost::uuids::detail::sha1 sha1;
    
    const size_t buffer_size = 4096;
    std::vector<char> buffer (buffer_size,0);
    
    while(ifs.read(&buffer[0], buffer_size)) {
      sha1.process_bytes(buffer.data(), buffer_size);
    }

    // consume the remainder
    sha1.process_bytes(buffer.data(), ifs.gcount());

    if (ifs.bad()) {
      ec = std::make_error_condition(std::errc::io_error);
      return "";
    } else {
      ec.clear();
    }

    // compute the hash
    uint32_t hash[5] = {0};
    char hash_buf[41] = {0};

    sha1.get_digest(hash);    

    for (int i = 0; i < 5; i++) {
      std::sprintf(hash_buf + (i << 3), "%08x", hash[i]);
    }

    return std::string(hash_buf);
  }

  //! Calculate the SHA1 sum for a file on disk
  //! Throws on error
  inline std::string sha1sum(const std::string &path) {
    std::error_condition ec{};
    auto ret = sha1sum(path, ec);

    if (ec) {
      throw std::runtime_error("Failed to compute SHA1 sum for file "s + path + ": error "s + ec.message());
    }

    return ret;
  }


}
