# Open1560 - Android port build harness
#
# This is a compile/link harness, not (yet) a working game build. Its purpose is
# to turn "what still depends on x86" into a precise, shrinking worklist:
#   * compile errors  -> Win32/x86-isms that need portable replacements
#   * undefined symbols -> functions still living in game.asm
#
# Usage: android/build.sh (runs inside WSL with the NDK toolchain)

cmake_minimum_required(VERSION 3.22)
project(open1560 CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(MIDTOWN ${ROOT}/code/midtown)
set(VENDOR ${ROOT}/vendor)

# Modules compiled for Android.
# Excluded on purpose:
#   agisw      - software rasterizer (x86 asm heavy, 345 fns left, GLES replaces it)
#   mmnetwork  - DirectPlay multiplayer
#   mmwidget   - debug widget UI (dev build only)
#   toolmgr    - dev tooling
#   mmcamtour  - dev camera tours
#   agirend    - dev build only
#   mmaudio    - DirectSound/EAX/MCI throughout; to be replaced by an SDL3 audio backend
set(MODULES
    core vector7 memory stream data7 dyna7 eventq7 localize
    arts7 agi agigl agisdl agiworld
    mmdjpeg mmphysics mmdyna mmanim mmbangers mmeffects
    mmcar mmcity mmcityinfo mmai mmcamcs mminput mmui mmgame
    pcwindis
)

# Individual translation units that are Windows-only and have no place in the
# Android build (their functionality is replaced elsewhere, or is dev tooling).
set(EXCLUDED_SOURCES
    agisdl/sdlswpipe.cpp        # software-rasterizer present path (agisw excluded)
    memory/stack.cpp            # DbgHelp stack walking / .MAP symbol lookup
    mmeffects/mmtext_win32.cpp  # GDI text rendering (freetype path is used instead)
    data7/ipc.cpp               # Win32 named-pipe IPC for the debug tools
    agiworld/mrkni.cpp          # SSE variant of agiMeshSet::ToScreen (ARTS_ENABLE_KNI only)

    # DirectInput force feedback (steering wheels) - no equivalent on Android
    mminput/effect.cpp
    mminput/joystick.cpp
    mminput/joyman.cpp
    mminput/collide.cpp
    mminput/friction.cpp
    mminput/road.cpp
    mminput/spring.cpp
    pcwindis/sdldinput.cpp      # SDL-backed DirectInput device shim
    pcwindis/dxinit.cpp         # DirectDraw/Direct3D device enumeration
)

set(SOURCES)
foreach(mod ${MODULES})
    file(GLOB mod_srcs ${MIDTOWN}/${mod}/*.cpp)
    list(APPEND SOURCES ${mod_srcs})
endforeach()

foreach(excluded ${EXCLUDED_SOURCES})
    list(REMOVE_ITEM SOURCES ${MIDTOWN}/${excluded})
endforeach()

file(GLOB VENDOR_C ${VENDOR}/miniz/*.c ${VENDOR}/glad/src/*.c)

add_library(midtown STATIC ${SOURCES})

target_include_directories(midtown PUBLIC
    ${MIDTOWN}
    ${VENDOR}/hedley
    ${VENDOR}/mem
    ${VENDOR}/glad/include
    ${VENDOR}/miniz
    ${VENDOR}/freetype-2.10.4/include
    ${VENDOR}/SDL3/include
    ${VENDOR}/dx6/include
    ${VENDOR}/sse2neon
    ${CMAKE_CURRENT_SOURCE_DIR}/compat
)

target_compile_definitions(midtown PUBLIC
    ARTS_TARGET_BUILD=1560
    ARTS_STANDALONE
    ARTS_ANDROID=1
)

target_compile_options(midtown PRIVATE
    -include arts_pch.h
    -fno-exceptions
    -ffast-math
    -Wall
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-unused-private-field
    -Wno-missing-field-initializers
    -Wno-invalid-offsetof
    -Wno-ignored-attributes
    -Wno-microsoft-cast
)
