set(GOLDILOCK_MINIMUM_VERSION 1.2.1)
hfc_ensure_goldilock_available(
  GOLDILOCK_REVISION 5f8b9de72c10a6216c89a8807db8d420cff05512
  GOLDILOCK_MINIMUM_VERSION ${GOLDILOCK_MINIMUM_VERSION}

  GOLDILOCK_URL_PREBUILT_Darwin_arm64 https://github.com/tipi-build/goldilock/releases/download/v${GOLDILOCK_MINIMUM_VERSION}/goldilock-macos.zip
  GOLDILOCK_SHA_PREBUILT_Darwin_arm64 313886e48529cd6b26a4738ab7b93265a11aac51

  GOLDILOCK_URL_PREBUILT_Darwin_x86_64 https://github.com/tipi-build/goldilock/releases/download/v${GOLDILOCK_MINIMUM_VERSION}/goldilock-macos-intel.zip
  GOLDILOCK_SHA_PREBUILT_Darwin_x86_64 7f1db0d71b143ed86cba4caa2f0b44f6010c6266

  GOLDILOCK_URL_PREBUILT_Linux_x86_64 https://github.com/tipi-build/goldilock/releases/download/v${GOLDILOCK_MINIMUM_VERSION}/goldilock-linux.zip
  GOLDILOCK_SHA_PREBUILT_Linux_x86_64 80729092c01a32a1f987438b2c026e993e0b81d8
)