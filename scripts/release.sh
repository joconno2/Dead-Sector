#!/usr/bin/env bash
# Build Dead Sector release packages for Linux and Windows.
# Run from the project root: ./scripts/release.sh
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DIST="$ROOT/dist"
VERSION="${1:-dev}"

echo "=== Dead Sector release build (version: $VERSION) ==="
echo

# ---------------------------------------------------------------------------
# Linux release
# ---------------------------------------------------------------------------
echo "--- Building Linux (native) ---"
cmake -S "$ROOT" -B "$ROOT/build-linux" \
    -DCMAKE_BUILD_TYPE=Release \
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
    -DCMAKE_EXPORT_COMPILE_COMMANDS=OFF
cmake --build "$ROOT/build-windows" --parallel "$(nproc)"

WIN_DIR="$DIST/dead-sector-windows-$VERSION"
rm -rf "$WIN_DIR"
mkdir -p "$WIN_DIR"
cp "$ROOT/build-windows/dead-sector.exe" "$WIN_DIR/"
cp "$ROOT/build-windows/SDL2.dll"        "$WIN_DIR/"
cp "$ROOT/build-windows/SDL2_ttf.dll"    "$WIN_DIR/"
cp -r "$ROOT/build-windows/assets"       "$WIN_DIR/"
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
