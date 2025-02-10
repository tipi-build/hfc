set(GOLDILOCK_MINIMUM_VERSION 1.2.0)
hfc_ensure_goldilock_available(
  GOLDILOCK_REVISION 5b34d2fed2d7da1280a2c5a8134f3d6fe10587df
  GOLDILOCK_MINIMUM_VERSION ${GOLDILOCK_MINIMUM_VERSION}

  GOLDILOCK_URL_PREBUILT_Darwin_arm64 https://github.com/tipi-build/goldilock/releases/download/v${GOLDILOCK_MINIMUM_VERSION}/goldilock-macos.zip
  GOLDILOCK_SHA_PREBUILT_Darwin_arm64 642c533ef1257187ab023dc33b95c66ec84b38af

  GOLDILOCK_URL_PREBUILT_Darwin_x86_64 https://github.com/tipi-build/goldilock/releases/download/v${GOLDILOCK_MINIMUM_VERSION}/goldilock-macos-intel.zip
  GOLDILOCK_SHA_PREBUILT_Darwin_x86_64 8ae6b38766ed485e0b8d6d0b7460bb7e8a71a015

  GOLDILOCK_URL_PREBUILT_Linux_x86_64 https://github.com/tipi-build/goldilock/releases/download/v${GOLDILOCK_MINIMUM_VERSION}/goldilock-linux.zip
  GOLDILOCK_SHA_PREBUILT_Linux_x86_64 da1c7b9e9d79fa019ca0e41a002b6fed4fd854ee
)