# Toolchain for offline source override tests
# Intentionally does NOT set HERMETIC_FETCHCONTENT_TOOLS_DIR from HFC_TEST_SHARED_TOOLS_DIR
# so that goldilock provisioning runs fresh every time (exercising env var overrides)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
