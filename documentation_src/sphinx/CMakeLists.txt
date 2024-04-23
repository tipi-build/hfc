# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

# tipi defaults
set(SPHINX_HTML ON)
set(CMake_SPHINX_CMAKE_ORG ON)
set(CMake_VERSION_NO_GIT "v0.0.0")

if(NOT CMake_SOURCE_DIR)
  set(CMakeHelp_STANDALONE 1)
  cmake_minimum_required(VERSION 3.13...3.27 FATAL_ERROR)
  get_filename_component(tmp "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
  get_filename_component(CMake_SOURCE_DIR "${tmp}" PATH)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake @ONLY)
  unset(CMAKE_DATA_DIR)
  unset(CMAKE_DATA_DIR CACHE)
  macro(CMake_OPTIONAL_COMPONENT)
    set(COMPONENT "")
  endmacro()
endif()
project(CMakeHelp NONE)

option(SPHINX_INFO "Build Info manual with Sphinx" OFF)
option(SPHINX_MAN "Build man pages with Sphinx" OFF)
option(SPHINX_HTML "Build html help with Sphinx" OFF)
option(SPHINX_SINGLEHTML "Build html single page help with Sphinx" OFF)
option(SPHINX_LINKCHECK "Check external links mentioned in documentation" OFF)
option(SPHINX_QTHELP "Build Qt help with Sphinx" OFF)
option(SPHINX_LATEXPDF "Build PDF help with Sphinx using LaTeX" OFF)
option(SPHINX_TEXT "Build text help with Sphinx (not installed)" OFF)
find_program(SPHINX_EXECUTABLE
  NAMES sphinx-build
  DOC "Sphinx Documentation Builder (sphinx-doc.org)"
  )
set(SPHINX_FLAGS "" CACHE STRING "Flags to pass to sphinx-build")
separate_arguments(sphinx_flags UNIX_COMMAND "${SPHINX_FLAGS}")

mark_as_advanced(SPHINX_TEXT)
mark_as_advanced(SPHINX_FLAGS)

if(NOT (SPHINX_INFO
  OR SPHINX_MAN
  OR SPHINX_HTML
  OR SPHINX_SINGLEHTML
  OR SPHINX_LINKCHECK
  OR SPHINX_QTHELP
  OR SPHINX_TEXT
  OR SPHINX_LATEXPDF
 ))
  return()
elseif(NOT SPHINX_EXECUTABLE)
  message(FATAL_ERROR "SPHINX_EXECUTABLE (sphinx-build) is not found!")
endif()

set(conf_copyright "tipi technologies Ltd.")
set(conf_docs "${CMake_SOURCE_DIR}")
set(conf_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(conf_version "${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}.${CMake_VERSION_PATCH}")
set(conf_release "${CMake_VERSION}")
configure_file(conf.py.in conf.py @ONLY)

set(doc_formats "")
if(SPHINX_HTML)
  list(APPEND doc_formats html)

  # we provide the path to the produced html output in the console
  # for tools that support URI protocol schemes
  set(html_post_commands
    COMMAND ${CMAKE_COMMAND} -E echo "sphinx-build html: HTML documentation generated in ${CMAKE_CURRENT_BINARY_DIR}/html/index.html run install target with --prefix to install to desired location"
  )

endif()

set(doc_html_opts "")
if(CMake_SPHINX_CMAKE_ORG)
  list(APPEND doc_html_opts
    -A googleanalytics=0
    -A opensearch=1
    -A versionswitch=1
    )

  if(CMake_SPHINX_CMAKE_ORG_OUTDATED)
    list(APPEND doc_html_opts -A outdated=1)
  endif()

  list(APPEND html_pre_commands
    COMMAND ${CMAKE_COMMAND} -Dversion=${CMake_VERSION} -P ${CMAKE_CURRENT_SOURCE_DIR}/tutorial_archive.cmake
    )
endif()

# Redirect `sphinx-build` output to `build-<format>.log` file?
set(sphinx_use_build_log TRUE)
set(sphinx_verbose_levels "DEBUG;TRACE")
set(sphinx_no_redirect_levels "VERBOSE;${sphinx_verbose_levels}")
# NOTE There is no generic verbosity level for all supported generators,
# so lets use CMake verbosity level to control if `sphinx-build` should
# redirect it's output to a file or a user wants to see it at build time.
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.25)
  cmake_language(GET_MESSAGE_LOG_LEVEL verbose_level)
else()
  # If building under CMake < 3.25, fallback to `CMAKE_MESSAGE_LOG_LEVEL`
  # variable.  It was added in 3.17 but it's OK to set it even for older
  # versions (w/o any effect on `message()` command of course).
  set(verbose_level ${CMAKE_MESSAGE_LOG_LEVEL})
endif()
if(DEFINED ENV{VERBOSE} OR CMAKE_VERBOSE_MAKEFILE OR verbose_level IN_LIST sphinx_no_redirect_levels)
  set(sphinx_use_build_log FALSE)
  if(verbose_level IN_LIST sphinx_verbose_levels)
    # NOTE Sphinx accept multiple `-v` options for more verbosity
    # but the output mostly for Sphinx developers...
    list(APPEND sphinx_flags "-v")
  endif()
endif()

set(doc_format_outputs "")
set(doc_format_last "")
foreach(format IN LISTS doc_formats)
  set(doc_format_output "doc_format_${format}")
  set(doc_format_log "")
  set(build_comment_tail " ...")
  if(sphinx_use_build_log)
    set(doc_format_log "build-${format}.log")
    set(build_comment_tail ": build log to be found in: ${CMAKE_CURRENT_BINARY_DIR}/${doc_format_log}")
    list(PREPEND doc_format_log ">")
  endif()
  if(CMake_SPHINX_CMAKE_ORG)
    set(doctrees "doctrees/${format}")
  else()
    set(doctrees "doctrees")
  endif()
  
  set(_args
      -c ${CMAKE_CURRENT_BINARY_DIR}
      -d ${CMAKE_CURRENT_BINARY_DIR}/${doctrees}
      -b ${format}
      ${sphinx_flags}
      ${doc_${format}_opts}
      ${CMake_SOURCE_DIR}/documentation_src
      ${CMAKE_CURRENT_BINARY_DIR}/${format}
    )

  add_custom_command(
    OUTPUT ${doc_format_output}
    ${${format}_pre_commands}
    COMMAND ${SPHINX_EXECUTABLE} ${_args} ${doc_format_log}
    ${${format}_post_commands}
    DEPENDS ${doc_format_last}
    COMMENT "sphinx-build ${format}${build_comment_tail}"
    VERBATIM
  )

  set_property(SOURCE ${doc_format_output} PROPERTY SYMBOLIC 1)
  list(APPEND doc_format_outputs ${doc_format_output})
  if(NOT CMake_SPHINX_CMAKE_ORG)
    set(doc_format_last ${doc_format_output})
  endif()
endforeach()

add_custom_target(documentation ALL DEPENDS ${doc_format_outputs})

if(CMake_SPHINX_DEPEND_ON_EXECUTABLES)
  foreach(t IN ITEMS cmake ccmake cmake-gui cpack ctest)
    if(TARGET ${t})
      # Build documentation after main executables.
      add_dependencies(documentation ${t})
    endif()
  endforeach()
endif()

install(CODE "if(EXISTS \"\${CMAKE_INSTALL_PREFIX}\")\nmessage(WARNING \"Install target folder is not empty - please clean first to be sure there's no leftovers - proceeding anyway\")\nfile(TOUCH \${CMAKE_INSTALL_PREFIX}/.dirtyinstall)\nendif()")
install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
  DESTINATION "."
  PATTERN .buildinfo EXCLUDE
)
install(CODE "if(EXISTS \"\${CMAKE_INSTALL_PREFIX}/.dirtyinstall\")\nmessage(WARNING \"Install target folder was not empty - please clean first before submitting to production to be sure there was leftovers\")\nfile(REMOVE \${CMAKE_INSTALL_PREFIX}/.dirtyinstall)\nendif()")