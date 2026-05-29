# Commit & PR description

## Commit message

```
HermeticFetchContent v1.0.24 : Fix find_package provider to properly mark dependencies as found

The dependency provider only set ${PACKAGE_NAME}_FOUND (uppercase).
CMake's dependency provider contract requires setting ${depName}_FOUND
(exact-case). Without it, CMake assumes the request was not fulfilled
and falls back to built-in mechanisms.

Applying the correct _FOUND fix (from Orphis in hfc#38) exposed cascading
failures in Apache Arrow's ThirdpartyToolchain.cmake:

1. string(REPLACE) on empty ${Thrift_VERSION} (line 1792)
2. if(${xsimd_VERSION} VERSION_LESS "8.1.0") with empty version (line 312)
3. Missing Boost::disable_autolinking / Boost::dynamic_linking targets

Root causes:
- No version tracking: provider never exposed _VERSION variables
- include() of ConfigVersion.cmake exits calling function (return() semantics)
- ConfigVersion.cmake appends platform suffixes ("13.0.0 (64bit)")
- Macro variable leakage between successive find_package() calls
- Boost utility targets live in BoostConfig.cmake logic, not export files

CHANGELOG

- Provider sets both ${pkg}_FOUND and ${PKG}_FOUND per CMake provider contract
- Auto-detect package version from ConfigVersion.cmake and pkg-config .pc files
- Add HERMETIC_VERSION parameter to FetchContent_MakeHermetic() (takes priority over auto-detection)
- Forward detected version through proxy toolchain to sub-builds
- Expose _VERSION, _LIB, _STATIC_LIB, _LIBRARY, _LIBRARIES, _INCLUDE_DIR(S) variables
- Fix macro variable leakage between successive find_package() calls
- Cache imported libraries list for idempotent target cache consumption
- Arrow example: add HERMETIC_VERSION for Thrift, add Boost utility target exports

Co-Authored-By: Claude Opus 4.6 <noreply@anthropic.com>
```

---

## PR description

### HermeticFetchContent v1.0.24 : Fix find_package provider to properly mark dependencies as found

Closes #463

## Context

The HFC dependency provider does not conform to CMake's dependency provider contract. From the [CMake documentation](https://cmake.org/cmake/help/git-master/command/cmake_language.html#dependency-providers):

> If the provider command fulfills the request, it must set the same variable that find_package() expects to be set. For a dependency named depName, the provider must set depName_FOUND to true if it fulfilled the request. If the provider returns without setting this variable, CMake will assume the request was not fulfilled and will fall back to the built-in implementation.

HFC only set `${PACKAGE_NAME}_FOUND` (uppercase), not the exact-case `${PackageName}_FOUND`. This worked historically because HFC also sets `_INCLUDE_DIR` and `_LIBRARY` variables that serve as hints to CMake's built-in Find modules, effectively getting the right result through a side door. But this is an escape hatch from hermetic isolation, not proper provider behavior.

@Orphis fixed this in hfc#38 by adding:
```cmake
set(${package_name}_FOUND ${package_found})
set(${package_name_uppercase}_FOUND ${package_found})
```

This fix is correct per the CMake spec, but it broke Apache Arrow (and the LSEG canary build) because Arrow's `ThirdpartyToolchain.cmake` now sees packages as properly found for the first time and tries to use them directly — hitting code paths that were never exercised before.

## Problem: cascade of failures in Arrow

Once `_FOUND` is correctly set, Arrow's `ThirdpartyToolchain.cmake` fails at two points:

**1. `Thrift_VERSION` is empty (line 1792):**
```
CMake Error at cmake_modules/ThirdpartyToolchain.cmake:1792 (string):
  string sub-command REPLACE requires at least four arguments.
```
Arrow does `string(REPLACE "." ";" VERSION_LIST ${Thrift_VERSION})` — crashes on empty string.

**2. `xsimd_VERSION` is empty (line 312):**
```
CMake Error at cmake_modules/ThirdpartyToolchain.cmake:312 (if):
  if given arguments:
    "VERSION_LESS" "8.1.0"
  Unknown arguments specified
```
Arrow's `resolve_dependency` does `if(${${PACKAGE_NAME}_VERSION} VERSION_LESS ${ARG_REQUIRED_VERSION})` — when `xsimd_VERSION` is empty, the `if()` receives `VERSION_LESS "8.1.0"` with no left-hand operand.

**3. Missing Boost utility targets:**
```
CMake Error: IMPORTED_LINK_DEPENDENT_LIBRARIES ... Boost::disable_autolinking
```
With `Boost_FOUND` now set, Arrow uses HFC-provided Boost targets which reference `Boost::disable_autolinking` and `Boost::dynamic_linking` in `INTERFACE_LINK_LIBRARIES`. These are created at runtime by `BoostConfig.cmake` logic (not in export files), so HFC's target scanner never captured them.

## Root cause analysis

| Symptom | Root cause |
|---------|-----------|
| `_VERSION` never set | Provider had no version tracking infrastructure |
| Version detection via `include()` silently aborts | `ConfigVersion.cmake` contains `return()` which exits the *calling function* per CMake `include()` semantics |
| Detected version is `"13.0.0 (64bit)"` (unparseable) | `xsimdConfigVersion.cmake` appends suffix when `CMAKE_SIZEOF_VOID_P` is defined at include time |
| `xsimd_VERSION` contains Thrift's version `"0.23.0"` | Provider is a CMake **macro** (no scope). Variables from previous `find_package(Thrift)` leak into subsequent `find_package(xsimd)` |
| Version detected during consume but not visible to provider | First-time detection happens inside `hfc_targets_cache_consume()` but provider reads version *before* calling consume |
| Missing `Boost::disable_autolinking` | Target created by `BoostConfig.cmake` logic, not present in cmake export files that HFC scans |

## Solution

| Issue | Fix |
|-------|-----|
| Missing exact-case `_FOUND` | Set both `${pkg}_FOUND` and `${PKG}_FOUND` (re-apply Orphis fix from hfc#38) |
| No version infrastructure | Add `VERSION` param to `hfc_targets_cache_register_dependency_for_provider` and `OUT_VERSION` to `get_registered_info` |
| `include()` + `return()` aborts function | Use `file(READ)` + `string(REGEX MATCH)` to parse `ConfigVersion.cmake` as text |
| Platform suffix in version string | Regex extracts only numeric prefix: `"([0-9]+([.][0-9]+)*)"` |
| Macro variable leakage | Reset `targetcache_version` to `""` before each `get_registered_info()` call |
| Version not available on first consume | Re-read `HERMETIC_FETCHCONTENT_${pkg}_VERSION` from cache after `hfc_targets_cache_consume()` returns |
| Version not forwarded to sub-builds | Forward `VERSION` in proxy toolchain registration |
| No version for packages without metadata (Thrift) | `HERMETIC_VERSION` parameter in `FetchContent_MakeHermetic()` — takes priority over auto-detection |
| Missing Boost utility targets | `HERMETIC_CMAKE_ADDITIONAL_EXPORTS` to declare `Boost::disable_autolinking` and `Boost::dynamic_linking` |
| `OUT_IMPORTED_LIBRARIES` empty on re-consume | Cache imported libraries list in `CACHE INTERNAL` for idempotent returns |

## New API: `HERMETIC_VERSION`

```cmake
FetchContent_MakeHermetic(
  Thrift
  HERMETIC_VERSION "0.23.0"  # takes priority over auto-detection
  HERMETIC_FIND_PACKAGES "OpenSSL;ZLIB;Boost"
  ...
)
```

When specified, `HERMETIC_VERSION` takes priority over auto-detected version. Use it for packages where auto-detection produces incorrect results, or for packages that don't install version metadata at all (no `*ConfigVersion.cmake`, no `.pc` file).

## Version auto-detection strategy

At target cache consume time, HFC now:
1. Globs for `*ConfigVersion.cmake` / `*-config-version.cmake` in `<install_prefix>/lib/cmake/<pkg>/` (both exact-case and lowercase)
2. Reads the file as text (avoids `include()` + `return()` trap)
3. Extracts version with `string(REGEX MATCH "set\\(PACKAGE_VERSION \"([0-9]+([.][0-9]+)*)\"" ...)`
4. Falls back to parsing `Version:` from pkg-config `.pc` files in `<install_prefix>/lib/pkgconfig/`
5. User-specified `HERMETIC_VERSION` always takes priority when provided

## Files changed

- `cmake/modules/hfc_provide_dependency_FINDPACKAGE.cmake` — Provider macro: version tracking, legacy variables, dual-casing `_FOUND`, variable reset
- `cmake/modules/hfc_targets_cache_consume.cmake` — Version auto-detection, imported libraries caching
- `cmake/modules/hfc_targets_cache_common.cmake` — `VERSION` / `OUT_VERSION` parameters, fix docstring
- `cmake/modules/hfc_generate_cmake_proxy_toolchain.cmake` — Forward `VERSION` through proxy toolchain
- `cmake/modules/hfc_make_available_single.cmake` — `HERMETIC_VERSION` parameter parsing and priority logic
- `example/arrow/thirdparty/build_thirdparty.cmake` — Thrift `HERMETIC_VERSION`, Boost `HERMETIC_CMAKE_ADDITIONAL_EXPORTS`

## Test plan

- [ ] Arrow example builds end-to-end with `ARROW_DEPENDENCY_SOURCE=SYSTEM` finding all HFC-provided deps
- [ ] Verify `Thrift_VERSION`, `xsimd_VERSION`, `Boost_FOUND` are correctly set inside Arrow's sub-build
- [ ] Verify packages with `ConfigVersion.cmake` (xsimd, Boost, lz4) auto-detect version without needing `HERMETIC_VERSION`
- [ ] Verify `HERMETIC_VERSION` takes priority over auto-detected version when both are available
- [ ] Existing examples (boost, grpc, openssl, get-started) still pass CI
- [ ] LSEG canary build passes with this fix applied
