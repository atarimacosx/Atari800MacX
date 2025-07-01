#!/bin/bash

# Test compilation script for new Atari800 files
# This verifies all new files compile properly before adding to Xcode

cd "$(dirname "$0")/atari800-MacOSX/src"

# Common compiler flags
CFLAGS="-I. -I./Atari800MacX -I./atari_ntsc"
DEFINES="-DNETSIO -DNTSC_FILTER -DPAL_BLENDING -DPOKEYREC -DVOICEBOX -DPLATFORM_MAP_PALETTE -DSUPPORTS_PLATFORM_PALETTEUPDATE -DSUPPORTS_CHANGE_VIDEOMODE"

echo "Testing compilation of new Atari800 files..."

# Test each new file
files=(
    "netsio.c"
    "artifact.c" 
    "filter_ntsc.c"
    "pal_blending.c"
    "colours_ntsc.c"
    "colours_pal.c"
    "colours_external.c"
    "videomode.c"
    "pokeyrec.c"
    "voicebox.c"
    "votraxsnd.c"
    "atari_ntsc/atari_ntsc.c"
)

failed=0
for file in "${files[@]}"; do
    echo -n "Compiling $file... "
    if gcc -c $CFLAGS $DEFINES "$file" -o "/tmp/$(basename "$file" .c).o" 2>/dev/null; then
        echo "✓"
    else
        echo "✗"
        echo "Error details:"
        gcc -c $CFLAGS $DEFINES "$file" -o "/tmp/$(basename "$file" .c).o"
        failed=$((failed + 1))
    fi
done

echo
if [ $failed -eq 0 ]; then
    echo "🎉 All files compile successfully!"
    echo "Required defines: $DEFINES"
else
    echo "❌ $failed files failed to compile"
    exit 1
fi