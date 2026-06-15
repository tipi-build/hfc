include_guard(GLOBAL)

include(${CMAKE_CURRENT_LIST_DIR}/hfc_log.cmake)

#[=======================================================================[.rst:
hfc_scan_file_flags
------------------------------------------------------------------------------

Detect compiler / linker flags that take a *file* as their argument (e.g.
``-fsanitize-ignorelist=msan.ignore``) inside a set of flag strings, and
produce a deterministic fingerprint contribution that folds the **content**
hash of each referenced file into the caller's toolchain fingerprint.

Without this, HFC's fingerprint only captures the flag *string*; the file the
flag points at is invisible, so editing e.g. a sanitizer ignorelist does not
invalidate already-built dependencies and they get reused with stale
instrumentation. Users previously worked around this by hashing the file
themselves and injecting the digest on the command line -- this module removes
that need by detecting the file argument automatically.

The built-in table covers the common GCC and Clang file-bearing flags
(sanitizer / coverage ignore-lists, PGO / sample / auto profiles, XRay lists,
modules, plugins / specs, preprocessor ``-include`` / ``-imacros``, linker
``--version-script`` / ``--dynamic-list`` / ``-T`` etc. including ``-Wl,`` and
``-Xlinker`` wrapping, and ``@response`` files). It can be extended via the
global ``HERMETIC_FETCHCONTENT_ADDITIONAL_FILE_FLAGS`` list.

Detection can be disabled wholesale via
``HERMETIC_FETCHCONTENT_DISABLE_FILE_FLAG_DETECTION``.

A referenced file that cannot be resolved (relative path we cannot anchor, or
a missing file) is skipped with a debug log; it never aborts configuration,
since a partially-parsed flag must not break an otherwise valid build.
#]=======================================================================]

# Flags whose FILE argument is ATTACHED with '=' (i.e. "flag=path").
set(_HFC_FILE_FLAGS_EQ
  # Clang/GCC sanitizer & coverage lists
  "-fsanitize-ignorelist"
  "-fsanitize-blacklist"
  "-fsanitize-system-ignorelist"
  "-fsanitize-coverage-ignorelist"
  "-fsanitize-coverage-allowlist"
  "-fsanitize-memory-ignorelist"
  # Clang XRay instrumentation lists
  "-fxray-attr-list"
  "-fxray-always-instrument"
  "-fxray-never-instrument"
  # PGO / sample / auto profiles (Clang + GCC)
  "-fprofile-list"
  "-fprofile-use"
  "-fprofile-instr-use"
  "-fprofile-sample-use"
  "-fauto-profile"
  "-fmemory-profile-use"
  # Modules / config (Clang + GCC)
  "-fmodule-file"
  "-fmodule-mapper"
  "--config"
  # Plugins / specs (GCC)
  "-fplugin"
  "-specs"
  "--specs"
)

# Flags whose FILE argument is the NEXT token (i.e. "flag path").
set(_HFC_FILE_FLAGS_SEP
  "-include"
  "-imacros"
  "-include-pch"
)

# Linker flags (seen after unwrapping "-Wl," / "-Xlinker"). Both the attached
# ("flag=path") and separated ("flag path") forms are accepted for these.
set(_HFC_FILE_FLAGS_LINKER
  "--version-script"
  "--dynamic-list"
  "--retain-symbols-file"
  "--just-symbols"
  "--script"
  "-T"
)

# Hash a single referenced file into the accumulator if it resolves to an
# existing regular file. base_dirs anchors relative paths (best effort).
# Appends a "file_flag_input:<resolved>=<sha256>\n" line to ${acc_var}.
function(_hfc_file_flags_hash_into acc_var raw_path base_dirs)
  if(raw_path STREQUAL "")
    return()
  endif()

  set(resolved "")
  if(IS_ABSOLUTE "${raw_path}")
    if(EXISTS "${raw_path}")
      set(resolved "${raw_path}")
    endif()
  else()
    foreach(base IN LISTS base_dirs)
      if(EXISTS "${base}/${raw_path}")
        get_filename_component(resolved "${base}/${raw_path}" ABSOLUTE)
        break()
      endif()
    endforeach()
  endif()

  if(resolved STREQUAL "" OR IS_DIRECTORY "${resolved}")
    hfc_log_debug(" - file-flag input not resolvable to a file, skipping: '${raw_path}'")
    return()
  endif()

  file(SHA256 "${resolved}" file_hash)
  hfc_log_debug(" - file-flag input hashed: '${resolved}' -> ${file_hash}")
  set(_acc "${${acc_var}}")
  string(APPEND _acc "file_flag_input:${resolved}=${file_hash}\n")
  set(${acc_var} "${_acc}" PARENT_SCOPE)
endfunction()

# Match a single flag token (already unwrapped from -Wl,/-Xlinker if needed)
# against a table of "flag" prefixes using the attached "flag=path" form.
# Returns the extracted path in OUT_path (empty if no match).
function(_hfc_file_flags_match_eq token flag_table OUT_path)
  set(${OUT_path} "" PARENT_SCOPE)
  foreach(flag IN LISTS flag_table)
    if(token MATCHES "^${flag}=(.+)$")
      set(path "${CMAKE_MATCH_1}")
      # -fmodule-file may be "name=path"; the file is after the last '='.
      if(flag STREQUAL "-fmodule-file" AND path MATCHES "=")
        string(REGEX REPLACE "^.*=" "" path "${path}")
      endif()
      set(${OUT_path} "${path}" PARENT_SCOPE)
      return()
    endif()
  endforeach()
endfunction()

#[=======================================================================[.rst:
hfc_scan_file_flags(<out_var>
    STRINGS    <flag string>...
    [BASE_DIRS <dir>...]
)

Scan the given flag STRINGS for file-bearing flags and set ``<out_var>`` in the
caller's scope to a deterministic, sorted fingerprint contribution (possibly
empty). Each STRINGS entry may be a whitespace- and/or ``;``-separated flag
string (as found in ``CMAKE_<LANG>_FLAGS`` or a ``COMPILE_OPTIONS`` directory
property). Relative paths are anchored against BASE_DIRS in order.
#]=======================================================================]
function(hfc_scan_file_flags out_var)
  cmake_policy(SET CMP0057 NEW)  # IN_LIST operator
  cmake_parse_arguments(PARSE_ARGV 1 FN_ARG "" "" "STRINGS;BASE_DIRS")

  set(${out_var} "" PARENT_SCOPE)

  if(HERMETIC_FETCHCONTENT_DISABLE_FILE_FLAG_DETECTION)
    return()
  endif()

  # Merge the user-extensible flag list into the attached-form table.
  set(eq_flags ${_HFC_FILE_FLAGS_EQ} ${HERMETIC_FETCHCONTENT_ADDITIONAL_FILE_FLAGS})

  # Flatten every STRINGS entry into one ordered token list. ';' (cmake list
  # separator inside directory properties) is normalised to whitespace, then
  # UNIX_COMMAND tokenisation handles quoting/spacing uniformly.
  set(tokens "")
  foreach(s IN LISTS FN_ARG_STRINGS)
    string(REPLACE ";" " " s "${s}")
    separate_arguments(_toks UNIX_COMMAND "${s}")
    list(APPEND tokens ${_toks})
  endforeach()

  set(accumulator "")
  list(LENGTH tokens token_count)
  set(idx 0)
  while(idx LESS token_count)
    list(GET tokens ${idx} token)

    # @response-file
    if(token MATCHES "^@(.+)$")
      _hfc_file_flags_hash_into(accumulator "${CMAKE_MATCH_1}" "${FN_ARG_BASE_DIRS}")

    # -Wl,opt,opt,... : unwrap and scan the comma-separated linker sub-tokens
    # (handles both "--version-script=foo" and "--version-script,foo" forms).
    elseif(token MATCHES "^-Wl,(.+)$")
      string(REPLACE "," ";" linker_toks "${CMAKE_MATCH_1}")
      list(LENGTH linker_toks lcount)
      set(lidx 0)
      while(lidx LESS lcount)
        list(GET linker_toks ${lidx} ltok)
        _hfc_file_flags_match_eq("${ltok}" "${_HFC_FILE_FLAGS_LINKER}" lpath)
        if(NOT lpath STREQUAL "")
          _hfc_file_flags_hash_into(accumulator "${lpath}" "${FN_ARG_BASE_DIRS}")
        elseif(ltok IN_LIST _HFC_FILE_FLAGS_LINKER)
          math(EXPR lnext "${lidx} + 1")
          if(lnext LESS lcount)
            list(GET linker_toks ${lnext} lval)
            _hfc_file_flags_hash_into(accumulator "${lval}" "${FN_ARG_BASE_DIRS}")
            set(lidx ${lnext})
          endif()
        endif()
        math(EXPR lidx "${lidx} + 1")
      endwhile()

    # -Xlinker <opt> : the next token is a single linker argument. A linker
    # flag and its file may arrive as two consecutive -Xlinker tokens.
    elseif(token STREQUAL "-Xlinker")
      math(EXPR next "${idx} + 1")
      if(next LESS token_count)
        list(GET tokens ${next} xtok)
        _hfc_file_flags_match_eq("${xtok}" "${_HFC_FILE_FLAGS_LINKER}" xpath)
        if(NOT xpath STREQUAL "")
          _hfc_file_flags_hash_into(accumulator "${xpath}" "${FN_ARG_BASE_DIRS}")
        elseif(xtok IN_LIST _HFC_FILE_FLAGS_LINKER)
          # value comes as the following "-Xlinker <file>" pair
          math(EXPR nnext "${idx} + 3")
          if(nnext LESS token_count)
            list(GET tokens ${nnext} xval)
            _hfc_file_flags_hash_into(accumulator "${xval}" "${FN_ARG_BASE_DIRS}")
          endif()
        endif()
        set(idx ${next})
      endif()

    else()
      # attached "flag=path" form (compiler flags + any linker flag passed
      # directly, e.g. -T on some drivers)
      _hfc_file_flags_match_eq("${token}" "${eq_flags};${_HFC_FILE_FLAGS_LINKER}" path)
      if(NOT path STREQUAL "")
        _hfc_file_flags_hash_into(accumulator "${path}" "${FN_ARG_BASE_DIRS}")
      elseif(token IN_LIST _HFC_FILE_FLAGS_SEP OR token IN_LIST _HFC_FILE_FLAGS_LINKER)
        # separated "flag path" form
        math(EXPR next "${idx} + 1")
        if(next LESS token_count)
          list(GET tokens ${next} val)
          _hfc_file_flags_hash_into(accumulator "${val}" "${FN_ARG_BASE_DIRS}")
          set(idx ${next})
        endif()
      endif()
    endif()

    math(EXPR idx "${idx} + 1")
  endwhile()

  if(accumulator STREQUAL "")
    return()
  endif()

  # Deterministic ordering regardless of flag/scan order.
  string(STRIP "${accumulator}" accumulator)
  string(REPLACE "\n" ";" acc_lines "${accumulator}")
  list(REMOVE_DUPLICATES acc_lines)
  list(SORT acc_lines)
  list(JOIN acc_lines "\n" result)

  set(${out_var} "${result}\n" PARENT_SCOPE)
endfunction()
