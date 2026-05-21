.. title:: HermeticFetchContent / Recipes / Configuring Offline or Custom Sources

Configuring Offline or Custom Sources
######################################

Hermetic FetchContent fetches several internal tools and modules from GitHub by default.
In environments where access to public repositories is restricted (e.g. air-gapped networks,
corporate proxies, or CI environments without internet access), you can override those sources
using environment variables.


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
Setting ``HFC_GOLDILOCK_URL_PREBUILT`` replaces that URL regardless of the host platform:

``HFC_GOLDILOCK_URL_PREBUILT``
  URL to the goldilock prebuilt archive (zip) to use instead of the default platform-specific URL.

``HFC_GOLDILOCK_SHA_PREBUILT``
  Expected SHA1 hash of the archive. If not set, hash verification is skipped.

Example — serving the prebuilt binary from an internal HTTP mirror:

.. code-block:: bash

  export HFC_GOLDILOCK_URL_PREBUILT="https://artifacts.internal/goldilock/v1.2.1/goldilock-linux.zip"
  export HFC_GOLDILOCK_SHA_PREBUILT="80729092c01a32a1f987438b2c026e993e0b81d8"
  cmake -S . -B build/


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

.. code-block:: bash

  export HFC_GOLDILOCK_GIT_REPOSITORY="file:///opt/git-mirrors/goldilock.git"
  export HFC_GOLDILOCK_GIT_TAG="5f8b9de72c10a6216c89a8807db8d420cff05512"
  cmake -S . -B build/


cmake-sbom
^^^^^^^^^^^

Hermetic FetchContent includes built-in support for generating SPDX Software Bill of Materials
(SBOM) documents via the `cmake-sbom <https://github.com/DEMCON/cmake-sbom>`_ project.

When enabled (``HFC_ENABLE_CMAKE_SBOM=ON``, which is the default), Hermetic FetchContent will
automatically fetch and bootstrap the ``cmake-sbom`` module if it is not already available on the
``CMAKE_MODULE_PATH``.

The following environment variables override the source:

``HFC_CMAKE_SBOM_GIT_REPOSITORY``
  Git repository URL used to fetch ``cmake-sbom``.

  Default: ``https://github.com/DEMCON/cmake-sbom.git``

``HFC_CMAKE_SBOM_GIT_TAG``
  Git tag or commit hash to checkout.

  Default: ``97b1a0715af7726cae93d96d322c48584945f96b`` (v1.1.2)

Example — pointing to an internal mirror:

.. code-block:: bash

  export HFC_CMAKE_SBOM_GIT_REPOSITORY="https://git.internal.example.com/mirrors/cmake-sbom.git"
  export HFC_CMAKE_SBOM_GIT_TAG="v1.1.2"
  cmake -S . -B build/

To disable SBOM generation entirely:

.. code-block:: bash

  cmake -S . -B build/ -DHFC_ENABLE_CMAKE_SBOM=OFF
