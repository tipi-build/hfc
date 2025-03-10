# HermeticFetchContent / targets cache common
#

include(hfc_log)
include(hfc_targets_cache_common)


# this is the list of all properties that CMake defines for targets
# except the following entries:
#
# - BINARY_DIR  / ignoring this because it's set to the current BINARY_DIR value
# - SOURCE_DIR  / ignoring this because it's set to the current BINARY_DIR value
# - CXX_MODULE_SETS / read-only
# - HEADER_SETS / read-only
# - INTERFACE_CXX_MODULE_SETS / read-only
function(__compute_HERMETIC_FETCHCONTENT_TARGETSCACHE_SUPPORTED_PROPERTIES)
  set(known_cmake_variables "ADDITIONAL_CLEAN_FILES;AIX_EXPORT_ALL_SYMBOLS;ALIAS_GLOBAL;ALIASED_TARGET;ANDROID_ANT_ADDITIONAL_OPTIONS;ANDROID_API;ANDROID_API_MIN;ANDROID_ARCH;ANDROID_ASSETS_DIRECTORIES;ANDROID_GUI;ANDROID_JAR_DEPENDENCIES;ANDROID_JAR_DIRECTORIES;ANDROID_JAVA_SOURCE_DIR;ANDROID_NATIVE_LIB_DEPENDENCIES;ANDROID_NATIVE_LIB_DIRECTORIES;ANDROID_PROCESS_MAX;ANDROID_PROGUARD;ANDROID_PROGUARD_CONFIG_PATH;ANDROID_SECURE_PROPS_PATH;ANDROID_SKIP_ANT_STEP;ANDROID_STL_TYPE;ARCHIVE_OUTPUT_DIRECTORY;ARCHIVE_OUTPUT_DIRECTORY_<CONFIG>;ARCHIVE_OUTPUT_NAME;ARCHIVE_OUTPUT_NAME_<CONFIG>;AUTOGEN_BETTER_GRAPH_MULTI_CONFIG;AUTOGEN_BUILD_DIR;AUTOGEN_COMMAND_LINE_LENGTH_MAX;AUTOGEN_ORIGIN_DEPENDS;AUTOGEN_PARALLEL;AUTOGEN_TARGET_DEPENDS;AUTOGEN_USE_SYSTEM_INCLUDE;AUTOMOC;AUTOMOC_COMPILER_PREDEFINES;AUTOMOC_DEPEND_FILTERS;AUTOMOC_EXECUTABLE;AUTOMOC_MACRO_NAMES;AUTOMOC_MOC_OPTIONS;AUTOMOC_PATH_PREFIX;AUTORCC;AUTORCC_EXECUTABLE;AUTORCC_OPTIONS;AUTOUIC;AUTOUIC_EXECUTABLE;AUTOUIC_OPTIONS;AUTOUIC_SEARCH_PATHS;BUILD_RPATH;BUILD_RPATH_USE_ORIGIN;BUILD_WITH_INSTALL_NAME_DIR;BUILD_WITH_INSTALL_RPATH;BUNDLE;BUNDLE_EXTENSION;C_EXTENSIONS;C_STANDARD;C_STANDARD_REQUIRED;COMMON_LANGUAGE_RUNTIME;COMPATIBLE_INTERFACE_BOOL;COMPATIBLE_INTERFACE_NUMBER_MAX;COMPATIBLE_INTERFACE_NUMBER_MIN;COMPATIBLE_INTERFACE_STRING;COMPILE_DEFINITIONS;COMPILE_FEATURES;COMPILE_FLAGS;COMPILE_OPTIONS;COMPILE_PDB_NAME;COMPILE_PDB_NAME_<CONFIG>;COMPILE_PDB_OUTPUT_DIRECTORY;COMPILE_PDB_OUTPUT_DIRECTORY_<CONFIG>;COMPILE_WARNING_AS_ERROR;<CONFIG>_OUTPUT_NAME;<CONFIG>_POSTFIX;CROSSCOMPILING_EMULATOR;CUDA_ARCHITECTURES;CUDA_CUBIN_COMPILATION;CUDA_EXTENSIONS;CUDA_FATBIN_COMPILATION;CUDA_OPTIX_COMPILATION;CUDA_PTX_COMPILATION;CUDA_RESOLVE_DEVICE_SYMBOLS;CUDA_RUNTIME_LIBRARY;CUDA_SEPARABLE_COMPILATION;CUDA_STANDARD;CUDA_STANDARD_REQUIRED;CXX_EXTENSIONS;CXX_MODULE_DIRS;CXX_MODULE_DIRS_<NAME>;CXX_MODULE_SET;CXX_MODULE_SET_<NAME>;CXX_SCAN_FOR_MODULES;CXX_STANDARD;CXX_STANDARD_REQUIRED;DEBUG_POSTFIX;DEFINE_SYMBOL;DEPLOYMENT_ADDITIONAL_FILES;DEPLOYMENT_REMOTE_DIRECTORY;DEPRECATION;DISABLE_PRECOMPILE_HEADERS;DLL_NAME_WITH_SOVERSION;DOTNET_SDK;DOTNET_TARGET_FRAMEWORK;DOTNET_TARGET_FRAMEWORK_VERSION;EchoString;ENABLE_EXPORTS;EXCLUDE_FROM_ALL;EXCLUDE_FROM_DEFAULT_BUILD;EXCLUDE_FROM_DEFAULT_BUILD_<CONFIG>;EXPORT_COMPILE_COMMANDS;EXPORT_FIND_PACKAGE_NAME;EXPORT_NAME;EXPORT_NO_SYSTEM;EXPORT_PROPERTIES;FOLDER;Fortran_BUILDING_INSTRINSIC_MODULES;Fortran_FORMAT;Fortran_MODULE_DIRECTORY;Fortran_PREPROCESS;FRAMEWORK;FRAMEWORK_MULTI_CONFIG_POSTFIX_<CONFIG>;FRAMEWORK_VERSION;GENERATOR_FILE_NAME;GHS_INTEGRITY_APP;GHS_NO_SOURCE_GROUP_FILE;GNUtoMS;HAS_CXX;HEADER_DIRS;HEADER_DIRS_<NAME>;HIP_ARCHITECTURES;HIP_EXTENSIONS;HIP_STANDARD;HIP_STANDARD_REQUIRED;IMPLICIT_DEPENDS_INCLUDE_TRANSFORM;IMPORTED;IMPORTED_COMMON_LANGUAGE_RUNTIME;IMPORTED_CONFIGURATIONS;IMPORTED_CXX_MODULES_COMPILE_DEFINITIONS;IMPORTED_CXX_MODULES_COMPILE_FEATURES;IMPORTED_CXX_MODULES_COMPILE_OPTIONS;IMPORTED_CXX_MODULES_INCLUDE_DIRECTORIES;IMPORTED_CXX_MODULES_LINK_LIBRARIES;IMPORTED_GLOBAL;IMPORTED_IMPLIB;IMPORTED_IMPLIB_<CONFIG>;IMPORTED_LIBNAME;IMPORTED_LIBNAME_<CONFIG>;IMPORTED_LINK_DEPENDENT_LIBRARIES;IMPORTED_LINK_DEPENDENT_LIBRARIES_<CONFIG>;IMPORTED_LINK_INTERFACE_LANGUAGES;IMPORTED_LINK_INTERFACE_LANGUAGES_<CONFIG>;IMPORTED_LINK_INTERFACE_LIBRARIES;IMPORTED_LINK_INTERFACE_LIBRARIES_<CONFIG>;IMPORTED_LINK_INTERFACE_MULTIPLICITY;IMPORTED_LINK_INTERFACE_MULTIPLICITY_<CONFIG>;IMPORTED_LOCATION;IMPORTED_LOCATION_<CONFIG>;IMPORTED_NO_SONAME;IMPORTED_NO_SONAME_<CONFIG>;IMPORTED_OBJECTS;IMPORTED_OBJECTS_<CONFIG>;IMPORTED_SONAME;IMPORTED_SONAME_<CONFIG>;IMPORT_PREFIX;IMPORT_SUFFIX;INCLUDE_DIRECTORIES;INSTALL_NAME_DIR;INSTALL_REMOVE_ENVIRONMENT_RPATH;INSTALL_RPATH;INSTALL_RPATH_USE_LINK_PATH;INTERFACE_AUTOMOC_MACRO_NAMES;INTERFACE_AUTOUIC_OPTIONS;INTERFACE_COMPILE_DEFINITIONS;INTERFACE_COMPILE_FEATURES;INTERFACE_COMPILE_OPTIONS;INTERFACE_INCLUDE_DIRECTORIES;INTERFACE_LINK_DEPENDS;INTERFACE_LINK_DIRECTORIES;INTERFACE_LINK_LIBRARIES;INTERFACE_LINK_LIBRARIES_DIRECT;INTERFACE_LINK_LIBRARIES_DIRECT_EXCLUDE;INTERFACE_LINK_OPTIONS;INTERFACE_POSITION_INDEPENDENT_CODE;INTERFACE_PRECOMPILE_HEADERS;INTERFACE_SOURCES;INTERFACE_SYSTEM_INCLUDE_DIRECTORIES;INTERPROCEDURAL_OPTIMIZATION;INTERPROCEDURAL_OPTIMIZATION_<CONFIG>;ISPC_HEADER_DIRECTORY;ISPC_HEADER_SUFFIX;ISPC_INSTRUCTION_SETS;JOB_POOL_COMPILE;JOB_POOL_LINK;JOB_POOL_PRECOMPILE_HEADER;LABELS;<LANG>_CLANG_TIDY;<LANG>_CLANG_TIDY_EXPORT_FIXES_DIR;<LANG>_COMPILER_LAUNCHER;<LANG>_CPPCHECK;<LANG>_CPPLINT;<LANG>_EXTENSIONS;<LANG>_INCLUDE_WHAT_YOU_USE;<LANG>_LINKER_LAUNCHER;<LANG>_STANDARD;<LANG>_STANDARD_REQUIRED;<LANG>_VISIBILITY_PRESET;LIBRARY_OUTPUT_DIRECTORY;LIBRARY_OUTPUT_DIRECTORY_<CONFIG>;LIBRARY_OUTPUT_NAME;LIBRARY_OUTPUT_NAME_<CONFIG>;LINK_DEPENDS;LINK_DEPENDS_NO_SHARED;LINK_DIRECTORIES;LINK_FLAGS;LINK_FLAGS_<CONFIG>;LINK_INTERFACE_LIBRARIES;LINK_INTERFACE_LIBRARIES_<CONFIG>;LINK_INTERFACE_MULTIPLICITY;LINK_INTERFACE_MULTIPLICITY_<CONFIG>;LINK_LIBRARIES;LINK_LIBRARIES_ONLY_TARGETS;LINK_LIBRARY_OVERRIDE;LINK_LIBRARY_OVERRIDE_<LIBRARY>;LINK_OPTIONS;LINK_SEARCH_END_STATIC;LINK_SEARCH_START_STATIC;LINK_WHAT_YOU_USE;LINKER_LANGUAGE;LINKER_TYPE;LOCATION;LOCATION_<CONFIG>;MACHO_COMPATIBILITY_VERSION;MACHO_CURRENT_VERSION;MACOSX_BUNDLE;MACOSX_BUNDLE_INFO_PLIST;MACOSX_FRAMEWORK_INFO_PLIST;MACOSX_RPATH;MANUALLY_ADDED_DEPENDENCIES;MAP_IMPORTED_CONFIG_<CONFIG>;MSVC_DEBUG_INFORMATION_FORMAT;MSVC_RUNTIME_LIBRARY;NAME;NO_SONAME;NO_SYSTEM_FROM_IMPORTED;OBJC_EXTENSIONS;OBJC_STANDARD;OBJC_STANDARD_REQUIRED;OBJCXX_EXTENSIONS;OBJCXX_STANDARD;OBJCXX_STANDARD_REQUIRED;OPTIMIZE_DEPENDENCIES;OSX_ARCHITECTURES;OSX_ARCHITECTURES_<CONFIG>;OUTPUT_NAME;OUTPUT_NAME_<CONFIG>;PCH_INSTANTIATE_TEMPLATES;PCH_WARN_INVALID;PDB_NAME;PDB_NAME_<CONFIG>;PDB_OUTPUT_DIRECTORY;PDB_OUTPUT_DIRECTORY_<CONFIG>;POSITION_INDEPENDENT_CODE;PRECOMPILE_HEADERS;PRECOMPILE_HEADERS_REUSE_FROM;PREFIX;PRIVATE_HEADER;PROJECT_LABEL;PUBLIC_HEADER;RESOURCE;RULE_LAUNCH_COMPILE;RULE_LAUNCH_CUSTOM;RULE_LAUNCH_LINK;RUNTIME_OUTPUT_DIRECTORY;RUNTIME_OUTPUT_DIRECTORY_<CONFIG>;RUNTIME_OUTPUT_NAME;RUNTIME_OUTPUT_NAME_<CONFIG>;SKIP_BUILD_RPATH;SOURCES;SOVERSION;STATIC_LIBRARY_FLAGS;STATIC_LIBRARY_FLAGS_<CONFIG>;STATIC_LIBRARY_OPTIONS;SUFFIX;Swift_COMPILATION_MODE;Swift_DEPENDENCIES_FILE;Swift_LANGUAGE_VERSION;Swift_MODULE_DIRECTORY;Swift_MODULE_NAME;SYSTEM;TEST_LAUNCHER;TYPE;UNITY_BUILD;UNITY_BUILD_BATCH_SIZE;UNITY_BUILD_CODE_AFTER_INCLUDE;UNITY_BUILD_CODE_BEFORE_INCLUDE;UNITY_BUILD_MODE;UNITY_BUILD_UNIQUE_ID;VERSION;VISIBILITY_INLINES_HIDDEN;VS_CONFIGURATION_TYPE;VS_DEBUGGER_COMMAND;VS_DEBUGGER_COMMAND_ARGUMENTS;VS_DEBUGGER_ENVIRONMENT;VS_DEBUGGER_WORKING_DIRECTORY;VS_DESKTOP_EXTENSIONS_VERSION;VS_DOTNET_DOCUMENTATION_FILE;VS_DOTNET_REFERENCE_<refname>;VS_DOTNET_REFERENCEPROP_<refname>_TAG_<tagname>;VS_DOTNET_REFERENCES;VS_DOTNET_REFERENCES_COPY_LOCAL;VS_DOTNET_STARTUP_OBJECT;VS_DOTNET_TARGET_FRAMEWORK_VERSION;VS_DPI_AWARE;VS_GLOBAL_KEYWORD;VS_GLOBAL_PROJECT_TYPES;VS_GLOBAL_ROOTNAMESPACE;VS_GLOBAL_<variable>;VS_IOT_EXTENSIONS_VERSION;VS_IOT_STARTUP_TASK;VS_JUST_MY_CODE_DEBUGGING;VS_KEYWORD;VS_MOBILE_EXTENSIONS_VERSION;VS_NO_COMPILE_BATCHING;VS_NO_SOLUTION_DEPLOY;VS_PACKAGE_REFERENCES;VS_PLATFORM_TOOLSET;VS_PROJECT_IMPORT;VS_SCC_AUXPATH;VS_SCC_LOCALPATH;VS_SCC_PROJECTNAME;VS_SCC_PROVIDER;VS_SDK_REFERENCES;VS_SOLUTION_DEPLOY;VS_SOURCE_SETTINGS_<tool>;VS_USER_PROPS;VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION;VS_WINRT_COMPONENT;VS_WINRT_REFERENCES;WATCOM_RUNTIME_LIBRARY;WIN32_EXECUTABLE;WINDOWS_EXPORT_ALL_SYMBOLS;XCODE_ATTRIBUTE_<an-attribute>;XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY;XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY;XCODE_EMBED_<type>;XCODE_EMBED_<type>_CODE_SIGN_ON_COPY;XCODE_EMBED_<type>_PATH;XCODE_EMBED_<type>_REMOVE_HEADERS_ON_COPY;XCODE_EXPLICIT_FILE_TYPE;XCODE_GENERATE_SCHEME;XCODE_LINK_BUILD_PHASE_MODE;XCODE_PRODUCT_TYPE;XCODE_SCHEME_ADDRESS_SANITIZER;XCODE_SCHEME_ADDRESS_SANITIZER_USE_AFTER_RETURN;XCODE_SCHEME_ARGUMENTS;XCODE_SCHEME_DEBUG_AS_ROOT;XCODE_SCHEME_DEBUG_DOCUMENT_VERSIONING;XCODE_SCHEME_DISABLE_MAIN_THREAD_CHECKER;XCODE_SCHEME_DYNAMIC_LIBRARY_LOADS;XCODE_SCHEME_DYNAMIC_LINKER_API_USAGE;XCODE_SCHEME_ENABLE_GPU_API_VALIDATION;XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE;XCODE_SCHEME_ENABLE_GPU_SHADER_VALIDATION;XCODE_SCHEME_ENVIRONMENT;XCODE_SCHEME_EXECUTABLE;XCODE_SCHEME_GUARD_MALLOC;XCODE_SCHEME_LAUNCH_CONFIGURATION;XCODE_SCHEME_LAUNCH_MODE;XCODE_SCHEME_MAIN_THREAD_CHECKER_STOP;XCODE_SCHEME_MALLOC_GUARD_EDGES;XCODE_SCHEME_MALLOC_SCRIBBLE;XCODE_SCHEME_MALLOC_STACK;XCODE_SCHEME_THREAD_SANITIZER;XCODE_SCHEME_THREAD_SANITIZER_STOP;XCODE_SCHEME_UNDEFINED_BEHAVIOUR_SANITIZER;XCODE_SCHEME_UNDEFINED_BEHAVIOUR_SANITIZER_STOP;XCODE_SCHEME_WORKING_DIRECTORY;XCODE_SCHEME_ZOMBIE_OBJECTS;XCODE_XCCONFIG;XCTEST")
  set(result "")

  set(supported_cmake_langs "C;CXX;CSharp;CUDA;OBJC;OBJCXX;Fortran;HIP;ISPC;Swift;ASM;ASM_NASM;ASM_MARMASM;ASM_MASM;ASM-ATT") # as per https://cmake.org/cmake/help/latest/command/enable_language.html
  set(supported_cmake_build_types "DEBUG;RELEASE;RELWITHDEBINFO;MINSIZEREL")

  if(DEFINED CMAKE_BUILD_TYPE)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" current_build_type_UPPER)
    list(APPEND supported_cmake_build_types "${current_build_type_UPPER}")
  endif()

  list(REMOVE_DUPLICATES supported_cmake_build_types)

  hfc_log(TRACE " -- creating HERMETIC_FETCHCONTENT_TARGETSCACHE_SUPPORTED_PROPERTIES")

  foreach(property_name IN LISTS known_cmake_variables)
    if("${property_name}" MATCHES <.+>)

      if("${property_name}" MATCHES <CONFIG>)
        foreach(entry IN LISTS supported_cmake_build_types)
          string(REPLACE "<CONFIG>" "${entry}" result_property_name "${property_name}")
          list(APPEND result "${result_property_name}")
        endforeach()
      elseif("${property_name}" MATCHES <LANG>)
        foreach(entry IN LISTS supported_cmake_langs)
          string(REPLACE "<CONFIG>" "${entry}" result_property_name "${property_name}")
          list(APPEND result "${result_property_name}")
        endforeach()
      else()
      hfc_log(TRACE " -- unsupported specialization of target properties ${property_name}")
      endif()
    else()
      list(APPEND result "${property_name}")
    endif()
  endforeach()

  hfc_log(TRACE " -- DONE")

  set(HERMETIC_FETCHCONTENT_TARGETSCACHE_SUPPORTED_PROPERTIES "${result}" PARENT_SCOPE)
endfunction()

set(HERMETIC_FETCHCONTENT_TARGETSCACHE_SUPPORTED_PROPERTIES "")
__compute_HERMETIC_FETCHCONTENT_TARGETSCACHE_SUPPORTED_PROPERTIES()


#
# load target information from <library>Targets.cmake (or exports.cmake) files in a
# library build or install tree
#
# Usage:
# hfc_targets_cache_create(
#   LOAD_TARGETS_COMMAND <cmake function name to invoke for loading targets>
#   LOAD_TARGETS_CMAKE <cmake code to eval for loading targets>
#   CACHE_DESTINATION_FILE <path>         # path to the target info cache to write. Parent directory will be created if not already present. Destination file will be overwritten if present
# )
function(hfc_targets_cache_create)

  # arguments parsing
  set(options "")
  set(oneValueArgs LOAD_TARGETS_COMMAND LOAD_TARGETS_CMAKE CACHE_DESTINATION_FILE CREATE_TARGET_ALIASES_FN)
  set(multiValueArgs )
  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if("${FN_ARG_LOAD_TARGETS_CMAKE}" STREQUAL "")

    if("${FN_ARG_LOAD_TARGETS_COMMAND}" STREQUAL "")
      hfc_log(FATAL_ERROR "hfc_targets_cache_create needs a value for LOAD_TARGETS_COMMAND or/and LOAD_TARGETS_CMAKE parameters to be defined")
    endif()

    if(NOT COMMAND ${FN_ARG_LOAD_TARGETS_COMMAND})
      hfc_log(FATAL_ERROR "Supplied value for argument CACHE_DESTINATION_FILE does not reference an existing function")
    endif()

  endif()

  if(NOT FN_ARG_CACHE_DESTINATION_FILE)
    hfc_log(FATAL_ERROR "hfc_targets_cache_create needs a value for the CACHE_DESTINATION_FILE parameter to be defined")
  endif()

  # have a baseline so we only process new targets
  get_property(targets_before_import DIRECTORY PROPERTY IMPORTED_TARGETS)

  if(COMMAND ${FN_ARG_LOAD_TARGETS_COMMAND})
    cmake_language(CALL "${FN_ARG_LOAD_TARGETS_COMMAND}")
  endif()

  if(NOT "${FN_ARG_LOAD_TARGETS_CMAKE}" STREQUAL "")
    cmake_language(EVAL CODE "${FN_ARG_LOAD_TARGETS_CMAKE}")
  endif()


  # gather imported targets
  get_property(targets_after_import DIRECTORY PROPERTY IMPORTED_TARGETS)

  # gather the list of newly declared targets
  set(OUT_targets_list "")
  foreach(target IN LISTS targets_after_import)
    if(NOT "${target}" IN_LIST targets_before_import)
      list(APPEND OUT_targets_list "${target}")
    else()
    endif()
  endforeach()

  #
  # Generate the content for the targets cache file
  #
  hfc_log_debug("Generating targets cache data")

  set(cached_targets "")
  set(targets_cache_content "")
  string(APPEND targets_cache_content "# This file has been generated by HermeticFetchContent\n")
  string(APPEND targets_cache_content "#\n\n")

  # gather all properties and export them
  foreach(target IN LISTS OUT_targets_list)

    if(COMMAND "${FN_ARG_CREATE_TARGET_ALIASES_FN}")
      hfc_log_debug(" - invoking alias defintion")
      cmake_language(CALL ${FN_ARG_CREATE_TARGET_ALIASES_FN} "${target}" aliases)
      hfc_log_debug(" -> aliases: ${aliases}")
    else()
      set(aliases "${target}")
    endif()

    foreach(alias IN LISTS aliases)

      list(APPEND cached_targets "${alias}")

      string(APPEND targets_cache_content "# Target: '${target}'\n")
      if(NOT alias STREQUAL target)
        string(APPEND targets_cache_content "# - alias of: '${alias}'\n")
      endif()

      set(found_target_properties "")

      foreach(property_name IN LISTS HERMETIC_FETCHCONTENT_TARGETSCACHE_SUPPORTED_PROPERTIES)

        get_target_property(property_value ${target} "${property_name}")

        if(NOT "${property_value}" STREQUAL "property_value-NOTFOUND")
          hfc_log_debug(" - Found property ${property_name} = ${property_value}")

          Hermetic_FetchContent_TargetsCache_getExportVariableName(
            TARGET_NAME "${alias}"
            PROPERTY_NAME "${property_name}"
            EXPORT_VARIABLE_PREFIX "${HERMETIC_FETCHCONTENT_TARGET_VARIABLE_PREFIX}"
            RESULT export_variable_name
          )

          if(property_name STREQUAL "NAME")
            set(property_value "${alias}")
          endif()

          # filter generator expressions for $<BUILD_INTERFACE...> and the like if present in INTERFACE_INCLUDE_DIRECTORIES
          if(property_name STREQUAL "INTERFACE_INCLUDE_DIRECTORIES")

            set(resulting_value "")

            foreach(entry IN LISTS property_value)
              if(entry MATCHES "^\\\$<BUILD_INTERFACE.+>$") # double escaping necessary bc. generator expression
                # if for some odd reason we have a $<BUILD_INTERFACE:...> generator expression we will just skip it,
                # there are a couple of other possibilities we don't want to discard (like $<TARGET_PROPERTY:...>
                # that are used correctly)
                continue()
              else()
                list(APPEND resulting_value "${entry}")
              endif()
            endforeach()

            set(property_value "${resulting_value}")

          endif()

          list(APPEND found_target_properties "${property_name}")
          string(APPEND targets_cache_content "set(${export_variable_name} \"${property_value}\")\n")
        endif()

      endforeach()

      string(APPEND targets_cache_content "\n")

      Hermetic_FetchContent_TargetsCache_get_found_property_variable_name("${alias}" found_properties_var_name)
      string(APPEND targets_cache_content "set(${found_properties_var_name} \"${found_target_properties}\")\n")

      string(APPEND targets_cache_content "\n")

    endforeach()

  endforeach()

  # add a list of all targets
  string(APPEND targets_cache_content "\n")
  string(APPEND targets_cache_content "# targets list in cache\n")
  string(APPEND targets_cache_content "set(${HERMETIC_FETCHCONTENT_TARGET_LIST_NAME} \"${cached_targets}\")\n")
  string(APPEND targets_cache_content "\n\n")
  string(APPEND targets_cache_content "# END OF FILE")

  # write that file to dest
  get_filename_component(parent_path "${FN_ARG_CACHE_DESTINATION_FILE}" PATH)
  if(NOT EXISTS "${parent_path}")
    file(MAKE_DIRECTORY "${parent_path}")
  endif()

  hfc_log_debug("Writing targets cache data to ${FN_ARG_CACHE_DESTINATION_FILE}")
  file(WRITE "${FN_ARG_CACHE_DESTINATION_FILE}" "${targets_cache_content}")

endfunction()

#
# runs hfc_targets_cache_create_isolated in an isolated context (in a separate CMake process)
#
# Usage:
# hfc_targets_cache_create_isolated(
#   TEMP_DIR                              # temporary folder to
#   LOAD_TARGETS_COMMAND <cmake function name to invoke for loading targets>
#   LOAD_TARGETS_CMAKE <cmake code to eval for loading targets>
#   CACHE_DESTINATION_FILE <path>         # path to the target info cache to write. Parent directory will be created if not already present. Destination file will be overwritten if present
# )
function(hfc_targets_cache_create_isolated content_name)

  # arguments parsing
  set(options "")
  set(oneValueArgs
    LOAD_TARGETS_COMMAND
    LOAD_TARGETS_CMAKE
    TOOLCHAIN_FILE
    CACHE_DESTINATION_FILE
    CREATE_TARGET_ALIASES
    TEMP_DIR
  )
  set(multiValueArgs )
  cmake_parse_arguments(FN_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  string(RANDOM LENGTH 10 rand_str)
  set(tmp_proj_dir "${FN_ARG_TEMP_DIR}/tmp_${rand_str}")

  file(MAKE_DIRECTORY "${tmp_proj_dir}")

  # make sure we have the same enabled languages if the FORCE_SYSTEM_<content-name>
  set(TEMPLATE_ENABLE_LANGUAGES "NONE")
  if(FORCE_SYSTEM_${content_name})
    get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    set(TEMPLATE_ENABLE_LANGUAGES ${languages})
  endif()

  block(SCOPE_FOR VARIABLES PROPAGATE
    HERMETIC_FETCHCONTENT_ROOT_DIR
    FN_ARG_LOAD_TARGETS_CMAKE
    FN_ARG_CACHE_DESTINATION_FILE
    FN_ARG_CREATE_TARGET_ALIASES
    TEMPLATE_ENABLE_LANGUAGES
  )
    set(HERMETIC_FETCHCONTENT_ROOT_DIR "${HERMETIC_FETCHCONTENT_ROOT_DIR}")
    set(LOAD_TARGETS_CMAKE "${FN_ARG_LOAD_TARGETS_CMAKE}")
    set(CACHE_DESTINATION_FILE "${FN_ARG_CACHE_DESTINATION_FILE}")
    set(CREATE_TARGET_ALIASES "${FN_ARG_CREATE_TARGET_ALIASES}")
    configure_file(
      "${HERMETIC_FETCHCONTENT_ROOT_DIR}/templates/create_imported_cmake_targets_cache.CMakeLists.txt.in"
      "${tmp_proj_dir}/CMakeLists.txt"
      @ONLY
    )
  endblock()

  execute_process(
    COMMAND ${CMAKE_COMMAND} "." "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}" "-DCMAKE_TOOLCHAIN_FILE=${FN_ARG_TOOLCHAIN_FILE}"
    WORKING_DIRECTORY "${tmp_proj_dir}"
    RESULT_VARIABLE CONFIGURE_RESULT
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO STDOUT
  )

  file(REMOVE_RECURSE "${tmp_proj_dir}")

endfunction()


#
# Creates a target cache file from an export declaration that can be executed
#
# Usage:
# hfc_targets_cache_create_from_export_declaration(
#   PROJECT_INSTALL_PREFIX
#   CMAKE_EXPORT_LIBRARY_DECLARATION <cmake code to evaluate for loading targets>
#   TOOLCHAIN_FILE <cmake code to eval for loading targets>
#   OUT_TARGETS_CACHE_FILE <path>         # path to the target info cache to write. Parent directory will be created if not already present. Destination file will be overwritten if present
# )
function(hfc_targets_cache_create_from_export_declaration content_name)

  set(options_params "")
  set(one_value_params
    PROJECT_INSTALL_PREFIX
    CMAKE_EXPORT_LIBRARY_DECLARATION
    TOOLCHAIN_FILE
    OUT_TARGETS_CACHE_FILE
  )

  set(multi_value_params "")

  cmake_parse_arguments(
    FN_ARG
    "${options_params}"
    "${one_value_params}"
    "${multi_value_params}"
    ${ARGN}
  )

  get_hermetic_target_cache_file_path(${content_name} ${FN_ARG_OUT_TARGETS_CACHE_FILE})

  hfc_log_debug("Generating targets cache for ${content_name} from export declaration at ${${FN_ARG_OUT_TARGETS_CACHE_FILE}}")

  hfc_targets_cache_create_isolated(
    ${content_name}
    LOAD_TARGETS_CMAKE "[==[${FN_ARG_CMAKE_EXPORT_LIBRARY_DECLARATION}]==]"
    TOOLCHAIN_FILE ${FN_ARG_TOOLCHAIN_FILE}
    CACHE_DESTINATION_FILE "${${FN_ARG_OUT_TARGETS_CACHE_FILE}}"
    TEMP_DIR "${HERMETIC_FETCHCONTENT_INSTALL_DIR}/targets_dump_tmp"
  )

  return(PROPAGATE ${FN_ARG_OUT_TARGETS_CACHE_FILE})
endfunction()