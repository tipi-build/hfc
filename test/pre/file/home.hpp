#pragma once

#include <boost/predef.h>
#include <string>

namespace pre::file {

  //! Returns current user home directory
  inline std::string get_home_dir() {
#if BOOST_OS_WINDOWS
    return std::string(std::getenv("HOMEDRIVE")) + std::string(std::getenv("HOMEPATH"));
#else // Any UNIXes
    return std::string(std::getenv("HOME"));
#endif
  }
}
