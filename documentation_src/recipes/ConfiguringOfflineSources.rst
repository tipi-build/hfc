.. title:: HermeticFetchContent / Recipes / Configuring Offline or Custom Sources

Configuring Offline or Custom Sources
######################################

Hermetic FetchContent fetches several internal tools and modules from GitHub by default.
In environments where access to public repositories is restricted (e.g. air-gapped networks,
corporate proxies, or CI environments without internet access), you can override those sources
using CMake variables.

These variables should be set **before** including HermeticFetchContent — typically in your
project's toolchain file or in a ``build_thirdparty.cmake`` included early in your build:

.. code-block:: cmake

  # In your toolchain file or build_thirdparty.cmake
  set(HFC_GOLDILOCK_URL_PREBUILT_Linux_x86_64 "https://artifacts.internal/goldilock/v1.2.1/goldilock-linux.zip")
  set(HFC_GOLDILOCK_SHA_PREBUILT_Linux_x86_64 "80729092c01a32a1f987438b2c026e993e0b81d8")

  # ... then include HFC
  include(HermeticFetchContent)


Goldilock
^^^^^^^^^

Goldilock is the directory-locking tool used internally by Hermetic FetchContent.
It is provisioned automatically through the following strategy:

1. Check if a compatible ``goldilock`` is already on ``PATH``
2. Download a prebuilt binary for the current platform
3. Build from source as a last resort

Overriding the Prebuilt Download URL
=====================================

By default, the prebuilt binary is downloaded from a platform-specific GitHub release URL.
The override variables include the platform suffix ``_${CMAKE_HOST_SYSTEM_NAME}_${CMAKE_HOST_SYSTEM_PROCESSOR}``
so that each target host can point to its own archive:

``HFC_GOLDILOCK_URL_PREBUILT_<system>_<processor>``
  URL to the goldilock prebuilt archive (zip) to use instead of the default platform-specific URL.

``HFC_GOLDILOCK_SHA_PREBUILT_<system>_<processor>``
  Expected SHA1 hash of the archive. **Required** — the prebuilt download is only accepted
  if the SHA is non-empty and matches. If not set, the built-in default SHA is used.
  If the SHA does not match (or is empty), the prebuilt is rejected and HFC falls through
  to building from source.

Where ``<system>`` is ``CMAKE_HOST_SYSTEM_NAME`` (e.g. ``Linux``, ``Darwin``) and ``<processor>``
is ``CMAKE_HOST_SYSTEM_PROCESSOR`` (e.g. ``x86_64``, ``arm64``).

Example — serving the prebuilt binary from an internal HTTP mirror for Linux x86_64:

.. code-block:: cmake

  set(HFC_GOLDILOCK_URL_PREBUILT_Linux_x86_64 "https://artifacts.internal/goldilock/v1.2.1/goldilock-linux.zip")
  set(HFC_GOLDILOCK_SHA_PREBUILT_Linux_x86_64 "80729092c01a32a1f987438b2c026e993e0b81d8")
  set(HFC_GOLDILOCK_URL_PREBUILT_Darwin_arm64 "https://artifacts.internal/goldilock/v1.2.1/goldilock-darwin-arm64.zip")
  set(HFC_GOLDILOCK_SHA_PREBUILT_Darwin_arm64 "a1b2c3d4e5f6...")


Overriding the Source Repository
================================

If the prebuilt binary download fails or is unavailable for the host platform, goldilock is
built from source. The source repository can be overridden with:

``HFC_GOLDILOCK_GIT_REPOSITORY``
  Git repository URL for goldilock sources.

  Default: ``https://github.com/tipi-build/goldilock.git``

``HFC_GOLDILOCK_GIT_TAG``
  Git tag or commit hash to checkout.

  Default: the pinned revision matching the current HFC release.

Example — using a local bare clone:

.. code-block:: cmake

  set(HFC_GOLDILOCK_GIT_REPOSITORY "file:///opt/git-mirrors/goldilock.git")
  set(HFC_GOLDILOCK_GIT_TAG "5f8b9de72c10a6216c89a8807db8d420cff05512")


cmake-sbom
^^^^^^^^^^^

Hermetic FetchContent includes built-in support for generating SPDX Software Bill of Materials
(SBOM) documents via the `cmake-sbom <https://github.com/DEMCON/cmake-sbom>`_ project.

When enabled (``HFC_ENABLE_CMAKE_SBOM=ON``, which is the default), Hermetic FetchContent will
automatically fetch and bootstrap the ``cmake-sbom`` module if it is not already available on the
``CMAKE_MODULE_PATH``.

The following CMake variables override the source:

``HFC_CMAKE_SBOM_GIT_REPOSITORY``
  Git repository URL used to fetch ``cmake-sbom``.

  Default: ``https://github.com/DEMCON/cmake-sbom.git``

``HFC_CMAKE_SBOM_GIT_TAG``
  Git tag or commit hash to checkout.

  Default: ``97b1a0715af7726cae93d96d322c48584945f96b`` (v1.1.2)

Example — pointing to an internal mirror:

.. code-block:: cmake

  set(HFC_CMAKE_SBOM_GIT_REPOSITORY "https://git.internal.example.com/mirrors/cmake-sbom.git")
  set(HFC_CMAKE_SBOM_GIT_TAG "v1.1.2")

To disable SBOM generation entirely:

.. code-block:: cmake

  # Pass on the command line or set in your CMakeLists.txt
  set(HFC_ENABLE_CMAKE_SBOM OFF)
