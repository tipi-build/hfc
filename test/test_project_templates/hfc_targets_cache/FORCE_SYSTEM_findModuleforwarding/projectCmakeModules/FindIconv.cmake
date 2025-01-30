message(STATUS "Our custom findmodule was executed TESTDATA_INJECTED=${TESTDATA_INJECTED}")

# just let cmake do the actual work...
include(${CMAKE_ROOT}/Modules/FindIconv.cmake)

# mark this so we know it's from us!
set_target_properties(Iconv::Iconv PROPERTIES LABELS "LABEL_BY_HFC")