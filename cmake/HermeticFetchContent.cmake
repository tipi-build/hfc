# Hermetic FetchContent

#[=======================================================================[.rst:
FetchContent_MakeHermetic
-------------------------

.. only:: html

   .. contents::


Set the configuration variables of Hermetic FetchContent for the content


Overview
^^^^^^^^

This module allows to setup an Hermetic FetchContent configuration for a given content,
augmenting what FetchContent can do by enabling consuming dependencies that are configured
in a separate cmake execution.


The following shows a typical example of declaring content detail for an examplary dependency:

.. code-block:: cmake

  include(HermeticFetchContent)

  # Hermetic FetchContent consumes information of FetchContent contents declared using the same name
  FetchContent_Declare(
    zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG 63779c798237346c2b245c546c40b72a5a5913fe
    SOURCE_SUBDIR build/cmake
  )

  # additional Hermetic FetchContent configuration
  FetchContent_MakeHermetic(
    zstd
    HERMETIC_BUILD_SYSTEM cmake
  )

  # choose when this content gets build
  HermeticFetchContent_MakeAvailableAtConfigureTime(zstd)
  # OR
  HermeticFetchContent_MakeAvailableAtBuildTime(zstd)


Hermetic FetchContent does consume information defined in `FetchContent_Declare` calls whenever possible
but enables additional configuration through the :command:`FetchContent_MakeHermetic` command.

Commands
^^^^^^^^

.. command:: FetchContent_MakeHermetic

  .. code-block:: cmake

    FetchContent_MakeHermetic(
      <name>
      [HERMETIC_BUILD_SYSTEM cmake | autotools | openssl]
      [HERMETIC_TOOLCHAIN_EXTENSION <cmake code>]
      [HERMETIC_FIND_PACKAGES <list of hermetic content names>]
      [HERMETIC_CREATE_TARGET_ALIASES <cmake code>]
      [HERMETIC_PREPATCHED_RESOLVER <cmake code>]
      [HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION <cmake code>]
      [HERMETIC_DISCOVER_TARGETS_FILE_PATTERN <regex pattern>]
    )

  The ``FetchContent_MakeHermetic()`` function records options that describe the additional
  parameters required to populate and consume the specified content in a hermetic way.

  The content ``<name>`` can be any string without spaces, but good practice
  would be to use only letters, numbers and underscores.

  The ``HERMETIC_BUILD_SYSTEM`` allows selecting which build system scheme will be used to
  configure and build the specified content. Currently the available choices are:

    ``cmake``
      For contents that come with a working CMake buildsystem

    ``autotools``
      For contents using the ``GNU Autotools`` as build system

    ``openssl``
      To consume the native openssl buildsystem

  The ``HERMETIC_TOOLCHAIN_EXTENSION`` option enables injecting code into the toolchain used
  in the isolated build of the specified content, enabling to set the required build configuration
  without interfering with the build configuration of other parts of your project.

  .. code-block:: cmake

    FetchContent_MakeHermetic(
      rapidjson
      HERMETIC_BUILD_SYSTEM cmake
      HERMETIC_TOOLCHAIN_EXTENSION [=[
        # don't build doc, examples & tests
        set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
        set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)

        # use std::string
        set(RAPIDJSON_HAS_STDSTRING ON CACHE BOOL "" FORCE)
      ]=]
    )

  Additionally to these dependency specific toolchain fragments Hermetic FetchContent is able
  to forward a defined set of CMake variables to the sub-builds by injecting values of the variables
  specified in the global ``HERMETIC_FETCHCONTENT_FORWARDED_CMAKE_VARIABLES`` into the
  generated proxy toolchain files:

  .. code-block:: cmake

    set(HERMETIC_FETCHCONTENT_FORWARDED_CMAKE_VARIABLES
      "CMAKE_BUILD_TYPE"
      "CMAKE_EXE_LINKER_FLAGS"
      "CMAKE_CXX_FLAGS"
      "CMAKE_C_FLAGS"
    )


  The ``HERMETIC_FIND_PACKAGES`` option enables specifying which packages that are provided
  by other Hermetic FetchContent declarations will be able to be ``find_package``-ed by the
  content being specified. This enables controlling the packages available in the individual
  builds by other means than by just order of declaration.

  In this example, the library ``libxml2`` gets to ``find_package()`` ``Ìconv`` and ``ZLIB``.
  These libraries have to be made available using Hermetic FetchContent in the same build.
  Such a call to ``find_package()`` will fail for other packages that are not explicitely
  whitelisted, either using the ``HERMETIC_FIND_PACKAGES`` option OR by adding said library
  to the global ``HERMETIC_FETCHCONTENT_BYPASS_PROVIDER_FOR_PACKAGES`` setting which allows
  ``find_package()`` to fall-back to CMake's native implementation if it cannot resolve the
  package among the whitelisted Hermetic FetchContent builds.

  .. code-block:: cmake

    # omitting the FetchContent_Declare() call

    FetchContent_MakeHermetic(
      LibXml2
      HERMETIC_BUILD_SYSTEM cmake
      HERMETIC_FIND_PACKAGES "Iconv;ZLIB" # <- this
      HERMETIC_TOOLCHAIN_EXTENSION [=[
        set(LIBXML2_WITH_LZMA OFF)
        set(LIBXML2_WITH_PYTHON OFF)
      ]=]
    )

  The ``HERMETIC_CREATE_TARGET_ALIASES`` options allows defining aliases for target during
  the configure phase. The CMake code provided will be invoked for every CMake target library
  that is exported by the content. The scope in which this code will be run contains the
  variable ``TARGET_NAME`` which will be set to the exported target name. The provided code
  fragment **must** set the ``list`` ``TARGET_ALIASES`` to contain **all** the names under
  which the exported target will be imported when it is consumed.

  It is recommended to test for specific values of ``TARGET_NAME`` instead of doing blanket
  declarations of aliases "no matter the input" to avoid issues in cases where a library might
  start to provide multiple exported libraries depending on the build configuration or just over
  time.

  In the example below, we will be renaming the target ``ZLIB::zlib`` that is exported by the
  project to ``ZLIB::ZLIB`` (notice the difference in casing). This can be required if for
  example another project is unable to find the library in it's original casing.

  .. code-block:: cmake

    FetchContent_Declare(
      ZLIB
      GIT_REPOSITORY https://github.com/cpp-pm/zlib.git
      GIT_TAG 57af136e436c5596e4f1c63fd5bdd2ce988777d1
    )

    FetchContent_MakeHermetic(
      ZLIB
      HERMETIC_BUILD_SYSTEM cmake
      HERMETIC_CREATE_TARGET_ALIASES [=[
        if("${TARGET_NAME}" STREQUAL "ZLIB::zlib")
          set(TARGET_ALIASES "ZLIB::ZLIB")
        endif()
      ]=]
    )

    HermeticFetchContent_MakeAvailableAtConfigureTime(ZLIB)


  The option ``HERMETIC_PREPATCHED_RESOLVER`` enables defining a lookup method for source population.
  This is particularly useful when used in combination with the ``PATCH_COMMAND`` setting in ``FetchContent_Declare()``
  because it allows the developper to store a pre-patched version of the source code of a library in a central
  location to avoid the burden of running potentially lengthy patch operations on every build while still retaining
  the capability of quickly trying out other versions of a dependency during upgrade trials.

  As for the previous option Hermetic FetchContent expects ``HERMETIC_PREPATCHED_RESOLVER`` to contain an executable
  fragment of CMake code (as a string) that is called in a isolated scope that contains the following variables with
  the values defined in ``FetchContent_Declare``:

    ``GIT_REPOSITORY``
      URL of the git repository. Any URL understood by the git command may be used.

    ``GIT_TAG``
      Git branch name, tag or commit hash to be checked out

    ``URL``
      List of paths and/or URL(s) of the external project's source.

    ``URL_HASH``
      Hash of the archive file to be downloaded. Should be of the form <algo>=<hashValue> where algo can be any of
      the hashing algorithms supported by the file() command.

  The CMake code can redefine any of these variables to change the source of the content.
  The CMake code must set ``RESOLVED_PATCH`` to a truthy value if it is able to set an alternate source for the content.

  If ``RESOLVED_PATCH`` is set to a truthy value the source populate operation will rely on the new information
  provided an skip the execution of the ``PATCH_COMMAND``. Otherwise the normal expected behavior is executed (e.g.
  downloading from the original source and running any specified ``PATCH_COMMAND``)

  In the following example we will assume that the project is storing a patched copy of the specified repository
  and revision of the original sources in it own github organization under a given commit ID:

  .. code-block:: cmake

    FetchContent_Declare(
      boost
      GIT_REPOSITORY https://github.com/boostorg/boost.git
      # that's v1.84
      GIT_TAG        ad09f667e61e18f5c31590941e748ac38e5a81bf
      # apply the patch "some.patch"
      PATCH_COMMAND cmake -E patch < ${CMAKE_CURRENT_LIST_DIR}/patchtest/some.patch
    )

    FetchContent_MakeHermetic(
      boost
      HERMETIC_BUILD_SYSTEM cmake
      HERMETIC_PREPATCHED_RESOLVER [=[
        if(${GIT_TAG} STREQUAL "ad09f667e61e18f5c31590941e748ac38e5a81bf")
          # hypothetical <my-org>/boost-prepatched.git repo
          set(GIT_REPOSITORY "https://github.com/<my-org>/boost-prepatched.git")
          set(GIT_TAG "9c83f4a27e4227dbe02e4a47ede372ac2a4a043e")
          set(RESOLVED_PATCH TRUE)
        endif()
      ]=]
    )

    HermeticFetchContent_MakeAvailableAtBuildTime(boost)


  The developer can use the options ``HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION``  to provide a
  a library export declaration that will enable Hermetic FetchContent to consume the targets
  as-if the project had provided a proper package configuration. This is useful in cases in which
  no such information is available (for example in the case of `HERMETIC_BUILD_SYSTEM == autotools`
  or `HERMETIC_BUILD_SYSTEM == openssl` or if the project's CMake buildsystem does not install and
  export targets)

  The following example shows how to define the ``Pcap::Pcap`` target properties required
  for Hermetic FetchContent to be able to construct a so-called "targets cache":

  .. code-block:: cmake

    FetchContent_MakeHermetic(
      Pcap
      HERMETIC_BUILD_SYSTEM autotools
      HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION
        [=[
          add_library(Pcap::Pcap STATIC IMPORTED)
          set_property(
            TARGET Pcap::Pcap
            PROPERTY IMPORTED_LOCATION
            "@HFC_PREFIX_PLACEHOLDER@/lib/libpcap.a"
          )
          set_property(
            TARGET Pcap::Pcap
            PROPERTY INTERFACE_INCLUDE_DIRECTORIES
            "@HFC_PREFIX_PLACEHOLDER@/include"
          )
        ]=]
    )

  The following template variables are available to define the targets:

  - ``@HFC_SOURCE_DIR_PLACEHOLDER@`` which will be replaced with the source directory location when the library is consumed later on.
  - ``@HFC_BINARY_DIR_PLACEHOLDER@`` which will be replaced with the binaries directory location when the library is consumed later on.
  - ``@HFC_PREFIX_PLACEHOLDER@`` which will be replaced with the final install prefix location when the library is consumed later on.

  In cases in which the dependencie's CMake build system does provide target exports files
  but is not complying to the common file nameming scheme for those exports (hermetic fetchContent
  uses the following by default ``([Tt]argets|[Ee]xport(s?))\.cmake``), another pattern can be
  supplied using the ``HERMETIC_DISCOVER_TARGETS_FILE_PATTERN`` option.

.. command:: HermeticFetchContent_MakeAvailableAtBuildTime

  .. code-block:: cmake

    HermeticFetchContent_MakeAvailableAtBuildTime(
      <name>
    )

  The ``HermeticFetchContent_MakeAvailableAtBuildTime()`` function makes the content ``<name>`` available
  at build time (basically only running the content's configure step) an exporting the required targets.

  After this call the content's targets are available for target linking and dependency management.
  Build graph matters (e.g. when and what to build) are left to the project's build system.

.. command:: HermeticFetchContent_MakeAvailableAtConfigureTime

  .. code-block:: cmake

    HermeticFetchContent_MakeAvailableAtConfigureTime(
      <name>
    )

  The ``HermeticFetchContent_MakeAvailableAtConfigureTime()`` function makes the content ``<name>`` available
  to the project an other Hermetic FetchContent contents by running the complete build when this statement
  gets executed.

  This is particularly useful when integrating with other build systems that cannot be integrated in the
  project's build graph seamlessly and that - thus - require headers and libraries to be available in an
  installed location during *their* configure step.


.. command:: HermeticFetchContent_SetBaseDir

  .. code-block:: cmake

    HermeticFetchContent_SetBaseDir(
      <directory>
    )

  Set the base directory for all the hermetic dependency build directory and related folders

Fallback to system provided dependencies
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For use-cases in which you want to have the option to revert to a system provided library but still have the ability
to build your own, you may set ``FORCE_SYSTEM_<content-name>`` to ``ON``. In this case Hermetic FetchContent will
execute a CMake ``find_package(<content-name> REQUIRED)`` call during targets discovery. 

If you need to specify arguments to ``find_package()`` you may add the ``FIND_PACKAGE_ARGS`` to the content declaration.

  .. code-block:: cmake
    set(FORCE_SYSTEM_boost ON) # <- take boost from the system, toggle this to OFF to have Boost built by your project
    FetchContent_Declare(
      boost
      GIT_REPOSITORY https://github.com/boostorg/boost.git
      GIT_TAG        ad09f667e61e18f5c31590941e748ac38e5a81bf   # that's v1.84
    )

    FetchContent_MakeHermetic(
      boost
      HERMETIC_BUILD_SYSTEM cmake
      FIND_PACKAGE_ARGS "1.84 EXACT REQUIRED" # <- makes sure we take a 1.84 only from the system!
    )

    HermeticFetchContent_MakeAvailableAtBuildTime(boost)

Note that in the special case of ``FORCE_SYSTEM_<content-name>=ON`` the context in which the find_package() will be
run, will be slightly different from the normal context for hermetic builds. One major difference is that the Hermetic
FetchContent will inject the project's ``CMAKE_MODULE_PATH`` into the toolchain extension in addition to forcing 
``find_package()`` to search exclusively on the system through setting ``CMAKE_FIND_ROOT_PATH_MODE_*`` appropriately.

Build introspection
^^^^^^^^^^^^^^^^^^^

HermeticFetchContent adds a number of cmake build targets to the build system that can be executed after the project configuration to enable
some build introspection:

.. command:: build target "hfc_list_dependencies_build_dirs"

Prints all project's dependencies build directories to the console. Warning! This list might be incomplete if the project build at hand
did not actually build the whole list of dependencies

.. command:: build target "hfc_list_dependencies_install_dirs"

Prints all project's dependencies install directories to the console. Warning! Depending on the build state, the listed install trees
might not be populated (yet).

.. command:: build target "hfc_echo_${content_name}_install_dir"

Prints the install directory for ``${content_name}`` on the console. Warning! Depending on the build state, the install tree might not be populated (yet).

.. command:: build target "hfc_list_${content_name}_STATIC_LIBRARIES_locations"

Prints the list of static libraries of ``${content_name}`` on the console.

.. command:: build target "hfc_list_${content_name}_SHARED_LIBRARIES_locations"

Prints the list of shared libraries of ``${content_name}`` on the console.

.. command:: build target "hfc_list_${content_name}_MODULE_LIBRARIES_locations"

Prints the list of module libraries of ``${content_name}`` on the console.

.. command:: build target "hfc_list_${content_name}_UNKNOWN_LIBRARIES_locations"

Prints the list of 'unknown type' libraries of ``${content_name}`` on the console.

.. command:: build target "hfc_list_${content_name}_EXECUTABLES_locations"

Prints the list of executables of ``${content_name}`` on the console.


Rationale
^^^^^^^^^

Ressembling add_subdirectory through external cmake execution is key to providing reusable install trees :
  * One project depending on a library can reuse the library built by another project
  * The only common element has to be the common CMAKE_TOOLCHAIN_FILE settings

Finally it allows a build graph optimization to be at maximum parallelity. Indeed usual CMake practices for externally built
libraries found via `find_package` allows reusing the same libraries across project, but requires building them in advance
before the configure of the dependent project might even be started. However not all target depend directly of the said dependencies.

With this techniques it can integrate the clean build of the dependencies in the graph, effectively building at the finest granularity
of dependencies everything  that isn't dependent of the dependencies.  Possibly allowing building earlier parts of the project that do
not depend on the dependency itself.

Plain add_subdirectory allows this but lacks the isolation from the parent CMake project, hindering the reuse of the dependent
library install tree without the presence of the sources, which is a big speedup in CI workflows, and new developer computer setup,
optimizing avoid the need of cloning the actual dependencies source files with all the git history and every private implementation files.

#]=======================================================================]


include_guard()

#
# Hermetic FetchContent options


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/modules")
include(hfc_log)
include(hfc_initialize)

hfc_initialize("${CMAKE_CURRENT_LIST_DIR}")

include(FetchContent)
include(hfc_saved_details)
include(hfc_make_available_single)
include(hfc_goldilock_helpers)
include(hfc_provide_dependency_FETCHCONTENT)

#
# Save the declaration details for this Hermetic Content
function(FetchContent_MakeHermetic content_name)
  string(TOLOWER ${content_name} contentNameLower)
  set(savedDetailsPropertyName "_HermeticFetchContent_${contentNameLower}_savedDetails")
  get_property(alreadyDefined GLOBAL PROPERTY ${savedDetailsPropertyName} DEFINED)
  if(alreadyDefined)
    return()
  endif()

  __FetchContent_getSavedDetails(${content_name} __fetchcontent_arguments)

  # extract som of the Generic FetchContent details so we can patch in information

  list(LENGTH __fetchcontent_arguments __fetchcontent_arguments_len)
  set(__fetchcontent_arguments_ix 0)

  set(SOURCE_DIR_VALUE)
  set(CONTENT_SOURCE_HASH)

  while(__fetchcontent_arguments_ix LESS __fetchcontent_arguments_len)
    list(GET __fetchcontent_arguments ${__fetchcontent_arguments_ix} __cmake_item)
    MATH(EXPR next_ix "${__fetchcontent_arguments_ix}+1")

    # we want to potentially rewrite the SOURCE_DIR property that is automatically set by
    # fetchcontent in order to place the source directory in the source cache location
    # set using
    if(__cmake_item STREQUAL "SOURCE_DIR")
      list(GET __fetchcontent_arguments ${next_ix} SOURCE_DIR_VALUE)

      # /!\ we're jumping this as we will be computing the entry of
      # SOURCE_DIR = <value> in a couple of lines
      MATH(EXPR next_ix "${next_ix}+1")
      set(__fetchcontent_arguments_ix ${next_ix})
      continue()

    # just getting the value for the GIT_TAG
    elseif(__cmake_item STREQUAL "GIT_TAG")
      list(GET __fetchcontent_arguments ${next_ix} CONTENT_SOURCE_HASH)
    elseif(__cmake_item STREQUAL "URL_HASH")
      list(GET __fetchcontent_arguments ${next_ix} content_URL_HASH)

      if(content_URL_HASH MATCHES ".+:.+")
        string(REGEX REPLACE ".+:" "" CONTENT_SOURCE_HASH "${content_URL_HASH}")
      else()
        string(SHA1 CONTENT_SOURCE_HASH "${content_URL_HASH}") # it's weird so we hash it to know what we got
      endif()
    endif()

    string(APPEND __cmdArgs " [==[${__cmake_item}]==]")
    MATH(EXPR __fetchcontent_arguments_ix "${__fetchcontent_arguments_ix}+1")
  endwhile()

  # now we can compute the source clone name and SOURCE_DIR property value
  #
  # we are combining the content name with a short hash of the dependency origin
  # to deal with the patching and changing sources without having to
  # deal with git remotes by hand...
  string(SUBSTRING "${CONTENT_SOURCE_HASH}" 0 8 source_hash_short)
  set(clone_dir_name "${content_name}-${source_hash_short}-src")

  # We need to know if this is an autotools/openssl build to supply an BUILD_IN_SOURCE_TREE parameter
  # so that this gets populated into the vicinities of the build directory and not into the clone cache
  cmake_parse_arguments(FN_ARG "" "HERMETIC_BUILD_SYSTEM" "" ${ARGN})
  if("${FN_ARG_HERMETIC_BUILD_SYSTEM}" STREQUAL "autotools" OR "${FN_ARG_HERMETIC_BUILD_SYSTEM}" STREQUAL "openssl")
    cmake_path(GET SOURCE_DIR_VALUE PARENT_PATH dest_parent_path)
    string(APPEND __cmdArgs " [==[BUILD_IN_SOURCE_TREE]==]")
    string(APPEND __cmdArgs " [==[ON]==]")
  endif()

  set(SOURCE_DIR_VALUE "${HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR}/${clone_dir_name}")

  string(APPEND __cmdArgs " [==[SOURCE_DIR]==]")
  string(APPEND __cmdArgs " [==[${SOURCE_DIR_VALUE}]==]")

  # Store HERMETIC_ details
  foreach(__item IN LISTS ARGN)
    string(APPEND __cmdArgs " [==[${__item}]==]")
  endforeach()

  define_property(GLOBAL PROPERTY ${savedDetailsPropertyName})
  cmake_language(EVAL CODE
    "set_property(GLOBAL PROPERTY ${savedDetailsPropertyName} ${__cmdArgs})"
  )
endfunction()


#
# HermeticFetchContent_MakeAvailableAtBuildTime
function(HermeticFetchContent_MakeAvailableAtBuildTime)
  hfc_initialize_enable_cmake_re_if_requested()

  foreach(content_name IN ITEMS ${ARGV})
    hfc_make_available_single(${content_name} OFF)
  endforeach()
endfunction()

#
# HermeticFetchContent_MakeAvailableAtConfigureTime
function(HermeticFetchContent_MakeAvailableAtConfigureTime)
  hfc_initialize_enable_cmake_re_if_requested()

  foreach(content_name IN ITEMS ${ARGV})
    hfc_make_available_single(${content_name} ON)
  endforeach()
endfunction()


macro(hfc_FetchContent_MakeAvailable_interlocked)

  foreach(content_name IN ITEMS ${ARGV})

    __FetchContent_getSavedDetails(${content_name} __fetchcontent_arguments)
    hfc_provide_dependency_FETCHCONTENT("FETCHCONTENT_MAKEAVAILABLE_SERIAL" ${content_name} ${__fetchcontent_arguments})
    string(TOLOWER "${content_name}" content_name_lower)

  endforeach()

  unset(__fetchcontent_arguments)
  unset(content_name_lower)
endmacro()

#
# HermeticFetchContent_SetBaseDir
macro(HermeticFetchContent_SetBaseDir directory)

  get_filename_component(dependency_base_dir "${directory}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

  get_property(alread_made_content_available GLOBAL PROPERTY HERMETIC_FETCHCONTENT_MADE_CONTENT_AVAILABLE SET)

  if(alread_made_content_available)
    message(FATAL_ERROR "Cannot HermeticFetchContent_SetBaseDir() after the first call to HermeticFetchContent_MakeAvailableAt...()")
  endif()

  #set(FETCHCONTENT_BASE_DIR "${dependency_base_dir}" CACHE INTERNAL "HFC's base dir")
  set(HERMETIC_FETCHCONTENT_INSTALL_DIR "${dependency_base_dir}" CACHE INTERNAL "HFC's dependencies installation dir")

endmacro()


#
# HermeticFetchContent_SetSouceCacheDir
macro(HermeticFetchContent_SetSouceCacheDir directory)

  get_filename_component(directory_abs "${directory}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

  get_property(alread_made_content_available GLOBAL PROPERTY HERMETIC_FETCHCONTENT_MADE_CONTENT_AVAILABLE SET)

  if(alread_made_content_available)
    message(FATAL_ERROR "Cannot HermeticFetchContent_SetSouceCacheDir() after the first call to HermeticFetchContent_MakeAvailableAt...()")
  endif()

  set(HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR "${directory_abs}" CACHE INTERNAL "Hermetic-FetchContent source cache dir")

endmacro()


include(hfc_targets_cache_alias)