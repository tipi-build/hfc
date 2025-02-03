if(CMAKE_SCRIPT_MODE_FILE) 
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
    cmake_policy(SET CMP0054 NEW)
endif()


include(hfc_log)
include(hfc_required_args)

find_program(git_executable 
    NAMES git 
    REQUIRED
)

function(git_exec)

    set(options_params)

    set(oneValueArgs_required
        COMMAND
        WORKING_DIRECTORY
    )

    set(one_value_params
        COMMAND_ERROR_IS_FATAL
        OUT_RESULT
        OUT_RETURN_CODE
        ${oneValueArgs_required}
    )

    set(multi_value_params )

    cmake_parse_arguments(
        FN_ARG
        "${options_params}"
        "${one_value_params}"
        "${multi_value_params}"
        ${ARGN}
    )
    hfc_required_args(FN_ARG ${oneValueArgs_required})

    if(FN_ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
            " ${FN_ARG_UNPARSED_ARGUMENTS}"
        )
    endif()

    if(FN_ARG_COMMAND_ERROR_IS_FATAL)
        set(execute_process_arg_COMMAND_ERROR_IS_FATAL COMMAND_ERROR_IS_FATAL ${FN_ARG_COMMAND_ERROR_IS_FATAL})
    endif()

    if(FN_ARG_OUT_RETURN_CODE)
        set(execute_process_arg_RESULT_VARIABLE RESULT_VARIABLE cmd_retcode)
    endif()

    if (WIN32)
        set(shell cmd /C)
	else()
        set(shell bash -c)
	endif()

    hfc_log(TRACE "Running command: '${FN_ARG_COMMAND}' in working directory: ${FN_ARG_WORKING_DIRECTORY}")

    execute_process(COMMAND ${shell} "${FN_ARG_COMMAND}"
        WORKING_DIRECTORY "${FN_ARG_WORKING_DIRECTORY}"
        OUTPUT_VARIABLE cmd_output OUTPUT_STRIP_TRAILING_WHITESPACE
        ${execute_process_arg_COMMAND_ERROR_IS_FATAL}
        ${execute_process_arg_RESULT_VARIABLE}
    )

    hfc_log_debug("Output of ${FN_ARG_COMMAND}: ${cmd_output}")

    if(FN_ARG_OUT_RESULT)
        set(${FN_ARG_OUT_RESULT} "${cmd_output}" PARENT_SCOPE)
    endif()

    if(FN_ARG_OUT_RETURN_CODE)
        set(${FN_ARG_OUT_RETURN_CODE} "${cmd_retcode}" PARENT_SCOPE)
    endif()

endfunction()

function(repo_has_revision) 
    set(options_params)

    set(oneValueArgs_required
        REPOSITORY_DIR
        GIT_REVISION
        OUT_RESULT
    )

    set(one_value_params   
        ${oneValueArgs_required}
    )

    set(multi_value_params )

    cmake_parse_arguments(
        FN_ARG
        "${options_params}"
        "${one_value_params}"
        "${multi_value_params}"
        ${ARGN}
    )
    hfc_required_args(FN_ARG ${oneValueArgs_required})

    if(FN_ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
            " ${FN_ARG_UNPARSED_ARGUMENTS}"
        )
    endif()

    hfc_log_debug(STATUS "Checking if repository '${FN_ARG_REPOSITORY_DIR}' has commit ${FN_ARG_GIT_REVISION}")
    git_exec(COMMAND "${git_executable} cat-file -e ${FN_ARG_GIT_REVISION}" WORKING_DIRECTORY "${FN_ARG_REPOSITORY_DIR}" OUT_RETURN_CODE return_code)

    if(return_code EQUAL 0)
        set(${FN_ARG_OUT_RESULT} TRUE PARENT_SCOPE)
    else()
        set(${FN_ARG_OUT_RESULT} FALSE PARENT_SCOPE)
    endif()

endfunction()

function(repo_has_revision) 
    set(options_params)

    set(oneValueArgs_required
        REPOSITORY_DIR
        GIT_REVISION
        OUT_RESULT
    )

    set(one_value_params   
        ${oneValueArgs_required}
    )

    set(multi_value_params )

    cmake_parse_arguments(
        FN_ARG
        "${options_params}"
        "${one_value_params}"
        "${multi_value_params}"
        ${ARGN}
    )
    hfc_required_args(FN_ARG ${oneValueArgs_required})

    if(FN_ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
            " ${FN_ARG_UNPARSED_ARGUMENTS}"
        )
    endif()

    hfc_log_debug("Checking if repository '${FN_ARG_REPOSITORY_DIR}' has commit ${FN_ARG_GIT_REVISION}")
    git_exec(COMMAND "${git_executable} cat-file -e ${FN_ARG_GIT_REVISION}" WORKING_DIRECTORY "${FN_ARG_REPOSITORY_DIR}" OUT_RETURN_CODE return_code)

    if(return_code EQUAL 0)
        set(${FN_ARG_OUT_RESULT} TRUE PARENT_SCOPE)
    else()
        set(${FN_ARG_OUT_RESULT} FALSE PARENT_SCOPE)
    endif()

endfunction()

function(repo_is_clean) 
    set(options_params 
        CHECK_IGNORED
    )

    set(oneValueArgs_required
        REPOSITORY_DIR
        OUT_RESULT
    )

    set(one_value_params        
        ${oneValueArgs_required}
    )

    set(multi_value_params )

    cmake_parse_arguments(
        FN_ARG
        "${options_params}"
        "${one_value_params}"
        "${multi_value_params}"
        ${ARGN}
    )
    hfc_required_args(FN_ARG ${oneValueArgs_required})

    if(FN_ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
            " ${FN_ARG_UNPARSED_ARGUMENTS}"
        )
    endif()

    if(FN_ARG_CHECK_IGNORED) 
        set(process_arg_CHECK_IGNORED "--ignored")
    endif()

    hfc_log_debug("Checking if repository '${FN_ARG_REPOSITORY_DIR}' has any changes (CHECK_IGNORED = ${FN_ARG_CHECK_IGNORED})")
    git_exec(COMMAND "${git_executable} status ${process_arg_CHECK_IGNORED} --porcelain" 
        WORKING_DIRECTORY "${FN_ARG_REPOSITORY_DIR}"
        OUT_RESULT cmd_output
        OUT_RETURN_CODE return_code
    )

    if(cmd_output STREQUAL "")
        set(${FN_ARG_OUT_RESULT} TRUE PARENT_SCOPE)
    else()
        set(${FN_ARG_OUT_RESULT} FALSE PARENT_SCOPE)
    endif()

endfunction()

function(repo_get_head_id) 
    set(options_params)

    set(oneValueArgs_required
        REPOSITORY_DIR
        OUT_COMMIT_ID
    )

    set(one_value_params   
        ${oneValueArgs_required}
    )

    set(multi_value_params )

    cmake_parse_arguments(
        FN_ARG
        "${options_params}"
        "${one_value_params}"
        "${multi_value_params}"
        ${ARGN}
    )
    hfc_required_args(FN_ARG ${oneValueArgs_required})

    if(FN_ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
            " ${FN_ARG_UNPARSED_ARGUMENTS}"
        )
    endif()

    git_exec(
        COMMAND "${git_executable} rev-parse HEAD" 
        WORKING_DIRECTORY "${FN_ARG_REPOSITORY_DIR}" 
        COMMAND_ERROR_IS_FATAL ANY
        OUT_RESULT cmd_output
    )

    hfc_log_debug("Repository '${FN_ARG_REPOSITORY_DIR}' is at ${cmd_output}")

    set(${FN_ARG_OUT_COMMIT_ID} "${cmd_output}" PARENT_SCOPE)
endfunction()

function(checkout_revision_force_clean)
    set(options_params)

    set(oneValueArgs_required
        REPOSITORY_DIR
        GIT_REVISION
    )

    set(one_value_params   
        OUT_SUCCESS
        ${oneValueArgs_required}

    )

    set(multi_value_params )

    cmake_parse_arguments(
        FN_ARG
        "${options_params}"
        "${one_value_params}"
        "${multi_value_params}"
        ${ARGN}
    )
    hfc_required_args(FN_ARG ${oneValueArgs_required})

    if(FN_ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
            " ${FN_ARG_UNPARSED_ARGUMENTS}"
        )
    endif()

    repo_get_head_id(REPOSITORY_DIR "${FN_ARG_REPOSITORY_DIR}"
        OUT_COMMIT_ID initial_commit_id
    )

    repo_is_clean(REPOSITORY_DIR "${FN_ARG_REPOSITORY_DIR}"
        CHECK_IGNORED
        OUT_RESULT initial_repo_is_clean
    )

    if(initial_commit_id STREQUAL FN_ARG_GIT_REVISION AND initial_repo_is_clean)
        hfc_log_debug("Repository ${FN_ARG_REPOSITORY_DIR} already at ${FN_ARG_GIT_REVISION} and clean - skipping")
        
        if(FN_ARG_OUT_SUCCESS)
            set(${FN_ARG_OUT_SUCCESS} TRUE PARENT_SCOPE)
        endif()
        return()
    endif()

    hfc_log_debug("Repository ${FN_ARG_REPOSITORY_DIR} currently at ${initial_commit_id} and is_clean=${initial_repo_is_clean}")

    if(NOT initial_repo_is_clean)
        hfc_log_debug("Cleaning any changes")
        git_exec(COMMAND "${git_executable} clean -xfdf" WORKING_DIRECTORY "${FN_ARG_REPOSITORY_DIR}" OUT_RETURN_CODE ret_stage0_clean_repo)
        git_exec(COMMAND "${git_executable} submodule foreach --recursive ${git_executable} clean -xfdf" WORKING_DIRECTORY "${FN_ARG_REPOSITORY_DIR}" OUT_RETURN_CODE ret_stage0_clean_submodules)

        if(FN_ARG_OUT_SUCCESS AND (ret_stage0_clean_repo GREATER 0 OR ret_stage0_clean_submodules GREATER 0))
            set(${FN_ARG_OUT_SUCCESS} FALSE PARENT_SCOPE)
            return()
        endif()        
    endif()

    hfc_log_debug("Checking is destination revision ${FN_ARG_GIT_REVISION} is available locally")

    # check if we have the revision locally
    repo_has_revision(REPOSITORY_DIR "${FN_ARG_REPOSITORY_DIR}"
        GIT_REVISION "${FN_ARG_GIT_REVISION}"
        OUT_RESULT revision_exists
    )

    if(NOT revision_exists)
        hfc_log_debug("Commit not found: fetching updated for repository ${FN_ARG_REPOSITORY_DIR}")
        git_exec(COMMAND "${git_executable} fetch origin" WORKING_DIRECTORY "${FN_ARG_REPOSITORY_DIR}" COMMAND_ERROR_IS_FATAL ANY)
    endif()

    # make sure submodules are in sync
    git_exec(COMMAND "${git_executable} submodule sync --recursive" WORKING_DIRECTORY "${FN_ARG_REPOSITORY_DIR}" COMMAND_ERROR_IS_FATAL ANY)

    # checkout the destination revision
    hfc_log(STATUS "Checking out ${FN_ARG_REPOSITORY_DIR}")
    git_exec(COMMAND "${git_executable} checkout -f ${FN_ARG_GIT_REVISION}" WORKING_DIRECTORY "${FN_ARG_REPOSITORY_DIR}" OUT_RETURN_CODE ret_stage1_checkout)
    
    if(FN_ARG_OUT_SUCCESS AND ret_stage1_checkout GREATER 0) 
        set(${FN_ARG_OUT_SUCCESS} FALSE PARENT_SCOPE)
        return()
    endif()

    git_exec(COMMAND "${git_executable} clean -xfdf" WORKING_DIRECTORY "${FN_ARG_REPOSITORY_DIR}" COMMAND_ERROR_IS_FATAL ANY)
    git_exec(COMMAND "${git_executable} submodule foreach --recursive ${git_executable} clean -xfdf" WORKING_DIRECTORY "${FN_ARG_REPOSITORY_DIR}" COMMAND_ERROR_IS_FATAL ANY)
    
    # make sure the submodules are up-to-date
    git_exec(COMMAND "${git_executable} submodule update --init --recursive" WORKING_DIRECTORY "${FN_ARG_REPOSITORY_DIR}" COMMAND_ERROR_IS_FATAL ANY)

    # \ö/
    set(${FN_ARG_OUT_SUCCESS} TRUE PARENT_SCOPE)
endfunction()


function(is_git_repository)
    set(options_params)
    set(oneValueArgs_required REPOSITORY_DIR OUT_RESULT)
    set(multi_value_params)

    cmake_parse_arguments(
        FN_ARG
        "${options_params}"
        "${oneValueArgs_required}"
        "${multi_value_params}"
        ${ARGN}
    )
    hfc_required_args(FN_ARG ${oneValueArgs_required})

    if(FN_ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Invalid arguments passed to ${CMAKE_CURRENT_FUNCTION}"
            " ${FN_ARG_UNPARSED_ARGUMENTS}"
        )
    endif()

    if(IS_DIRECTORY "${FN_ARG_REPOSITORY_DIR}/.git") 

        hfc_log_debug("Checking if folder '${FN_ARG_REPOSITORY_DIR}' contains a git repository")
        git_exec(COMMAND "${git_executable} rev-parse --is-inside-work-tree" 
            WORKING_DIRECTORY "${FN_ARG_REPOSITORY_DIR}"
            OUT_RETURN_CODE return_code
        )

    endif()

    if(return_code EQUAL 0)
        set(${FN_ARG_OUT_RESULT} TRUE PARENT_SCOPE)
    else()
        set(${FN_ARG_OUT_RESULT} FALSE PARENT_SCOPE)
    endif()

endfunction()