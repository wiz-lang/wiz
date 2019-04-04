wiz
===

**by Andrew G. Crowell**

A high-level assembly language for writing homebrew software and games on retro console platforms.

Features
--------

Wiz has the following features:

- A syntax that should feel more approachable to users of higher-level languages, but avoiding most of the hidden runtime costs, while simultaneously providing direct control over the hardware and memory layout.
- Static type system supporting integers, structures, arrays, pointers, and other types.
- High-level structured programming features (if, while, for, do/while, etc)
- Low-level branching and operations where needed.
- Uniform expression syntax for both compile-time and run-time expressions.
- Familiar high-level syntax with expressions and statements that map directly to low-level instructions.
- Direct access to registers and memory via assignment syntax.
- Access to platform instrinsics for stack manipulation, comparison, bit-twiddling, I/O, etc.
- Compile-time inlining, constant expression folding, and conditional compilation.
- Function calls with register/temporary passing.
- Bank declarations to customize memory map and output.
- const/writeonly access modifiers to prevent undesired access to memory.

Wiz is intended to cross-compile programs that run on specific hardware, as opposed to an abstract machine which requires a runtime library to support it. This means programs must be written with the feature set and limitations of the system being targeted in mind (registers, addressing modes, limitations on instructions), and programs are highly platform-dependent.

Wiz currently supports the following CPU architectures:

- MOS 6502 (used by the NES, C64, Atari 2600/5200/7800/8-bit, Apple II)
- MOS 65C02
- WDC 65C02
- Rockwell 65C02
- HuC6280 (used by the PC Engine / TurboGrafx-16)
- WDC 65816 (used by the SNES, SA-1, Apple IIgs)
- SPC700 (used by the SNES APU)
- Zilog Z80 (used by the Sega Master System, Game Gear, MSX, ZX, etc)
- Game Boy CPU / DMG-CPU / GBZ80 / SM83 / LR35902 (used by the Game Boy and Game Boy Color)

Wiz currently supports exporting the following output formats:

- .bin
- .nes
- .gb
- .sms / .gg
- .sfc
- .smc
- .pce
- .a26
- In some of the above cases, the CPU architecture automatically detected by the output format.


The License
-----------

This code is released under an MIT license.

    Copyright (C) 2019 by Andrew G. Crowell

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.


Other Notes
-----------

- Source code and documentation for the new version is (hopefully) coming soon.
- For the old version of Wiz that was written in D, see the `release/wiz-d-legacy` branch of this repo.
- For the even older esoteric language project that turned into Wiz, see: http://github.com/Bananattack/nel-d
- Twitter: https://twitter.com/eggboycolor

TODO: improve this readme