include_guard()

include(hfc_log)
include(hfc_git_helpers)

#[=======================================================================[.rst:
hfc_populate_cache_state
------------------------------------------------------------------------------------------
HFC-owned warm/cold detection for the central source cache.

HFC only ever calls FetchContent_Populate() against a COLD (missing/empty)
source dir: populating into an empty dir is safe under both FetchContent
implementations (sub-build and CMP0168 direct population), while re-running
populate against a warm cache is what nukes/re-downloads shared sources under
direct population (its stamps live per consumer build tree, see
tipi-build/specs-cmake-re#96). Warmness is decided here, from the cache
contents themselves:

- git content: the clone IS the state — HEAD at the declared GIT_TAG means
  warm (local uncommitted modifications are allowed: they are the debugging
  workflow of a56de21d). HEAD elsewhere on a valid clone means UPDATE
  (fetch + checkout in place, never a re-clone).
- URL content: a populate marker file written next to the source dir after a
  successful populate (or, for caches populated before the marker existed,
  the legacy sub-build populate-complete stamp).
#]=======================================================================]

# The marker is a sibling of the source dir so it travels with the cache on
# relocation and survives consumer build-tree wipes.
function(hfc_populate_marker_path source_dir OUT_result)
  string(REGEX REPLACE "/+$" "" source_dir_normalized "${source_dir}")
  set(${OUT_result} "${source_dir_normalized}.hfc_populated" PARENT_SCOPE)
endfunction()

function(hfc_populate_marker_write content_name source_dir)
  hfc_populate_marker_path("${source_dir}" marker_path)
  file(WRITE "${marker_path}" "populated by Hermetic FetchContent: ${content_name}\n")
endfunction()

function(hfc_populate_marker_remove source_dir)
  hfc_populate_marker_path("${source_dir}" marker_path)
  file(REMOVE "${marker_path}")
endfunction()

#
# Classify the cache state for a content into OUT_STATE:
#   WARM    sources present and correct — do not populate
#   UPDATE  valid git clone at another revision — update in place
#   CORRUPT non-empty dir that is neither a valid clone nor a completed
#           populate — invalidate, then treat as COLD
#   COLD    populate required (dir missing or empty)
#
# LEGACY_SUBBUILD_DIR is where the pre-direct-population sub-build lived for
# this content; its populate-complete stamp marks pre-existing caches warm.
function(hfc_populate_check_cache_state content_name)
  set(one_value_params
    SOURCE_DIR
    GIT_REPOSITORY
    GIT_TAG
    URL
    LEGACY_SUBBUILD_DIR
    OUT_STATE
  )
  cmake_parse_arguments(FN_ARG "" "${one_value_params}" "" ${ARGN})

  set(source_dir_has_content FALSE)
  if(EXISTS "${FN_ARG_SOURCE_DIR}")
    file(GLOB existing_entries "${FN_ARG_SOURCE_DIR}/*")
    if(existing_entries)
      set(source_dir_has_content TRUE)
    endif()
  endif()

  if(NOT source_dir_has_content)
    set(${FN_ARG_OUT_STATE} "COLD" PARENT_SCOPE)
    return()
  endif()

  if(FN_ARG_GIT_REPOSITORY)

    is_git_repository(REPOSITORY_DIR "${FN_ARG_SOURCE_DIR}" OUT_RESULT is_git_repo)
    if(NOT is_git_repo)
      # non-empty but no valid clone (e.g. .git removed): not a deliberate dev
      # checkout (those stay valid git repos), clear it for a fresh clone
      set(${FN_ARG_OUT_STATE} "CORRUPT" PARENT_SCOPE)
      return()
    endif()

    repo_get_head_id(REPOSITORY_DIR "${FN_ARG_SOURCE_DIR}" OUT_COMMIT_ID head_commit_id)

    if(head_commit_id STREQUAL FN_ARG_GIT_TAG)
      set(${FN_ARG_OUT_STATE} "WARM" PARENT_SCOPE)
      return()
    endif()

    # a git *tag* name resolving to HEAD is warm (tags are immutable by
    # convention). Anything else — including branch names, which are expected
    # to move — takes the UPDATE path, whose fetch follows the branch when the
    # origin is reachable and falls back to the local state when it is not.
    git_exec(COMMAND "${git_executable} rev-parse --verify --quiet refs/tags/${FN_ARG_GIT_TAG}^{commit}"
      WORKING_DIRECTORY "${FN_ARG_SOURCE_DIR}"
      OUT_RESULT resolved_tag_commit
      OUT_RETURN_CODE resolve_return_code
    )
    if(resolve_return_code EQUAL 0)
      string(STRIP "${resolved_tag_commit}" resolved_tag_commit)
      if(resolved_tag_commit STREQUAL head_commit_id)
        set(${FN_ARG_OUT_STATE} "WARM" PARENT_SCOPE)
        return()
      endif()
    endif()

    set(${FN_ARG_OUT_STATE} "UPDATE" PARENT_SCOPE)
    return()

  endif()

  # URL content: trust only a completed populate (marker, or the legacy
  # sub-build stamp for caches populated before the marker existed)
  hfc_populate_marker_path("${FN_ARG_SOURCE_DIR}" marker_path)
  string(TOLOWER "${content_name}" content_name_lower)
  set(legacy_complete_stamp "${FN_ARG_LEGACY_SUBBUILD_DIR}/CMakeFiles/${content_name_lower}-populate-complete")

  if(EXISTS "${marker_path}" OR EXISTS "${legacy_complete_stamp}")
    set(${FN_ARG_OUT_STATE} "WARM" PARENT_SCOPE)
    return()
  endif()

  # non-empty extraction dir without a completed populate: half-extracted or
  # foreign content — re-populate from a clean slate
  set(${FN_ARG_OUT_STATE} "CORRUPT" PARENT_SCOPE)
endfunction()

#
# Update an existing git clone in place to the declared GIT_TAG: fetch (best
# effort — a stale origin only matters if the revision is missing locally)
# then force-clean checkout. This deliberately discards local modifications:
# a revision change is an explicit request for that revision's sources (warm
# HEAD==GIT_TAG clones never reach this path, so debugging edits survive).
function(hfc_populate_update_git_worktree content_name)
  set(one_value_params
    SOURCE_DIR
    GIT_TAG
    OUT_SUCCESS
  )
  cmake_parse_arguments(FN_ARG "" "${one_value_params}" "" ${ARGN})

  hfc_log(STATUS "🔁 Updating ${FN_ARG_SOURCE_DIR} to ${FN_ARG_GIT_TAG}")

  # pick up moved branches/tags when the origin is reachable; tolerate failure
  # (offline reconfigures keep working from the local clone when the revision
  # is already present)
  git_exec(COMMAND "${git_executable} fetch origin --tags"
    WORKING_DIRECTORY "${FN_ARG_SOURCE_DIR}"
    OUT_RETURN_CODE fetch_return_code
  )
  if(NOT fetch_return_code EQUAL 0)
    hfc_log(STATUS " - could not fetch origin for ${content_name} (offline?); using local state")
  endif()

  # when GIT_TAG is a branch, the fetched state lives in the remote-tracking
  # ref — a plain checkout of the local branch name would stay stale
  set(checkout_revision "${FN_ARG_GIT_TAG}")
  git_exec(COMMAND "${git_executable} rev-parse --verify --quiet origin/${FN_ARG_GIT_TAG}^{commit}"
    WORKING_DIRECTORY "${FN_ARG_SOURCE_DIR}"
    OUT_RESULT remote_tracking_commit
    OUT_RETURN_CODE remote_tracking_return_code
  )
  if(remote_tracking_return_code EQUAL 0)
    string(STRIP "${remote_tracking_commit}" remote_tracking_commit)
    set(checkout_revision "${remote_tracking_commit}")
  endif()

  checkout_revision_force_clean(
    REPOSITORY_DIR "${FN_ARG_SOURCE_DIR}"
    GIT_REVISION "${checkout_revision}"
    OUT_SUCCESS checkout_success
  )

  set(${FN_ARG_OUT_SUCCESS} ${checkout_success} PARENT_SCOPE)
endfunction()
