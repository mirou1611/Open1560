#!/bin/bash
# Android build harness for Open1560. Run from WSL:  bash android/build.sh
# Syncs the Windows-side repo into the WSL filesystem (10x faster I/O), then
# cross-compiles with the NDK and summarizes what failed.
#
#   ABI=arm64-v8a bash android/build.sh   # the shipping target (default)
#   ABI=x86_64    bash android/build.sh   # runs natively on the emulator, so
#                                         # crashes give real backtraces
set -u

NDK="${NDK:-$HOME/android/android-ndk-r27c}"
WIN_SRC="${WIN_SRC:-/mnt/c/Users/amir/Desktop/OPENMM/Open1560}"
WSL_SRC="${WSL_SRC:-$HOME/mm/Open1560}"
ABI="${ABI:-arm64-v8a}"
API="${API:-24}"

# Both ABIs keep their own build directory so they can coexist.
if [ "$ABI" = "x86_64" ]; then
    BUILD_DIR=build-x64
else
    BUILD_DIR=build
fi

mkdir -p "$WSL_SRC"
rsync -a --delete \
    --exclude '.git' --exclude 'build' --exclude 'android/build' --exclude 'android/build-x64' \
    "$WIN_SRC/" "$WSL_SRC/"

# rsync preserves the Windows mtime, so a stubs.S regenerated in the same minute as
# the last build can look older than its object file and get silently skipped.
touch "$WSL_SRC/android/stubs/stubs.S"

cd "$WSL_SRC/android" || exit 1
mkdir -p "$BUILD_DIR"

cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI="$ABI" \
    -DANDROID_PLATFORM="android-$API" \
    -DCMAKE_BUILD_TYPE=Release > "$BUILD_DIR/cmake.log" 2>&1 || {
    echo "=== cmake failed ==="
    tail -20 "$BUILD_DIR/cmake.log"
    exit 1
}

make -C "$BUILD_DIR" -k -j"$(nproc)" 2>&1 \
    | tee "$BUILD_DIR/build.log" \
    | grep -E "error:|undefined symbol" > "$BUILD_DIR/errors.log"

total_tu=$(find "$WSL_SRC/android/$BUILD_DIR/CMakeFiles/midtown.dir" -name '*.o' 2>/dev/null | wc -l)
echo
echo "=== abi: $ABI  objects built: $total_tu ==="
echo "=== error lines: $(wc -l < "$BUILD_DIR/errors.log") ==="
echo "--- top failing files ---"
sed 's/:[0-9].*//' "$BUILD_DIR/errors.log" | sort | uniq -c | sort -rn | head -20
echo "--- top error kinds ---"
sed 's/.*error: //' "$BUILD_DIR/errors.log" | sed "s/'[^']*'/'X'/g" | sort | uniq -c | sort -rn | head -25
