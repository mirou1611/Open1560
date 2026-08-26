# Open1560 on Android

A native ARM64 port of Open1560. This directory holds everything Android-specific:
the cross-compile harness, the stub generator that stands in for code still living
in `game.asm`, and the APK packaging.

## State

| | |
|---|---|
| Compiles | 371 translation units, `arm64-v8a`, no errors |
| Links | `libmain.so`, 15 MB |
| Packages | `open1560.apk`, installs and launches |
| Still in x86 assembly | **764 symbols** — 614 functions, 150 globals — currently stubbed |

The stubs log every call, so the game runs until it needs something real. That log
is the worklist: whatever it asks for first is what to implement first.

## Layout

| Path | What it is |
|---|---|
| `CMakeLists.txt` | The build. Compiles the portable C++ modules, links `libmain.so` |
| `build.sh` | Syncs the repo into WSL and cross-compiles with the NDK |
| `gen_stubs.py` | Turns the linker's undefined-symbol list into `stubs/stubs.S` |
| `stubs/stubs.S` | Generated. Functions log and return 0; globals are zeroed blocks |
| `src/stub_runtime.cpp` | `ArtsStubCalled` — logs and counts stub hits |
| `worklist.sh` | Reports what is still missing, from the built static library |
| `app/` | AndroidManifest and the `SDLActivity` subclass |
| `package_apk.sh` | Builds and signs the APK with the SDK tools (no Gradle) |

## Building

Prerequisites, all already in place on this machine:

* Android NDK r27c at `~/android/android-ndk-r27c` (in WSL)
* SDL 3.4.4 built for `arm64-v8a` at `~/android/SDL3/build-android`
* freetype 2.13.3 built for `arm64-v8a` at `~/android/freetype-2.13.3/build-android`
* Android SDK + Android Studio's JBR on the Windows side

```sh
# native library (from Windows)
wsl.exe -d Ubuntu -- bash -lc "bash /mnt/c/Users/amir/Desktop/OPENMM/Open1560/android/build.sh"

# APK (from Git Bash)
bash android/package_apk.sh install
```

After a change that adds or removes calls into `game.asm`, regenerate the stubs:

```sh
wsl.exe -d Ubuntu -- bash -lc "python3 ~/mm/Open1560/android/gen_stubs.py \
    ~/mm/Open1560/android/build/build.log \
    --out /mnt/c/Users/amir/Desktop/OPENMM/Open1560/android/stubs/stubs.S"
```

## Game data

Open1560 needs the original game's archives — at minimum `audio.ar`, `core.ar` and
`ui.ar`. Push them to the app's external files directory, which the game chdir()s
into at startup:

```sh
adb push audio.ar core.ar ui.ar /sdcard/Android/data/com.open1560.app/files/
```

`commandline.txt` in the same directory takes the usual Open1560 arguments — the
only way to pass any, since an Android app gets no argv.

## What is not built yet

| Excluded | Why | Plan |
|---|---|---|
| `mmaudio` | DirectSound, EAX and MCI throughout | Reimplement over SDL3 audio |
| `agisw` | x86 software rasterizer | Not needed; GLES draws everything |
| `mmnetwork` | DirectPlay | Out of scope |
| `mmwidget`, `toolmgr`, `agirend`, `mmcamtour` | Dev-build debug UI | Not needed |
| DirectInput force feedback | Steering wheel effects | Not applicable |

The renderer in `agigl/` still targets desktop OpenGL through glad — immediate mode
paths, `GL_BGRA`, and desktop-only entry points. Porting it to GLES 3 is the next
piece of platform work after the stub log shows the engine getting that far.
