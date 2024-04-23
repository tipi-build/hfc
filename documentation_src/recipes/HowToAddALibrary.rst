.. title:: HermeticFetchContent / Recipes / Add a library


How to Add a Library with Hermetic FetchContent
###############################################

To add a new dependency to your build using Hermetic FetchContent, you first need to determine 
the build system used by the library. Currently, Hermetic FetchContent supports adding dependencies
that use either `CMake` or `GNU Autotools` as their primary build system.

The following sections provide walk-throughs for the most common scenarios, along with suggestions 
for parameters or workarounds applicable in typical situations.

Basics
^^^^^^

Hermetic FetchContent is an augmentation of CMake's native FetchContent module, providing enhanced 
control over project dependencies and their build settings. It offers a structured and more isolated
approach to declaring each dependency's build settings, reducing the risk of dependencies interfering
with each other (such as through conflicting options or settings).

A key difference is that in Hermetic FetchContent, builds of dependencies are "unaware" of each other 
*unless* explicitly made visible.

For dependencies using a modern CMake build system with comprehensive install targets and 
`package configuration` exports (not to be confused with `PkgConfig`, which Hermetic FetchContent 
does not currently support), integration is straightforward.


Libraries using CMake
^^^^^^^^^^^^^^^^^^^^^

Modern CMake Build System Available
===================================

First, declare the content settings of the library you wish to add. For example, `civetweb` can 
be fetched directly from its Git repository on *github.com*. The code below demonstrates this:

.. code-block:: cmake

  # Declare the dependency
  FetchContent_Declare(
    civetweb  # Content name 
    GIT_REPOSITORY https://github.com/civetweb/civetweb.git
    GIT_TAG ad35e1abd1657936452515046e74c6d266f513cf  # Specific commit ID
  )

The value of `content_name` as used in the first ``FetchContent_Declare()`` call has to stay constant in 
subsequent calls to ``FetchContent_MakeHermetic()`` and ``HermeticFetchContent_MakeAvailable*()``.

.. note:: Best Practice for Version Selection

  It's advisable to reference a specific commit ID, as shown above, instead of a `tag` or `branch`. 
  While less immediately readable, a commit ID represents an immutable revision, offering greater 
  security against manipulation from the repository hoster or library vendor.

Properties required for the Hermetic build of the library can be defined in a call to
``FetchContent_MakeHermetic()`` as follows:

.. code-block:: cmake

  FetchContent_MakeHermetic(
    civetweb
    HERMETIC_BUILD_SYSTEM cmake
    HERMETIC_TOOLCHAIN_EXTENSION [=[
      set(CIVETWEB_ENABLE_CXX ON CACHE BOOL "" FORCE)
      set(CIVETWEB_ALLOW_WARNINGS ON CACHE BOOL "" FORCE)
      set(CIVETWEB_CXX_STANDARD "c++17" CACHE STRING "" FORCE) 
      set(CIVETWEB_BUILD_TESTING OFF CACHE BOOL "" FORCE) 
      set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldl")
    ]=]
  )

The ``HERMETIC_BUILD_SYSTEM`` argument specifies that `civetweb` will use a `cmake` build scheme. 

Build options for `civetweb` are injected into its Hermetic build using the ``HERMETIC_TOOLCHAIN_EXTENSION`` 
argument. This content is appended to the content-specific proxy toolchain that Hermetic FetchContent 
generates for this build.

.. note:: CMake Language Bracket-Argument Notation for Multi-Line Strings

  Using "bracket argument" notation for multi-line strings in CMake (available since CMake 3) is highly practical. 
  This method, detailed in the 
  [CMake Language Documentation](https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#bracket-argument), 
  maintains all escape characters and spaces, eliminating the need for double escaping or explicit newline declarations.

Finally, to make the content's build targets available to your project, you can do so at either *configure time* or 
*build time*. The reasons and situations where each approach is required will be explained below. With 
``HermeticFetchContent_MakeAvailableAtBuildTime()`` being the more frequent choice in typical builds.

The process typically involves calling:

.. code-block:: cmake

  # make civetweb available as build time targets
  # only the content population and configuration steps will be executed at this point.
  HermeticFetchContent_MakeAvailableAtBuildTime(civetweb)

  #
  # OR
  #

  # make civetweb availabe at configure time
  # the full dependency build (population, configure, build and hermetic install) will
  # be run at this point
  HermeticFetchContent_MakeAvailableAtConfigureTime(civetweb)


At this point the targets exported by `civetweb` will be availabe for target linking
and consumption as any other CMake target.


Making One Dependency Available to Another
""""""""""""""""""""""""""""""""""""""""""

As mentioned in the introduction, a key distinction of using ``Hermetic FetchContent`` 
is that dependencies are built in isolation from each other.

In scenarios where one dependency needs to ``find_package()`` another, such an 
inter-dependency can be established using the ``HERMETIC_FIND_PACKAGES`` argument 
in ``FetchContent_MakeHermetic()``.

.. code-block:: cmake

  # after a prior declaration of the packages / contents "OpenSSL" and "ZLIB"
  # declaring the Hermetic FetchContent properties for curl (which depends on
  # both the above) would be as follows: 
  
  FetchContent_Declare(
    CURL
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG        47ccaa4218c408e70671a2fa9caaa3caf8c1a877 # curl 8.0.0
  )

  FetchContent_MakeHermetic(
    CURL
    HERMETIC_BUILD_SYSTEM cmake
    HERMETIC_FIND_PACKAGES "OpenSSL;ZLIB" # this means "CURL can find_package() OpenSSL and ZLIB"
    HERMETIC_TOOLCHAIN_EXTENSION [=[
      set(CURL_USE_OPENSSL ON)
      set(CURL_USE_ZLIB ON)
      # *snip* more options omitted for brevity
    ]=]

  )

  HermeticFetchContent_MakeAvailableAtConfigureTime(CURL)

The declaration mentioned earlier will allow the dependency provider, integrated within 
the hermetic dependency build, to resolve these packages in calls to ``find_package()``.

.. note:: Hermetic FetchContent's CMake Dependency Provider

  This provider is not available at the project level by default. This means that calls 
  to ``find_package()`` within your project will utilize CMake's native implementation.

  Your project should **not** attempt to use `find_package()` to locate hermetic dependencies. 
  Instead, it should rely on the *targets* that are made available through Hermetic FetchContent.


Declaring Library Alias Names
"""""""""""""""""""""""""""""

In complex builds, you might encounter situations where different libraries have diverging 
expectations regarding the names under which packages or libraries are found. 

Aliases can be defined using the ``HERMETIC_CREATE_TARGET_ALIASES`` argument in 
``FetchContent_MakeHermetic()``. This feature allows for the definition of multiple names 
for any of the targets originally exported by the package.

Consider the following example: the `BZip2` content exports a target named ``BZip2::bz2``, 
required by one dependency in our exemplary build. However, another dependency requires the 
same library to be available under the name ``BZip2::BZip2``.

The code fragment within ``HERMETIC_CREATE_TARGET_ALIASES`` is executed for each target. The
``TARGET_NAME`` variable is available within this scope as input, and the `list` ``TARGET_ALIASES`` 
should contain all the alternate names under which the library will be published. The initial 
value of the ``TARGET_ALIASES`` `list` is only the initial ``TARGET_NAME``.

.. code-block:: cmake

  FetchContent_MakeHermetic(
    BZip2
    HERMETIC_BUILD_SYSTEM cmake
    HERMETIC_CREATE_TARGET_ALIASES [=[
      if("${TARGET_NAME}" STREQUAL "BZip2::bz2")
        list(APPEND TARGET_ALIASES "BZip2::bz2" "BZip2::BZip2")
        #
        # the above will allow to do both 
        # target_link_libraries(executable_1 PRIVATE BZip2::bz2)
        # and
        # target_link_libraries(executable_2 PRIVATE BZip2::BZip2)
      endif()
    ]=]
  )

This facility also allows for the complete renaming of the original target, instead of merely 
defining aliases. Similarly, providing an empty alias list can be used to mask certain 
targets if needed.


Using a Source Cache for Pre-Patched Dependencies
"""""""""""""""""""""""""""""""""""""""""""""""""

Hermetic FetchContent includes the ``HERMETIC_PREPATCHED_RESOLVER`` argument, which facilitates 
the injection of "pre-patched" sources into the build. This acts as a sort of cache for source 
files that have already been patched.

This feature allows users to selectively modify the source origin and revision information. It’s 
particularly useful in combination with ``FetchContent``'s `PATCH_COMMAND` capabilities, as it 
provides stable and quick access to sources with all patches pre-applied. Additionally, it 
retains the flexibility to "quickly try out" updates to a dependency.

In the next example, we will refer to the namespaced version of `boost`:

.. code-block:: cmake

  FetchContent_Declare(
    boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG        ad09f667e61e18f5c31590941e748ac38e5a81bf   # boost 1.84
    PATCH_COMMAND "cmake -E patch < ${CMAKE_CURRENT_LIST_DIR}/patchtest/some.patch"
  )

  FetchContent_MakeHermetic(
    boost
    HERMETIC_BUILD_SYSTEM cmake  
    # *snip* declarations omitted for brevity

    HERMETIC_PREPATCHED_RESOLVER [=[
      # note: hypothetical <my-org>/boost-prepatched.git repo
      
      if(${GIT_TAG} STREQUAL "564e2ac16907019696cdaba8a93e3588ec596062")
        set(GIT_REPOSITORY "https://github.com/<my-org>/boost-prepatched.git")
        set(GIT_TAG "9c83f4a27e4227dbe02e4a47ede372ac2a4a043e") # contains patched boost 1.83
        set(RESOLVED_PATCH TRUE)
      # revision in https://github.com/boostorg/boost.git  
      elseif(${GIT_TAG} STREQUAL "ad09f667e61e18f5c31590941e748ac38e5a81bf")
        set(GIT_REPOSITORY "https://github.com/<my-org>/boost-prepatched.git")
        set(GIT_TAG "11cd0a559cfec0636b7a8c53b3f705c8c3676f80") # contains patched boost 1.84
        set(RESOLVED_PATCH TRUE)
      endif()
    ]=]
  )

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
          
          set(GIT_REPOSITORY "https://github.com/<my-org>/boost-prepatched.git")
          set(GIT_TAG "9c83f4a27e4227dbe02e4a47ede372ac2a4a043e")
          set(RESOLVED_PATCH TRUE)
        endif()
      ]=]
    )

  HermeticFetchContent_MakeAvailableAtBuildTime(boost)

In Hermetic FetchContent, if the code within ``HERMETIC_PREPATCHED_RESOLVER`` sets ``RESOLVED_PATCH`` 
to a truthy value, any modifications to the ``GIT_REPOSITORY`` and ``GIT_TAG`` variables are then 
used as the source for the ``populate`` operation.

- If ``RESOLVED_PATCH`` is truthy: The resolver uses the updated ``GIT_REPOSITORY`` and ``GIT_TAG`` values for sourcing.
- If ``RESOLVED_PATCH`` is falsy: The original source specified in ``FetchContent_Declare()`` is used, and the `PATCH_COMMAND` is executed.

It’s important to note that Hermetic FetchContent does not automatically populate the pre-patched source 
repository. The responsibility lies with the user to prepare a correctly patched revision in the repository 
and to update the necessary details via ``HERMETIC_PREPATCHED_RESOLVER``.


Autotools and Related Build Systems
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Hermetic FetchContent offers support for dependencies that do not use a native CMake build system, 
but instead rely on ``GNU Autotools`` or other closely related systems.


.. code-block:: cmake

  FetchContent_Declare(
    Iconv
    URL https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.17.tar.gz
    URL_HASH SHA1=409a6a8c07280e02cc33b65ae871311f4b33077b
    # note: could also be a git repository
  )

  FetchContent_MakeHermetic(
    Iconv
    HERMETIC_BUILD_SYSTEM autotools
    # this emulate target exports 
    HERMETIC_CMAKE_EXPORT_LIBRARY_DECLARATION [=[
      add_library(Iconv::Iconv STATIC IMPORTED)
      set_property(TARGET Iconv::Iconv PROPERTY IMPORTED_LOCATION "@HFC_PREFIX_PLACEHOLDER@/lib/libiconv.a")    
      set_property(TARGET Iconv::Iconv PROPERTY INTERFACE_INCLUDE_DIRECTORIES "@HFC_PREFIX_PLACEHOLDER@/include")
    ]=]
  )
  HermeticFetchContent_MakeAvailableAtConfigureTime(Iconv)
  

This approach is typically suitable for projects following a classic ``configure`` / ``make`` / ``make install`` 
command sequence, accepting commonly used parameters for configuration.

Note the use of the placeholder ``@HFC_PREFIX_PLACEHOLDER@`` in some target properties above. It will be 
replaced by the actual path of the dependency's install prefix by the importing project.

.. note:: Hermetic FetchContent's CMake Dependency Provider

  Special care is needed when an Autotools-based library depends on another Hermetic FetchContent dependency
  (referred to as a *nested dependency*). This applies regardless of whether the *nested dependency* uses CMake
  or Autotools.

  - The *nested dependency* must be fully available at build time because Autotools' configure step typically cannot work with not-yet-existing targets and include paths.
  - To manage this, call ``HermeticFetchContent_MakeAvailableAtConfigureTime(<content-name>)`` for all dependencies that must be resolved *before* the *library* is made available.
  - This dependency applies primarily to the configuration step because the *nested dependency* needs to be fully installed for the *library* to pass its configuration.

  Finally, this does not imply that the *library* must be made available at configure time.
