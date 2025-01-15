.. title:: HermeticFetchContent / Recipes / How to define build Environments

How to define build Environments
################################

In order for the *Hermetic* aspect of Hermetic FetchContent to work as expected, it is mandatory
to extract some information that we consider to be *environment specific* from the project's 
``CMakeLists.txt`` files and to unify this as part of a "Hermetic Environment Definition".

At its core, this approach utilizes CMake's ``TOOLCHAIN_FILE`` concept, but does so in a way that enables 
more benefits once used in conjunction with ``cmake-re`` and/or ``tipi.build``.


Environment structure
^^^^^^^^^^^^^^^^^^^^^

In your project, the location for environment specifications would usually be under ``<project-root>/toolchains``.

Here's what the typical structure of a Hermetic FetchContent build environment looks like::

  <environment-folder>/<name>.cmake  
  <environment-folder>/environment/*

The ``<name>.cmake`` file is the ``-DCMAKE_TOOLCHAIN_FILE`` argument used in your
configure settings and should contain basic toolchain settings (an example is provided below).

The ``environment/`` sub-folder is meant to contain toolchain fragments that can be included by
the toolchain file.

Additionally, for `tipi` support, you may want to add the following entries:

- ``<environment-folder>/cmake/test-injector.cmake``
- ``<environment-folder>/<name>.pkr.js/<name>.pkr.js``

More details on these components are provided below.


Example
^^^^^^^

Below a simple example of a toolchain using the system-provided `GCC-13` installation:


In file `example/example-gcc13.cmake`

.. code-block:: cmake

  include("${CMAKE_CURRENT_LIST_DIR}/environment/base.cmake") 

  # Refine behaviour by giving CMAKE_FIND_ROOT_PATH priority to our install_prefix
  get_filename_component(current_source_absolute_path "${CMAKE_CURRENT_LIST_DIR}/../../"  ABSOLUTE)
  list(INSERT CMAKE_FIND_ROOT_PATH 0 "${current_source_absolute_path}/toolchains/" )
  list(APPEND CMAKE_PREFIX_PATH "${current_source_absolute_path}/toolchains/" )

  # configure find_* behavior to get to a hermetic state
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)


In file `example/environment/base.cmake`

.. code-block:: cmake

  set(CMAKE_SYSTEM_NAME Linux)
  set(CMAKE_HOST_SYSTEM_NAME Linux)
  set(CMAKE_CROSSCOMPILING FALSE)
  set(CMAKE_SYSTEM_VERSION 1)
  set(CMAKE_HOST_SYSTEM_PROCESSOR x86_64)
  set(CMAKE_SYSTEM_PROCESSOR x86_64)

  include("${CMAKE_CURRENT_LIST_DIR}/gcc-13.cmake")
  set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "" FORCE)
  set(CMAKE_CXX_STANDARD "17" CACHE STRING "" FORCE)



In file `example/environment/gcc-13.cmake`

.. code-block:: cmake

  find_program(CMAKE_C_COMPILER gcc PATHS "/usr/bin/;/usr/local/bin/" NO_DEFAULT_PATH)
  find_program(CMAKE_CXX_COMPILER g++ PATHS "/usr/bin/;/usr/local/bin/" NO_DEFAULT_PATH)

  if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR "gcc not found")
  endif()

  if(NOT CMAKE_CXX_COMPILER)
    message(FATAL_ERROR "g++ not found")
  endif()

  if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 13)
    message(FATAL_ERROR "Wrong version of g++ found (expected 13.x found ${CMAKE_CXX_COMPILER_VERSION})")
  endif()

  set(CMAKE_C_COMPILER "${CMAKE_C_COMPILER}" CACHE STRING "C compiler" FORCE)
  set(CMAKE_CXX_COMPILER "${CMAKE_CXX_COMPILER}" CACHE STRING "C++ compiler" FORCE)


CMake-RE and tipi remote build support
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To make these environments compatible with remote builds using `cmake-re` and the `tipi.build` cloud, 
additional configurations are required. 

By adding a specific cmake module to ``<environment-folder>/cmake/test-injector.cmake``, you can enable 
support for `cmake-re`'s ``--test`` option (and related parameters).

.. code-block:: cmake

  include_guard(GLOBAL) #!important!

  macro(add_test)
      separate_arguments(separated_args NATIVE_COMMAND $ENV{TIPI_TESTS_ARGUMENTS})
      set(tat_params ${ARGV})
      list(APPEND tat_params ${separated_args})    
      _add_test(${tat_params})
  endmacro()

This macro allows `cmake-re` to inject test arguments into the `ctest` call chain.

Additionally, you can specify the Docker image used as the host environment for remote `cmake-re` builds. 
To do this, replace the `<container URL>` placeholder in the template below with the appropriate Docker 
image URL. It's important to place this file at ``<environment-folder>/<name>.pkr.js/<name>.pkr.js``. 

Ensure that `<name>` matches exactly with the name of the environment or toolchain file.

.. code-block:: json
  
  {
    "variables": { },
    "builders": [
      {
        "type": "docker",
        "image": "<container URL>",
        "commit": true
      }
    ],
    "post-processors": [
      { 
        "type": "docker-tag",
        "repository": "linux",
        "tag": "latest"
      }
    ],
    "_tipi_version":"{{tipi_version_hash}}"
  }


The container URL should be a docker registry image URL including its tag or hash.


