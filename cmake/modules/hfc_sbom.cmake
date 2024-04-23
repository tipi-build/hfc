# bootstrap sbom
include_guard()

include(hfc_populate_project)
include(hfc_log)

include(sbom OPTIONAL RESULT_VARIABLE sbom_found)

if(sbom_found STREQUAL "NOTFOUND")

    set(cmake_sbom_GIT_REPOSITORY "https://github.com/DEMCON/cmake-sbom.git")
    set(cmake_sbom_GIT_TAG "97b1a0715af7726cae93d96d322c48584945f96b") # v1.1.2

    hfc_log(STATUS "Enabling HFC cmake-sbom support (${cmake_sbom_GIT_REPOSITORY} @rev: ${cmake_sbom_GIT_TAG})")

    string(SUBSTRING "${cmake_sbom_GIT_TAG}" 0 8 source_hash_short)
    set(cmake_sbom_clone_dir_name "hfc_cmake_sbom-${source_hash_short}-src")

    if(HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR)
        set(cmake_sbom_clone_dir_root  "${HERMETIC_FETCHCONTENT_SOURCE_CACHE_DIR}")
    else()
        set(cmake_sbom_clone_dir_root  "${CMAKE_BINARY_DIR}/_deps")
    endif()

    if(NOT EXISTS "${cmake_sbom_clone_dir_root}")
        file(MAKE_DIRECTORY "${cmake_sbom_clone_dir_root}")
    endif()

    # cmake-SBOM bootstrapping
    hfc_populate_project_declare(hfc_cmake_sbom)
    hfc_populate_project_invoke_internal(
        hfc_cmake_sbom
        GIT_REPOSITORY "${cmake_sbom_GIT_REPOSITORY}"
        GIT_TAG "${cmake_sbom_GIT_TAG}"
        SOURCE_DIR "${cmake_sbom_clone_dir_root}/${cmake_sbom_clone_dir_name}"     
        BINARY_DIR "${cmake_sbom_clone_dir_root}/${cmake_sbom_clone_dir_name}-bin"
    )

    FetchContent_GetProperties(hfc_cmake_sbom)
    hfc_log(STATUS " - success - available in: '${hfc_cmake_sbom_SOURCE_DIR}'")
    list(APPEND CMAKE_MODULE_PATH "${hfc_cmake_sbom_SOURCE_DIR}/cmake")
else()
    hfc_log(STATUS "Enabling HFC cmake-sbom support (using cmake-sbom already on CMAKE_MODULE_PATH at ${sbom_found})")
endif()


include(sbom)
include(version)
include(hfc_saved_details)
include(hfc_log)

function(hfc_generate_sbom)

    set(fn_options_params 
        NO_VERIFY
        VERIFY
    )

    set(fn_one_value_params
        OUTPUT
        LICENSE
        SUPPLIER
        SUPPLIER_URL
    )

    set(fn_multi_value_params "")

    cmake_parse_arguments(
        FN_ARG
        "${fn_options_params}"
        "${fn_one_value_params}"
        "${fn_multi_value_params}"
        ${ARGN}
    )

    hfc_log_debug("Generating SBOM (OUTPUT '${FN_ARG_OUTPUT}' LICENSE '${FN_ARG_LICENSE}' SUPPLIER '${FN_ARG_SUPPLIER}' SUPPLIER_URL '${FN_ARG_SUPPLIER_URL}')")
    sbom_generate(
        OUTPUT "${FN_ARG_OUTPUT}"
        LICENSE "${FN_ARG_LICENSE}"
        SUPPLIER "${FN_ARG_SUPPLIER}"
        SUPPLIER_URL "${FN_ARG_SUPPLIER_URL}"
    )

    set(sbom_generated_fragments "")
    list(APPEND sbom_generated_fragments "${CMAKE_CURRENT_BINARY_DIR}/SPDXRef-DOCUMENT.spdx.in") # sadly here, the trick using ${SBOM_LAST_SPDXID} doesn't work vs. the use case below where sbom_add() sets this to a useful value
    
    foreach(content_name IN LISTS HERMETIC_FETCHCONTENT_TARGETS_CACHE_CONSUMED_CONTENTS)
        hfc_details_declared(${content_name} ${content_name}_content_defined)
        if (${content_name}_content_defined) 
          hfc_saved_details_get(${content_name} __fetchcontent_arguments)

          set(options_params "")  
          set(one_value_params
              URL
              URL_HASH
              GIT_REPOSITORY
              GIT_TAG
              HERMETIC_FIND_PACKAGES
              SBOM_LICENSE
              SBOM_SUPPLIER
          )

          set(multi_value_params "")

          cmake_parse_arguments(
              HFC_ARG
              "${options_params}"
              "${one_value_params}"
              "${multi_value_params}"
              ${__fetchcontent_arguments}
          )

          set(download_location "")
          set(sbom_version "${HFC_ARG_SBOM_VERSION}")
          set(sbom_license "${HFC_ARG_SBOM_LICENSE}")
          set(sbom_supplier "${HFC_ARG_SBOM_SUPPLIER}")
          
          set(sbom_license "${HFC_ARG_SBOM_LICENSE}")
          if(NOT HFC_ARG_SBOM_LICENSE)
              hfc_log(WARNING "HFC Content ${content_name} has no definition of SBOM_LICENSE")
              set(sbom_license "Unknown")
          endif()

          set(sbom_supplier "${HFC_ARG_SBOM_SUPPLIER}")
          if(NOT HFC_ARG_SBOM_SUPPLIER)
              hfc_log(WARNING "HFC Content ${content_name} has no definition of SBOM_SUPPLIER")
              set(sbom_supplier "Unknown")
          endif()

          if(HFC_ARG_GIT_REPOSITORY AND HFC_ARG_URL) 
              hfc_log(WARNING "HFC Content ${content_name} has definitions for both GIT_REPOSITORY and URL")
          endif()

          if(HFC_ARG_GIT_REPOSITORY)
              set(download_location "${HFC_ARG_GIT_REPOSITORY}")
              set(sbom_version "${HFC_ARG_GIT_TAG}")
          else()
              set(download_location "${HFC_ARG_URL}")
              set(sbom_version "${HFC_ARG_URL_HASH}")
          endif()
          
          hfc_log_debug("Adding SBOM entry for ${content_name} (PACKAGE ${content_name} DOWNLOAD_LOCATION ${download_location} LICENSE ${sbom_license} SUPPLIER ${sbom_supplier} VERSION ${sbom_version})")

          sbom_add(
              PACKAGE "${content_name}"
              DOWNLOAD_LOCATION "${download_location}"
              #[EXTREF <ref>...]
              LICENSE "${sbom_license}"
              #[RELATIONSHIP <string>]
              #[SPDXID <id>]
              SUPPLIER "${sbom_supplier}"
              VERSION "${sbom_version}"
          )
          
          set(spdx_ID_${content_name} "${SBOM_LAST_SPDXID}")

          hfc_log_debug(" - gather generated SPDX fragment generator ${SBOM_LAST_SPDXID}")
          list(APPEND sbom_generated_fragments "${CMAKE_CURRENT_BINARY_DIR}/${SBOM_LAST_SPDXID}.cmake")
        endif()
        
    endforeach()

    #
    # DIY finalize so we control the runtime (no need to install()... )
    #
    #hfc_log_debug("Finalizing SBOM")

    set(output_destination_path "${CMAKE_BINARY_DIR}/${FN_ARG_OUTPUT}")
    set(generate_sbom_script "${CMAKE_CURRENT_BINARY_DIR}/hfc_sbom/generate_sbom.cmake")

    file(
		GENERATE
		OUTPUT "${generate_sbom_script}"
		CONTENT "
# This file has been generated by HermeticFetchContent's SBOM module
# it is designed to be run in CMake Script mode to generate the SPDX SBOM at the following location:
# > ${output_destination_path}
#
# To execute it build the custom target hfc_generate_sbom of your project:
# > cmake --build . --target hfc_generate_sbom 

# note: create a temp copy of the generate input file so we do not end up coliding
# if this custom target is run concurrently
string(RANDOM LENGTH 8 tmp_suffix)
set(spdx_in_file_dest \"${CMAKE_CURRENT_BINARY_DIR}/sbom/sbom.spdx.in\") # needs to match the impl. detail from sbom-cmake project...
set(spdx_in_file_tmp \"${CMAKE_CURRENT_BINARY_DIR}/sbom/sbom.spdx.in.\${tmp_suffix}\")

cmake_policy(SET CMP0007 NEW)
function(file)
    list(GET ARGN 0 file_OP)
    list(GET ARGN 1 param_2)
    list(SUBLIST ARGN 2 -1 remaining_args)

    if((\"\${file_OP}\" STREQUAL \"APPEND\") AND (\"\${param_2}\" STREQUAL \"\${spdx_in_file_dest}\"))
        _file(APPEND \"\${spdx_in_file_tmp}\" \${remaining_args})
    elseif(\"\${file_OP}\" STREQUAL \"READ\") 
        _file(\${ARGN})
        list(GET ARGN -1 out_variable_name)
        set(\${out_variable_name} \${\${out_variable_name}} PARENT_SCOPE)
    else()
        _file(\${ARGN})
    endif()    
endfunction()

file(WRITE \"\${spdx_in_file_tmp}\" \"\")
set(sbom_generated_fragments \"${sbom_generated_fragments}\")
foreach(fragment IN LISTS sbom_generated_fragments)

    if(fragment MATCHES \"\\\\.spdx\\\\.in$\")
        message(STATUS \" - copying contents from \${fragment}\")
        file(READ \"\${fragment}\" contents)     
        file(APPEND \"\${spdx_in_file_tmp}\" \"\${contents}\")
    else()
        message(STATUS \" - including \${fragment}\")
        include(\${fragment})        
    endif()

endforeach()
message(STATUS \" - generating SBOM\")

set(SBOM_VERIFICATION_CODE \"\")
configure_file(\"\${spdx_in_file_tmp}\" \"${output_destination_path}.\${tmp_suffix}\")
file(COPY_FILE \"${output_destination_path}.\${tmp_suffix}\" \"${output_destination_path}\" ONLY_IF_DIFFERENT)

# clean up temp files
message(STATUS \"Cleaning up temp files\")
file(REMOVE 
    \"\${spdx_in_file_tmp}\" 
    \"${output_destination_path}.\${tmp_suffix}\"
)

message(STATUS \"===SBOM===\")
message(STATUS \"${output_destination_path}\")
	")

    add_custom_target(hfc_generate_sbom
      COMMENT "Generate the project's SBOM"
      COMMAND ${CMAKE_COMMAND} "-P ${generate_sbom_script}"
    )

endfunction()