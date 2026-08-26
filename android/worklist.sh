#!/bin/bash
# Reports what the Android build still needs from game.asm.
#
# Every symbol referenced by the compiled objects but defined nowhere in them is
# either a function still living in the x86 assembly, or something the platform
# libraries provide (libc, libc++, SDL, GL). The first group is the porting
# worklist; this script separates them and groups the result by module.
set -u

NDK="${NDK:-$HOME/android/android-ndk-r27c}"
BUILD="${BUILD:-$HOME/mm/Open1560/android/build}"
SRC="${SRC:-$HOME/mm/Open1560/code/midtown}"
OUT="${OUT:-$BUILD/worklist}"

NM="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-nm"
LIB="$BUILD/libmidtown.a"

mkdir -p "$OUT"

"$NM" --defined-only "$LIB" 2>/dev/null | awk '{print $NF}' | sort -u > "$OUT/defined.txt"
"$NM" --undefined-only "$LIB" 2>/dev/null | awk '{print $NF}' | sort -u > "$OUT/undefined.txt"
comm -23 "$OUT/undefined.txt" "$OUT/defined.txt" > "$OUT/unresolved.txt"

# Split off what the platform provides from what the game owes us.
grep -E '^_Z' "$OUT/unresolved.txt" > "$OUT/unresolved_cxx.txt" || true
grep -vE '^_Z' "$OUT/unresolved.txt" > "$OUT/unresolved_c.txt" || true

grep -vE '^_Z(St|NSt|N9__gnu_cxx|N10__cxxabiv)' "$OUT/unresolved_cxx.txt" > "$OUT/game_symbols.txt" || true

echo "=== unresolved: $(wc -l < "$OUT/unresolved.txt") ==="
echo "  C++ mangled : $(wc -l < "$OUT/unresolved_cxx.txt")"
echo "  C / other   : $(wc -l < "$OUT/unresolved_c.txt")"
echo "  game symbols: $(wc -l < "$OUT/game_symbols.txt")"
echo
echo "--- C / other (platform libraries + a few game symbols) ---"
head -40 "$OUT/unresolved_c.txt"
