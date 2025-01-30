# compute the lockfile path
function(__get_lockfile_path dir result_var)
  cmake_path(GET dir PARENT_PATH lock_location)
  cmake_path(GET dir FILENAME dir_name)
  set(${result_var} "${lock_location}/${dir_name}.v2.lock" PARENT_SCOPE) 
endfunction()


function(__get_unlockfile_path lockfile_path result_var)
  set(${result_var} "${lockfile_path}.clear" PARENT_SCOPE) 
endfunction()

# get the executing shell for safe execute_process() in this environment
function(__get_env_shell_command result_var)
  if (WIN32)
    set(${result_var} cmd /C PARENT_SCOPE)
  else()
    set(${result_var} bash -c PARENT_SCOPE)
  endif()  
  
endfunction()



function(hfc_get_goldilock_acquire_command lockfile_path result_var)
  # run goldilock in detached mode and what for either parent process (cmake) to die or unlock file to appear
  # Y/ --no-timeout or should we set a really high timeout (like in 1hour) before the locking fails?
  __get_unlockfile_path("${lockfile_path}" unlock_file_path)
  set(${result_var} "${HERMETIC_FETCHCONTENT_goldilock_BIN} --lockfile ${lockfile_path} --unlockfile ${unlock_file_path} --no-timeout --watch-parent-process ninja,make,cmake --detach" PARENT_SCOPE)
endfunction()

# acquire a directory (goldi)lock 
#
# usage: hfc_goldilock_acquire(<dir> <success_variable)
function(hfc_goldilock_acquire dir success_var)

  __get_lockfile_path("${dir}" lockfile_path)
  __get_env_shell_command(shell)
  hfc_get_goldilock_acquire_command("${lockfile_path}" cmd)

  hfc_log_debug("Trying to aquire lock ${lockfile_path} for ${dir} running ${cmd}")

  execute_process(
    COMMAND ${shell} "${cmd}"
    RESULT_VARIABLE ret_code
    OUTPUT_VARIABLE command_stdoutput
    ERROR_VARIABLE command_stderr
  )

  if(ret_code EQUAL 0)
    set(${success_var} TRUE PARENT_SCOPE)
  else()
    set(${success_var} FALSE PARENT_SCOPE)
    hfc_log(WARNING "Failed to acquire lock: ${lockfile_path}\n${command_stdoutput}")
  endif()

endfunction()

function(hfc_get_goldilock_release_command lockfile_path result_var)
  __get_unlockfile_path("${lockfile_path}" unlock_file_path)
  set(${result_var} "${CMAKE_COMMAND} -E touch ${unlock_file_path}" PARENT_SCOPE)
endfunction()

# release a directory (goldi)lock 
#
# usage: hfc_goldilock_release(<dir> <success_variable)
function(hfc_goldilock_release dir success_var)

  __get_lockfile_path("${dir}" lockfile_path)
  __get_env_shell_command(shell)  
  hfc_get_goldilock_release_command("${lockfile_path}" cmd)
  hfc_log_debug("Trying to release lock ${lockfile_path} for ${dir}")

  execute_process(
    COMMAND ${shell} "${cmd}"
    RESULT_VARIABLE ret_code
    OUTPUT_VARIABLE command_stdoutput
  )

  if(ret_code EQUAL 0)
    set(${success_var} TRUE PARENT_SCOPE)
  else()
    set(${success_var} FALSE PARENT_SCOPE)
    hfc_log(WARNING "Failed to release lock: ${lockfile_path}\n${command_stdoutput}")
  endif()

endfunction()
