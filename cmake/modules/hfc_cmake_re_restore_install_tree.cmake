#[=======================================================================[.rst:
hfc_get_cmake_re_restore_install_tree_command
------------------------------------------------------------------------------------------
Generate the ``cmake-re`` command to restore the install tree from cache if possible based on revisions and abi-hash
defining parameters.

  ``origin``
  The origin of the project sources used to generate that install tree
  
  ``revision```
  The revisions of the sources for which the install tree cache is wished. 

  ``project_sources_dir```
  Where the top-level CMakeLists of the project resides

  ``project_build_dir``
  Where the build folder is expected and should be created

  ``project_install_prefix``
  Where the install folder is expected and should be created 

  ``toolchain_file``
  The CMAKE_TOOLCHAIN_FILE to use to determine the projects ABI-HASH

  ``out_var``
  The resutling command line

#]=======================================================================]
function(hfc_get_cmake_re_restore_install_tree_command origin revision project_sources_dir project_build_dir project_install_prefix toolchain_file out_var)
  set(${out_var} "${CMAKE_RE_PATH} -v --origin ${origin} -S ${project_sources_dir} -B ${project_build_dir} -DCMAKE_TOOLCHAIN_FILE=${toolchain_file} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} --install-prefix=${project_install_prefix} --cache restore --revision ${revision} ")

  # --install-prefix ${project_install_prefix}  -S ${project_sources_dir} -B ${project_build_dir}  --cache restore --revision ${revision}
  return(PROPAGATE ${out_var})
endfunction()



#[=======================================================================[.rst:
hfc_cmake_re_restore_install_tree
------------------------------------------------------------------------------------------
Restores the install tree from cache if possible based on revisions and abi-hash
defining parameters.

  ``origin``
  The origin of the project sources used to generate that install tree
  
  ``revision```
  The revisions of the sources for which the install tree cache is wished. 

  ``project_sources_dir```
  Where the top-level CMakeLists of the project resides

  ``project_build_dir``
  Where the build folder is expected and should be created

  ``project_install_prefix``
  Where the install folder is expected and should be created 

  ``toolchain_file``
  The CMAKE_TOOLCHAIN_FILE to use to determine the projects ABI-HASH

  ``out_var``
  Sets as result the variable after provided name out_var to 0 when the restore worked or 1 if no cache entry could be found.

#]=======================================================================]
function(hfc_cmake_re_restore_install_tree origin revision project_sources_dir project_build_dir project_install_prefix toolchain_file out_var)
  set(${out_var} 1)
  if(CMAKE_RE_PATH)

    hfc_get_cmake_re_restore_install_tree_command(${origin} ${revision} ${project_sources_dir} ${project_build_dir} ${project_install_prefix} ${toolchain_file} restore_tree_command)

    execute_process(
      COMMAND  bash -c "${restore_tree_command}"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/
      RESULT_VARIABLE ${out_var}
      COMMAND_ECHO STDOUT
    )
  endif()

  return(PROPAGATE ${out_var})
endfunction()