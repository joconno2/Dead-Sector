#!/usr/bin/env bash
# Build Dead Sector release packages and upload to Steam.
# Usage: ./scripts/release.sh <version> <steam_account>
# Example: ./scripts/release.sh v0.3.14 blademaster313
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DIST="$ROOT/dist"
STEAM_ACCOUNT="${1:-}"
STEAMCMD="$ROOT/deps/steamworks/tools/ContentBuilder/builder_linux/steamcmd.sh"
STEAM_SCRIPT="$ROOT/steampipe/scripts/app_build_4532540.vdf"

# Auto-detect version from git tag, fall back to CMakeLists.txt
VERSION="$(git -C "$ROOT" describe --tags --exact-match 2>/dev/null \
    || grep -m1 'project(DeadSector VERSION' "$ROOT/CMakeLists.txt" \
       | grep -oP '[\d]+\.[\d]+\.[\d]+')"
VERSION="${VERSION:-dev}"

echo "=== Dead Sector release build (version: $VERSION) ==="
echo

# ---------------------------------------------------------------------------
# Linux release
# ---------------------------------------------------------------------------
echo "--- Building Linux (native) ---"
cmake -S "$ROOT" -B "$ROOT/build-linux" \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_STEAM=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=OFF
cmake --build "$ROOT/build-linux" --parallel "$(nproc)"

LINUX_DIR="$DIST/dead-sector-linux-$VERSION"
rm -rf "$LINUX_DIR"
mkdir -p "$LINUX_DIR"
cp "$ROOT/build-linux/dead-sector"    "$LINUX_DIR/"
cp -r "$ROOT/build-linux/assets"      "$LINUX_DIR/"
echo "Linux build: $LINUX_DIR"

# ---------------------------------------------------------------------------
# Windows release (cross-compile with MinGW)
# ---------------------------------------------------------------------------
echo
echo "--- Building Windows (MinGW cross-compile) ---"
cmake -S "$ROOT" -B "$ROOT/build-windows" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/toolchain-mingw64.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_STEAM=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=OFF
cmake --build "$ROOT/build-windows" --parallel "$(nproc)"

WIN_DIR="$DIST/dead-sector-windows-$VERSION"
rm -rf "$WIN_DIR"
mkdir -p "$WIN_DIR"
cp "$ROOT/build-windows/dead-sector.exe" "$WIN_DIR/"
cp -r "$ROOT/build-windows/assets"       "$WIN_DIR/"
cp "$ROOT/build-windows/"*.dll           "$WIN_DIR/" 2>/dev/null || true
echo "Windows build: $WIN_DIR"

# ---------------------------------------------------------------------------
# Zip archives
# ---------------------------------------------------------------------------
echo
echo "--- Creating archives ---"
cd "$DIST"
zip -r "dead-sector-linux-$VERSION.zip"   "dead-sector-linux-$VERSION/"
zip -r "dead-sector-windows-$VERSION.zip" "dead-sector-windows-$VERSION/"

echo
echo "=== Done ==="
echo "  $DIST/dead-sector-linux-$VERSION.zip"
echo "  $DIST/dead-sector-windows-$VERSION.zip"

# ---------------------------------------------------------------------------
# Steam upload
# ---------------------------------------------------------------------------
if [ -z "$STEAM_ACCOUNT" ]; then
    echo
    echo "--- Skipping Steam upload (no account provided) ---"
    echo "    To upload: ./scripts/release.sh <steam_account>"
    exit 0
fi

echo
echo "--- Uploading to Steam as $STEAM_ACCOUNT ---"
bash "$STEAMCMD" +login "$STEAM_ACCOUNT" +run_app_build "$STEAM_SCRIPT" +quit

echo
echo "=== Steam upload complete ==="
echo "    Set build live at: https://partner.steamgames.com/apps/builds/4532540"
