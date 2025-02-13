#
# Prints either CPU core count or the value of $ENV{CMAKE_BUILD_PARALLEL_LEVEL}


# fallback, there's certainly at least one CPU core to run our code on...
set(result "1")

if(DEFINED ENV{CMAKE_BUILD_PARALLEL_LEVEL})
    set(result $ENV{CMAKE_BUILD_PARALLEL_LEVEL})
else()
    include(ProcessorCount)
    ProcessorCount(NUM_CPUS)

    if(NOT NUM_CPUS EQUAL 0)
        set(result ${NUM_CPUS})
    endif()
endif()

# print the result without a newline so it can be used in shell eval without trimming first
execute_process(COMMAND ${CMAKE_COMMAND} -E echo_append "${result}")