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
# Just a very simple
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CROSSCOMPILING ON CACHE BOOL "" FORCE)
set(CMAKE_SYSTEM_NAME "Linux" CACHE STRING "" FORCE)
