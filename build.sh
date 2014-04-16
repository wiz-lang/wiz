#!/usr/bin/env bash
set -e

# Minify the JS version of the compiler.
#cat %files% | jsmin > src/js/editor/dependencies/wiz.min.js
cat src/js/wiz/*.js src/js/wiz/ast/*.js src/js/wiz/cpu/*.js src/js/wiz/cpu/gameboy/*.js src/js/wiz/cpu/mos6502/*.js src/js/wiz/compile/*.js src/js/wiz/fs/*.js src/js/wiz/parse/*.js src/js/wiz/sym/*.js > src/js/editor/dependencies/wiz.min.js

# Build the compiler.
dmd src/d/wiz/*.d src/d/wiz/ast/*.d src/d/wiz/cpu/*.d src/d/wiz/cpu/gameboy/*.d src/d/wiz/cpu/mos6502/*.d src/d/wiz/sym/*.d src/d/wiz/parse/*.d src/d/wiz/compile/*.d -Isrc/d -ofwiz -g -debug

# Compile a test program.
./wiz examples/gameboy/snake/snake.wiz -gb -o examples/gameboy/snake/snake.gb
echo
# Compile another test program.
./wiz examples/nes/hello/hello.wiz -6502 -o examples/nes/hello/hello.nes
echo
# Compile another test program.
./wiz examples/nes/scroller/scroller.wiz -6502 -o examples/nes/scroller/scroller.nes
echo
# Compile another test program.
./wiz examples/nes/parallax/main.wiz -6502 -o examples/nes/parallax/parallax.nes
echo
# Compile another test program.
./wiz examples/gameboy/selfpromo/main.wiz -gb -o examples/gameboy/selfpromo/selfpromo.gb
echo
# Compile another test program.
./wiz examples/gameboy/shmup/main.wiz -gb -o examples/gameboy/shmup/shmup.gb
echo
# Compile a SNES driver program.
./wiz examples/gameboy/supaboy/snes_main.wiz -6502 -o examples/gameboy/supaboy/snes_main.6502
echo
# Compile another test program, depends on the SNES driver
./wiz examples/gameboy/supaboy/main.wiz -gb -o examples/gameboy/supaboy/supaboy.gb
echo
# Success!
if [ -f build2.sh ]; then
    ./build2.sh
else
    exit 0
fi