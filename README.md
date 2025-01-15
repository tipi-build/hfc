# tipi.build HermeticFetchContent

Enhanced and compatible `FetchContent` with source+build caching, install tree caching and  

Usable with either `cmake` or `cmake-re` this CMake module provides an 
integration of  Third-party libraries into the project's CMake build system 
giving better control (in regard to especially toolchain matter).

## Usage

* ➡ Add `include(HermeticFetchContent)` to the CMakelists
* 📘 Learn more in the [reference documentation](https://tipi-build.github.io/hfc/)
* 🚀 [Get Started](./example/get-started/)
```cmake
# Bootstrap hfc : Copy this after your project() call
include(FetchContent)
FetchContent_Populate(hfc
  GIT_REPOSITORY https://github.com/tipi-build/hfc.git
  GIT_TAG main
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/src"
  SUBBUILD_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/subbuild"
  BINARY_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache/hfc/bin"
)
FetchContent_GetProperties(hfc)
list(APPEND CMAKE_MODULE_PATH "${hfc_SOURCE_DIR}/cmake")
include(HermeticFetchContent)
```

The different dependencies are made available to the project's targets as 
libraries and executables that can be "target linked" or "found" as one would 
normally expect in a modern CMake build system.

### Hermeticity

When used with `cmake` the dependency builds are placed as content builds in 
isolated source / build / install trees in a folder structure under
`<build-prefix>/_deps/<content-name>-<src|build|install|...>`.
 
The hermeticity extends to the level that the dependency builds are not aware
of each other *unless* they are made `find_package`-able explicitely using 
the options in the `FetchContent_MakeHermetic()` declaration for each
respective dependency.

### Build Graph reorganization

Depending on the selection of either being made available at build time or 
configure time by using:

  * `HermeticFetchContent_MakeAvailableAtBuildTime(<content-name>)`
  * `HermeticFetchContent_MakeAvailableAtConfigureTime(<content-name>)`

The dependency will be either just configured or build and configured during the
project's configure phase. The former case guarentees best performance when 
running the builds because dependencies can be built as needed (as they are 
fully integrated in the build graph).

### Cacheability

Relying targets scanning technique to enable depending only on FetchContent 
install/ trees, when used with `cmake-re` the builds of the dependencies are
executed in a manner that enables them to be cached independently of the project
to improve cache hits and library build reeuses on the L1 cache (by using
 `cmake-re`'s mirroring and caching technology).

This feature allows to save time needlessly spent on CI runs checking out code
of thirdparties instead of building changes.

### SBOM

Hermetic FetchContent provides an out of the box integration with 
[cmake-SBOM by DEMCON](https://github.com/DEMCON/cmake-sbom.git) (which can be 
either auto-bootstrapped or provided by your project - if `include(sbom)` works
the provided version is used)

The feature is enabled by default but can be disabled by setting the option
`HFC_ENABLE_CMAKE_SBOM` to `OFF`

To add information about licences and author to the metadata you can:

```cmake
FetchContent_MakeHermetic(
  <content_name>
  [...snip...]
  SBOM_LICENSE "MIT License"
  SBOM_SUPPLIER "Mr. Wonderful"
)
```

Here's how to trigger the actual SBOM generation (should be placed after all dependencies were registered)

```cmake
include(hfc_sbom)
hfc_generate_sbom(
  OUTPUT "my-awesome-sbom.spdx"
  LICENSE "Proprietary"
  SUPPLIER "You"
  SUPPLIER_URL "your-url.com"
  NO_VERIFY
)
```

### How does it compare to CPM ?
CMake FetchContent popularity is constantly growing with projects like CPM or cmake-init.

However plain FetchContent or CPM lacks serious features :
  - Only works with high quality CMake dependencies: OpenSSL, autotools build systems and real-life CMake projects with no consideration for FetchContent aren't supported
  - It needs to clone all sources on builds
    * `hfc` can fetch only the install tree of a dependency when `cmake-re` is enabled
  - It builds everything from source every time : no build caching
  - Needs polluting main projet and descendent variable scope to configure dependencies
    * If one wants to configure 2 dependencies using the same option name with different value it can be problematic.
  - Missing support for dependencies traceability and SBOMs

With **HermeticFetchContent** we solve this by enhancing FetchContent with :
#### Hermeticity
  - Dependencies are made available to each other selectively
  - FetchContent libraries that can find_package their dependencies
  - If cmake-re in use absolute hermeticity guaranteed

#### Cacheability
When run within cmake-re : If an install cache is available it doesn't even pull the sources
If pointed to an HERMETIC_FETCHCONTENT_INSTALL_DIR, it can reuse install tree locally also in plain CMake. With `cmake-re` it pulls it automatically from your user or organization cache. 

#### Build Graph Reorganization
  - Choose wether the library needs to be built before project builds or mixed in your build graph :
    - `HermeticFetchContent_MakeAvailableAtBuildTime()`
    - `HermeticFetchContent_MakeAvailableAtConfigureTime()`

#### SBOMs
  - Run the hfc_generate_sbom and you will get you a full SBOM in SPDX format

## Developer Guide

You can run the HermeticFetchContent test suite this way:

  * `cmake -S . -B build/ -DCMAKE_TOOLCHAIN_FILE=toolchain/toolchain.cmake -GNinja` **[*]** 
  * `cmake --build build/`
  * `ctest -R check_options_and_defines_forwarding`


**[*]** Additionally the following variable can be defined to ease development:

  * `export HFC_TEST_SHARED_TOOLS_DIR=$PWD/thirdparty/cache/.hfc_tools`

  * `HFC_PROJECT_HFC_SOURCE_CACHEDIR` to use a common source cache for the builds of the test project 
  * `HFC_PROJECT_HFC_BASEDIR` to use a common "HFC base dir" (which will contain the dependency install trees)
  * `HERMETIC_FETCHCONTENT_TOOLS_DIR` if you have one, avoid rebuilding `goldilock` too frequently, by default a `${CMAKE_BINARY_DIR}/.hfc_tools/` directory will be created

## Authors

Authored with love, blood and tears in Zürich, Switzerland by team **[tipi.build](https://tipi.build)**.

Parts of `HermeticFetchContent` rely on CMake modules extracted from Ruslan Baratov's [hunter](https://github.com/ruslo/hunter) project (BSD 2-Clause "Simplified" License: [link](https://github.com/ruslo/hunter/blob/master/LICENSE)).
