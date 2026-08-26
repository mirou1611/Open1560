#!/bin/bash
# Builds android/app into an installable debug APK.
#
# Runs on the Windows side (Git Bash), because that is where the Android SDK and
# a JDK live; the native libraries come from the WSL build tree. No Gradle - the
# SDK's own aapt2/d8/apksigner do the whole job.
#
#   bash android/package_apk.sh          # build the APK
#   bash android/package_apk.sh install  # ...and adb install it
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SDK="${ANDROID_SDK:-$HOME/AppData/Local/Android/Sdk}"
BUILD_TOOLS="${BUILD_TOOLS:-$SDK/build-tools/37.0.0}"
PLATFORM_JAR="${PLATFORM_JAR:-$SDK/platforms/android-36/android.jar}"
JAVA_HOME="${JAVA_HOME:-/c/Program Files/Android/Android Studio/jbr}"

# Where the WSL build tree and its dependencies live, as seen from Windows.
WSL_ROOT="${WSL_ROOT:-//wsl.localhost/Ubuntu/home/mirou}"
SDL_SOURCE="${SDL_SOURCE:-$WSL_ROOT/android/SDL3}"

OUT="$HERE/build-apk"
ABI="${ABI:-arm64-v8a}"
PACKAGE=com.open1560.app

# Each ABI has its own native build and dependency directories.
if [ "$ABI" = "x86_64" ]; then
    NATIVE_BUILD="${NATIVE_BUILD:-$WSL_ROOT/mm/Open1560/android/build-x64}"
    SDL_BUILD=build-android-x64
else
    NATIVE_BUILD="${NATIVE_BUILD:-$WSL_ROOT/mm/Open1560/android/build}"
    SDL_BUILD=build-android
fi

# The SDK tools are Windows executables and need Windows paths, not the /c/...
# form Git Bash hands out.
winpath() {
    printf '%s' "$1" | sed 's|^/\([a-zA-Z]\)/|\1:/|'
}

rm -rf "$OUT"
mkdir -p "$OUT/classes" "$OUT/apk/lib/$ABI" "$OUT/java"

echo "=== collecting sources ==="
cp -r "$SDL_SOURCE/android-project/app/src/main/java/org" "$OUT/java/"
cp -r "$HERE/app/java/com" "$OUT/java/"

echo "=== javac ==="
find "$OUT/java" -name '*.java' | while read -r f; do winpath "$f"; echo; done > "$OUT/sources.txt"
"$JAVA_HOME/bin/javac" -source 17 -target 17 -nowarn \
    -classpath "$(winpath "$PLATFORM_JAR")" \
    -d "$(winpath "$OUT/classes")" \
    "@$(winpath "$OUT/sources.txt")" 2>&1 | grep -v '^Note:' || true

echo "=== d8 ==="
find "$OUT/classes" -name '*.class' | while read -r f; do winpath "$f"; echo; done > "$OUT/classes.txt"
JAVA_HOME="$(winpath "$JAVA_HOME")" "$BUILD_TOOLS/d8.bat" --min-api 24 \
    --output "$(winpath "$OUT/apk")" \
    --lib "$(winpath "$PLATFORM_JAR")" \
    "@$(winpath "$OUT/classes.txt")"

echo "=== native libraries ==="
cp "$NATIVE_BUILD/libmain.so" "$OUT/apk/lib/$ABI/"
cp "$SDL_SOURCE/$SDL_BUILD/libSDL3.so" "$OUT/apk/lib/$ABI/"
ls -la "$OUT/apk/lib/$ABI/"

echo "=== aapt2 link ==="
"$BUILD_TOOLS/aapt2.exe" link \
    -I "$(winpath "$PLATFORM_JAR")" \
    --manifest "$(winpath "$HERE/app/AndroidManifest.xml")" \
    --min-sdk-version 24 \
    --target-sdk-version 34 \
    -o "$(winpath "$OUT/base.apk")" \
    --auto-add-overlay

echo "=== packaging ==="
cd "$OUT/apk"
"$JAVA_HOME/bin/jar" --update --file "$(winpath "$OUT/base.apk")" \
    classes.dex "lib/$ABI/libmain.so" "lib/$ABI/libSDL3.so"
cd "$HERE"

echo "=== signing ==="
# Lives outside $OUT, which is wiped each build - a new key every time would
# make every install fail with INSTALL_FAILED_UPDATE_INCOMPATIBLE.
KEYSTORE="$HERE/debug.keystore"

if [ ! -f "$KEYSTORE" ]; then
    "$JAVA_HOME/bin/keytool" -genkeypair -keystore "$(winpath "$KEYSTORE")" \
        -storepass android -keypass android \
        -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 \
        -dname "CN=Android Debug,O=Android,C=US" >/dev/null 2>&1
fi

"$BUILD_TOOLS/zipalign.exe" -f -p 4 "$(winpath "$OUT/base.apk")" "$(winpath "$OUT/open1560-aligned.apk")"
JAVA_HOME="$(winpath "$JAVA_HOME")" "$BUILD_TOOLS/apksigner.bat" sign \
    --ks "$(winpath "$KEYSTORE")" --ks-pass pass:android --key-pass pass:android \
    --out "$(winpath "$HERE/open1560.apk")" "$(winpath "$OUT/open1560-aligned.apk")"

echo
ls -la "$HERE/open1560.apk"

if [ "${1:-}" = "install" ]; then
    "$SDK/platform-tools/adb.exe" install -r "$(winpath "$HERE/open1560.apk")"
    echo
    echo "Game data goes in /sdcard/Android/data/$PACKAGE/files/ - at minimum"
    echo "audio.ar, core.ar and ui.ar from a Midtown Madness installation."
fi
