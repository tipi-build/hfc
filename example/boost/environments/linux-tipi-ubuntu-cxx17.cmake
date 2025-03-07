find_program(CMAKE_C_COMPILER clang)
find_program(CMAKE_CXX_COMPILER clang++)

if(NOT CMAKE_C_COMPILER)
  message(FATAL_ERROR "clang not found")
endif()

if(NOT CMAKE_CXX_COMPILER)
  message(FATAL_ERROR "clang++ not found")
endif()

set(
    CMAKE_C_COMPILER
     "${CMAKE_C_COMPILER}"
    CACHE
    STRING
    "C compiler"
    FORCE
)

set(
    CMAKE_CXX_COMPILER
     "${CMAKE_CXX_COMPILER}"
    CACHE
    STRING
    "C++ compiler"
    FORCE
)

set(CMAKE_CXX_FLAGS_INIT "-std=c++17" CACHE STRING "" FORCE)  
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ Standard (toolchain)" FORCE)
set(CMAKE_CXX_STANDARD_REQUIRED YES CACHE BOOL "C++ Standard required" FORCE)
set(CMAKE_CXX_EXTENSIONS NO CACHE BOOL "C++ Standard extensions" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "fPIC" FORCE)