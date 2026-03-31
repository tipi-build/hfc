include_guard()

#[=======================================================================[.rst:
hfc_get_property_from_directory_chain
--------------------------------------

Collects a directory property from the current directory and all parent directories,
accumulating values in hierarchical order (parent → child).

.. code-block:: cmake

  hfc_get_property_from_directory_chain(<result_var> <property_name>)

``<result_var>``
  Variable to store accumulated property values (set in parent scope)

``<property_name>``
  Directory property to collect (e.g., COMPILE_OPTIONS, LINK_OPTIONS,
  COMPILE_DEFINITIONS, INCLUDE_DIRECTORIES)

Useful for collecting inherited settings from ``add_compile_options()``,
``add_link_options()``, ``add_compile_definitions()``, etc. that may be
set at various directory levels.

#]=======================================================================]
function(hfc_get_property_from_directory_chain RESULT_VAR PROPERTY_NAME)
    set(accumulated_values "")
    set(current_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    
    while(current_dir)
        # Get the property from the current directory
        get_directory_property(dir_value DIRECTORY "${current_dir}" ${PROPERTY_NAME})
        
        if(dir_value)
            list(PREPEND accumulated_values ${dir_value})
        endif()
        
        # Move to parent directory
        get_directory_property(parent_dir DIRECTORY "${current_dir}" PARENT_DIRECTORY)
        set(current_dir "${parent_dir}")
    endwhile()
    
    # Return the accumulated values
    set(${RESULT_VAR} "${accumulated_values}" PARENT_SCOPE)
endfunction()