#!/bin/bash
# Android build harness for Open1560. Run from WSL:  bash android/build.sh
# Syncs the Windows-side repo into the WSL filesystem (10x faster I/O), then
# cross-compiles with the NDK and summarizes what failed.
set -u

NDK="${NDK:-$HOME/android/android-ndk-r27c}"
WIN_SRC="${WIN_SRC:-/mnt/c/Users/amir/Desktop/OPENMM/Open1560}"
WSL_SRC="${WSL_SRC:-$HOME/mm/Open1560}"
ABI="${ABI:-arm64-v8a}"
API="${API:-24}"

mkdir -p "$WSL_SRC"
rsync -a --delete \
    --exclude '.git' --exclude 'build' --exclude 'android/build' \
    "$WIN_SRC/" "$WSL_SRC/"

cd "$WSL_SRC/android" || exit 1
cmake -S . -B build \
    -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI="$ABI" \
    -DANDROID_PLATFORM="android-$API" \
    -DCMAKE_BUILD_TYPE=Release > build/cmake.log 2>&1 || \
cmake -S . -B build \
    -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI="$ABI" \
    -DANDROID_PLATFORM="android-$API" \
    -DCMAKE_BUILD_TYPE=Release

make -C build -k -j"$(nproc)" 2>&1 | tee build/build.log | grep -E "error:|undefined symbol" > build/errors.log

total_tu=$(find "$WSL_SRC/android/build/CMakeFiles/midtown.dir" -name '*.o' 2>/dev/null | wc -l)
echo
echo "=== objects built: $total_tu ==="
echo "=== error lines: $(wc -l < build/errors.log) ==="
echo "--- top failing files ---"
sed 's/:[0-9].*//' build/errors.log | sort | uniq -c | sort -rn | head -20
echo "--- top error kinds ---"
sed 's/.*error: //' build/errors.log | sed "s/'[^']*'/'X'/g" | sort | uniq -c | sort -rn | head -25
