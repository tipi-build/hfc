#!/usr/bin/env bash
# Generate a tipi distro.json that provisions CMake 4.x instead of the distro
# default, for running the HFC test suite against CMake 4.
#
# tipi's distro system installs tools from a distro.json manifest; the
# TIPI_DISTRO_JSON environment variable overrides which manifest is used (a
# local file path is accepted as-is, no SHA required). This script:
#   1. resolves the latest stable CMake 4.x release (or uses the version
#      passed as $2)
#   2. downloads the Kitware linux-x86_64 tarball and repacks it as a .zip
#      (the distro ship step only extracts zip archives)
#   3. patches the cmake entry of the base distro.json to a file:// URL of
#      that zip
#   4. prints the generated manifest path — export it as TIPI_DISTRO_JSON so
#      every subsequent `tipi run ...` provisions and uses that CMake
#
# Usage:
#   test/generate_cmake4_distro.sh <output-dir> [<cmake-version e.g. 4.4.0>]
#   export TIPI_DISTRO_JSON=<output-dir>/distro-cmake4.json
set -euo pipefail

OUT_DIR="${1:?usage: generate_cmake4_distro.sh <output-dir> [<cmake-version>]}"
REQUESTED_VERSION="${2:-}"
BASE_DISTRO_JSON="${TIPI_BASE_DISTRO_JSON:-/usr/local/share/.tipi/distro.json}"

if [ ! -f "$BASE_DISTRO_JSON" ]; then
  echo "error: base distro.json not found at $BASE_DISTRO_JSON" >&2
  echo "       (run any 'tipi' command once to materialize it, or set TIPI_BASE_DISTRO_JSON)" >&2
  exit 1
fi

mkdir -p "$OUT_DIR"
OUT_DIR="$(cd "$OUT_DIR" && pwd)"
cd "$OUT_DIR"

if [ -z "$REQUESTED_VERSION" ]; then
  REQUESTED_VERSION="$(curl -fsSL "https://api.github.com/repos/Kitware/CMake/releases?per_page=30" \
    | python3 -c 'import json,sys; rels=json.load(sys.stdin); print(next(r["tag_name"][1:] for r in rels if not r["prerelease"] and r["tag_name"].startswith("v4.")))')"
fi
echo "Using CMake version: $REQUESTED_VERSION" >&2

ARCHIVE_ROOT="cmake-${REQUESTED_VERSION}-linux-x86_64"
if [ ! -f "${ARCHIVE_ROOT}.zip" ]; then
  curl -fsSLo "${ARCHIVE_ROOT}.tar.gz" \
    "https://github.com/Kitware/CMake/releases/download/v${REQUESTED_VERSION}/${ARCHIVE_ROOT}.tar.gz"
  rm -rf "${ARCHIVE_ROOT}"
  tar xzf "${ARCHIVE_ROOT}.tar.gz"
  zip -qr "${ARCHIVE_ROOT}.zip" "${ARCHIVE_ROOT}"
fi

SHA1="$(sha1sum "${ARCHIVE_ROOT}.zip" | cut -d' ' -f1)"
MODULE_PATH="$(cd "${ARCHIVE_ROOT}/share" && ls -d cmake-*)/Modules"
MODULE_PATH="share/${MODULE_PATH}"

python3 - "$BASE_DISTRO_JSON" "$OUT_DIR" "$ARCHIVE_ROOT" "$SHA1" "$MODULE_PATH" <<'EOF'
import json, sys
base_json, out_dir, archive_root, sha1, module_path = sys.argv[1:6]
d = json.load(open(base_json))
tools = d["distro"] if isinstance(d, dict) else d
for tool in tools:
    if tool.get("name") == "cmake":
        tool["platforms"]["linux"]["x86_64"] = {
            "url": f"file://{out_dir}/{archive_root}.zip",
            "sha1": sha1,
            "root": archive_root,
            "module_path": module_path,
            "path": ["bin"],
        }
        break
else:
    raise SystemExit("error: no 'cmake' tool entry found in base distro.json")
with open(f"{out_dir}/distro-cmake4.json", "w") as f:
    json.dump(d, f, indent=1)
EOF

echo "${OUT_DIR}/distro-cmake4.json"
