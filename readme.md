wiz
===

**by Andrew G. Crowell**

Wiz is a high-level assembly language for writing homebrew software for retro console platforms.

Contact
-------

- Please report issues in the GitHub issue tracker.
- Project Website: http://wiz-lang.org/
- Project Discord: https://discord.gg/BKnTg7N
- Project GitHub: https://github.com/wiz-lang/wiz
- My Twitter: https://twitter.com/eggboycolor
- My Email: eggboycolor AT gmail DOT com
- My Github: http://github.com/Bananattack

Features
--------

Wiz is intended to cross-compile programs that run on specific hardware, as opposed to an abstract machine which requires a runtime library to support it. This means programs must be written with the feature set and limitations of the system being targeted in mind (registers, addressing modes, limitations on instructions), and programs are highly platform-dependent.

Here are some features that Wiz supports:

- Familiar high-level syntax and structured programming features, such as `if`, `while`, `for`, `do`/`while`, functions, etc.
- Static type system that supports integers, structures, arrays, pointers, and other types. Supports automatic type inference for initializers.
- Expressions and statements that map directly to low-level instructions.
- Raw access to the hardware. Low-level register and memory access, branching, stack manipulation, comparison, bit-twiddling, I/O, etc.
- Compile-time attributes, inlining, constant expression folding, conditional compilation, array comprehensions.
- Support for multiple different systems and output file formats.
- `bank` declarations to customize memory map and output.
- `const` and `writeonly` modifiers to prevent undesired access to memory.

Wiz supports several different CPU architectures, with hopes to support more platforms in the future!

- `6502` - MOS 6502 (used by the NES, C64, Atari 2600/5200/7800/8-bit, Apple II)
- `65c02` - MOS 65C02
- `wdc65c02` - WDC 65C02
- `rockwell65c02` - Rockwell 65C02
- `huc6280` - HuC6280 (used by the PC Engine / TurboGrafx-16)
- `wdc65816` - WDC 65816 (used by the SNES, SA-1, Apple IIgs)
- `spc700` - SPC700 (used by the SNES APU)
- `z80` - Zilog Z80 (used by the Sega Master System, Game Gear, MSX, ZX, etc)
- `gb` - Game Boy CPU / DMG-CPU / GBZ80 / SM83 / LR35902 (used by the Game Boy and Game Boy Color)

Wiz has built-in support for many ROM formats, and provides conveniences for configuring headers, and for automatically filling in checksums and rounding up ROM sizes. In some case the CPU architecture can be automatically detected by the output format. It currently exports the following output formats:

- `.bin` (raw binary format -- supports optional trimming)
- `.nes` (NES / Famicom - header config, auto-pad)
- `.gb`, `.gbc` (Game Boy - header config, auto-checksum, auto-pad)
- `.sms`, `.gg` (Sega Master System - header config, auto-checksum, auto-pad) 
- `.sfc`, `.smc` (SNES / Super Famicom - header config, auto-checksum, auto-pad)
- `.pce` (TurboGrafx-16 / PC Engine)
- `.a26` (Atari 2600)

Using Wiz
---------

Usage:

```
wiz [options] <input>
```

Arguments:

- `input` - the name of the input file to load. A filename of `-` implies the standard input stream.
- `-o filename` or `--output=filename` - the name of the output file to produce. the file extension determines the output format, and can sometimes automatically suggest a target system.
- `-m sys` or `--system=sys` - specifies the target system that the program is being built for. Supported systems: `6502`, `65c02` `rockwell65c02`, `wdc65c02`, `huc6280`, `z80`, `gb`, `wdc65816`, `spc700` 
- `-I dir` or `--import-dir=dir` - adds a directory to search for `import` and `embed` statements.
- `--color=setting` - sets the color preference for the terminal (Defaults to `auto`). `auto` will automatically detects if a TTY is attached, and only emits color escapes when there is one. `none` disables color. `ansi` will always use ANSI-escapes, even if no TTY is detected, or if the terminal uses different method of coloring (eg. Windows console).
- `--help` - lists a help message.
- `--version` - lists the current compiler version.

Example invocation:

```
wiz hello.wiz --system=6502 -o hello.nes
```

For now, please consult the example folder to see how Wiz can be used, or failing that, the compiler source code. Additional documentation isn't ready yet. In the future, there are plans to have a language reference, a reference for the instruction capabilities of each target, and tutorials showing how to make stuff. Help here is greatly appreciated!

Building Source
---------------

NOTE: All build instructions require a C++14, C++17, or later compiler. It will not build with an earlier version of the standard.

### Windows (Visual Studio)

- Install Visual Studio 2017 or later. 
- Open `vc/wiz.sln` in Visual Studio. Build the solution.
- If the build succeeds, a file named `wiz.exe` should exist in the `bin/` folder under the root of this repository.

### Windows (mingw)

- Install MinGW-w64. https://mingw-w64.org/
- Install GnuWin32 "Make for Windows". http://gnuwin32.sourceforge.net/packages/make.htm
- Open a terminal window, such as the Command Prompt (cmd) or something else.
- Run `make` in the terminal. For a debug target, run `make CFG=debug` instead. Feel free to add `-j8` or similar to build faster.
- If the build succeeds, a file named `wiz.exe` should exist in the `bin/` folder under the root of this repository.

### Windows (Cygwin)

- Install Cygwin
- Within Cygwin, install either GCC or Clang, and install GNU Make.
- Open a terminal window.
- Run `make` in the terminal. For a debug target, run `make CFG=debug` instead. Feel free to add `-j8` or similar to build faster.
- If the build succeeds, a file named `wiz.exe` should exist in the `bin/` folder under the root of this repository.
- NOTE: This exe produced in this fashion will require Cygwin, so it is not preferred for native Windows executable compilation.

### Mac OS X

- Install Xcode and install Command Line Tools. 
- Open a terminal window.
- Run `make` in the terminal. For a debug target, run `make CFG=debug` instead. Feel free to add `-j8` or similar to build faster.
- If the build succeeds, a file named `wiz` should exist in the `bin/` folder under the root of this repository.

### Linux

- Install GCC or Clang.
- Install GNU Make. 
- Run `make` in the terminal. For a debug target, run `make CFG=debug` instead. Feel free to add `-j8` or similar to build faster.
- If the build succeeds, a file named `wiz` should exist in the `bin/` folder under the root of this repository.

### Web / Emscripten

- Install emscripten.
- Install GNU Make.
- Run `make PLATFORM=emcc` in the terminal. For a debug target, run `make CFG=debug PLATFORM=emcc` instead.
- If the build succeeds, a file named `wiz.js` should exist in the `bin/` folder under the root of this repository.
- Copy the `bin/wiz.js` over `try-in-browser/wiz.js` to update the version used by the HTML try-in-browser sandbox.
- See the `try-in-browser/` test page for an example program that wraps the web version of the compiler.
- Note that the license of JSNES (JavaScript Nintendo emulator) is GPL, and if distributed with the try-in-browser code, the other code must be released under the GPL. However, the non-emulator code in this folder is explicitly MIT, so remove this emulator dependency if these licensing terms are desired.

Installing
----------

You don't need to install Wiz in order to use it. But if you want, you can install the compiler binary to your system path so that you can more easily use it. Wiz is currently only a single executable, and there are no standard libraries yet. It just has optional examples that are used by some test programs but they're not essential to running the compiler. Maybe down the road, this will change. In any case, here's some help.

### Windows

- First acquire `wiz.exe` for your platform or build it.
- Create a folder for Wiz.
- Copy `wiz.exe` into that folder.
- Add that folder to your `PATH` environment variable.
- Close and reopen any command window you had open.
- You should be now able to execute `wiz` from any folder, not just the one that contains it.

### Mac OS X, Linux

- First acquire `wiz` for your platform or build it.
- If you're using the makefile, you can use `make install` to install `wiz` to the system install location. (defaults to `/usr/local/bin` but can be customized via the `DESTDIR` and `PREFIX` variables)
- Alternatively, copy the `wiz` executable into your preferred install folder (eg. `/usr/local/bin` or wherever else)

Language Quick Guide
--------------------

This is a quick run-down of language features. This section is largely unfinished, so please consult example programs for better documentation!

### Banks

Banks are sections or segments that are used to hold the different pieces of the program, such as variables, constants, or executable code.

A bank declaration reserves a bank of memory with a given type and address. It has the following syntax:

```
bank name @ address, name @ address, name @ address : [type; size];
```

The type of bank of `bank` determines what kind of declarations can be placed there. 

- `vardata` is uninitialized RAM. useful for variables that can be written to and read back at runtime. Because this section is uninitialized, variables declared here must be assigned a value at execution time. Because of these limitations, compiled code and and constant data cannot be placed here.
- `prgdata` is ROM used for program code and constants. Cannot be written to.
- `constdata` is ROM used for constants and program code. Cannot be written to.
- `chrdata` is ROM used for character / tile graphic data. This type of bank has a special meaning on platforms like the NES, where character data is on a separate memory bus. It is otherwise the same as `constdata`.
- The distinction between `prgdata` and `constdata` will only exist on Harvard architectures, where the program and constant data live on separate buses. Otherwise, just use it for clarifying the purpose of ROM banks, if it matters.
- `varinitdata` is initialized RAM. Useful for programs with code and data that get uploaded to RAM. 

The size of a `bank` is the number of bytes that it will hold. Exceeding this size limitation is considered an error.

The address of a `bank` after the `@` is optional but if it appears, it must be an integer literal. Without an address, the `bank` can cannot contain label declarations.

After a bank is declared, it must be selected to be used by an `in` directive.

```
in bank {
    // ...
}
```

An address can also be provided to an `in` directive. If a `bank` previously had no address, then the address becomes the origin of the `bank`. However, if a `bank` previously had an address, the block is seeked ahead for to given address.

```
in bank @ address {
    // ...
}
```

Example:

```
bank zeropage @ 0x00 : [vardata; 256];

in zeropage {
    var timer : u8;
}
```

### Constants

A constant declaration reserves pre-initialized data that can't be written to at run-time. A constant can appear in a ROM bank, such as `prgdata`, `constdata`, or `chrdata`.

```
const name : type = value;
```

Example: 
```
const hp : u8 = 100;
const wow : [u8] = [0, 1, 2, 3, 4, 5, 6];
const egg : [u16; 5] = [12345, 54321, 32333, 49293];
```

If the type is provided, the initializer is narrowed to the type provided. This means that integer initializers of type `iexpr` that have no size can be implicitly narrowed to sized integer types like `u8`, and array initializers of type `[iexpr]` can be narrowed to `[u8]`, and so on.

Note that The size of an array type can be left off, if the tyep. 

If the type of a constant can be inferred from the value, then the type can be left off. However, only types with a known size can be constants. `iexpr` has unknown size, so type-suffixes on integers may be required.

Example: 

```
const length = 100u8;
const message = "HELLO WORLD";
const table = embed "table.bin";
```

The name of a constant is also optional, when placing data in the ROM is desired but a label to it isn't needed.

Example: 

```
const = embed "hero.chr";
```

Array comprehensions are a nice way to generate tables of data.

```
const tripled_values : [u8] = [x * 3 for let x in 0 .. 49];
```

### Variables

A variable declaration reserves a space for mutable storage. A variable can appear either in RAM bank such as `vardata`, or `varinitdata`.

```
var x : u8;
```

A variable declared in a `varinitdata` bank is allowed to have a initializer.

```
var x = 100; 
```

If an initializer is given, the type can be omitted if the initializer expression has a type of known size.

```
var x = 
var message = "hello"; 
```

A variable declared in a `vardata` bank, on the other hand, is reserved in uninitialized RAM, so it cannot have an initializer and can only be assigned a value at run-time.

### Extern Variables (Memory Mapped I/O Registers)

An `extern` variable can be used to declare a piece of external storage that exists at a specific address. Its primary use is for defining memory-mapped I/O registers on a system, so that they can referenced by name.

Example:

```
extern var reg @ 0xF000; // read/write
extern writeonly reg2 @ 0xF001; // write-only
extern const reg3 @ 0xF002; // read-only
```

### Let Definitions

A `let` definition can be used to bind a name to a constant expression. This can reduce the amount of magic numbers appearing in code. These don't require any ROM space where they're defined, but are instead are substituted into whatever expressions that use them.

```
let name = expression;
```

The expression of a `let` definition can be any valid expression. It is recommended to not pass expressions that contain
 side-effects. In the future, the compiler may forbid this, but for now this is not validated.

Example:

```
let HP = 100;
let NAME = "Hello";
```

It is also for a `let` definition to take arguments:

```
let CEILING_DIV(x, y) = (x + y - 1) / y;
let MAP_WIDTH = CEILING_DIV(1000, 8);
```

### Empty Statement

A single `;` is allowed anywhere, and is an empty statement. It has no effect.

### Assignment Statement

An assignment statement is used to store a value somewhere. Depending on where the value is stored, it can be retrieved again later.

```
dest = source; // assignment
dest += source; // compound assignment
dest++; // post-increment
dest--; // post-decrement
++dest; // pre-increment
--dest; // pre-decrement
```

The left-hand side of an assignment `=` is the destination, which can be a mutable register or a mutable location in memory. The right-hand side of an assignment `=` is the source, which can be a constant, a register, a readable memory location, or a run-time calculation. The source and destination must be of compatible type, or it is an error.

Example:

```
x = 100;
a = max_hp - hp;
damage = a;
```

After constant expression folding, any remaining run-time operations must have instructions available for them. It must be either an exact instruction that does the calcuation and stores it in the destination, or a series of instructions that first load the into the destination, and then modify the destination. For an assignment containing a binary expression like `a = b + c;`, instruction selection will first look for an exact instruction of form `a = b + c;`. Failing that, it will attempt decomposing the assignment `a = b + c` into the form `a = b; a = a + c;`.

Failing that, it is an error, and will probably require the assignment to be manually re-written in terms of multiple assignment statements. Assignments containing run-time operations cannot require an implicit temporary expression, or it is an error.

Most expressions are evaluated in left-to-right fashion, and as a result, they must be written so that they can be left-folded without temporaries. This requirement means parenthesized expressions can happen on the left, but the right-hand side will be a constant or simple term.

Example:

```
// This is OK, because it can be decomposed.
a = (5 + c) - b; 
// The above becomes:
// a = 5;
// a = a + c
// a = a - b;

// This is an error, even in its reduced form:
a = b - (5 + c);
// The above becomes:
// a = b;
// a = a - (5 + c) // the (5 + c) requires a temporary.
```

Oftentimes, it won't be possible to assign a value directly to memory. In these cases, the code will need to put an intermediate assignment into a register first, and then store the register into the memory afterwards.

Example:

```
// If there's no instruction available to assign immediate values to memory, this line will be an error.
hp = 100;

// It must be re-written to use a register in the middle.
// This will work if there is a register named a,
// and there is an instruction to load an immediate to register a,
// and there is another instruction to load memory with the value of the register.
a = 100;
hp = a;
```

Assignments can be chained together if they're compatible.

Example:

```
// This is the same as:
// a = x;
// y = a;
y = a = x;

// This is the same as:
// a = hp;
// a = a - 10;
// hp = a;
hp = a = hp - 10;
```

Compound assignment operators exist for most binary arithmetic operations. For some operation `+`, the statement `a += b` is the same as writing `a = a + b;`. If the right-hand side contains a calculation it is implicitly parenthesized, so that `a += b + c` is the same as `a = a + (b + c);`

Example:

```
a += 5; // same as a = a + 5;
```

There are also unary incrementation `++` and decrementation `--` operators.

Example:

```
x++;
y--;
++x;
++y;
```

Sometimes assignments might be used for their side-effects only, such as when writing to an external hardware register. Some such registers are `writeonly`, which indicates that they cannot be read back later, or that reading back will produce an open-bus value.

```
snes.ppu.bg_mode = a;
```

Similarly, there can be side-effects that occur when reading an external hardware register.

Assignments may also result in processor status flag registers being changed as a side-effect. Sometimes, a throw-away calculation in some registers or temporary memory might be performed simply to affect a conditional flag. The effect on flags depends on what sequence of instructions are generated as a result of the assignment. Not every operation affects every flag consistently, due to the design of the processor hardware. Knowing what instructions affect flags and which don't requires some study.

A common-to-find flag is the `zero` flag. If an instruction affects the zero flag, it might be used to indicate whether the result of the last operation was equal to zero. This side-effect can be used sometimes to avoid an extra comparison instruction later.

Example:

```
x = 10;
do {
    x--;
} while !zero;
```

A `carry` flag is a common flag to be affected by arithmetic operations such as addition/subtraction. The interpretation of a `carry` flag is dependent on the system. There are operations like add with carry `+#`, subtract with carry `-#`, rotate left with carry `<<<<#`, and rotate right with carry `>>>>#` which depend upon the last state of the `carry`. These operations can be useful for extending arithmetic operations to work on larger numeric values.

Example:

```
// add 0x0134 to bc.
c = a = c + 0x34;
b = a = b +# 0x01;
```

Some CPUs allow the `carry` to be set or cleared by assigning to it.

Examples:

```
carry = false;
carry = true;
```

### Call Statement

A call statement is a kind of statement that invokes a function. If the function being called produces a return value, the value is discarded. If the function call returns, the code following it will execute. Intrinsic instructions can also be called in the same manner.

```
function(argument1, argument2, argument3);
```

### Block Statement

A block statement is a scoped section of code that contains a collection of statements. It is started with an opening brace `{` and must be terminated later with a closing brace `}`. All declarations within.

Many types of statement contain block statements as part of their syntax. For these kinds of statements, the braces delimiting a block statement are not optional, even if only a single statement is present.

### If Statement

An `if` statement allows a block of code to be executed conditionally. The condition of an `if` statement is some expression of type `bool`. If a condition evaluates to `true`, then the block directly following it is executed. Otherwise, the block immediately following the condition is skipped, and the next alternative delimited by an `else if` is evaluated. Finally, there is `else` when all other alternatives have evaluated to `false`.

```
if condition {
    statements;
} else if condition {
    statements;
} else {
    statements;
}
```

A conditional expression can be a register term of type `bool` or its negation, or some combination of multiple `bool` operands with `&&` and `||` operators

Example:

```
a = hp;
if zero {
    dead();
}
```

There are also high-level operators for comparison: `==`, `!=`, `<`, `>=`, `>`, `<=`. Which kinds of comparisons are possible, and which registers can perform comparisons, depends on the target system.

Example:

```
a = gold;
x = item_index;

if a >= item_cost[x] {
    a -= item_cost[x];
    gold = a;
    buy_item(x);
} else {
    not_enough();
}
```

A conditional statement can also use "side-effect expressions" that evaluate some operation as required before a conditional expression. This sort of "setup-and-test" conditional can make some code read better, used carefully.

Example:

```
if { a = attack - defense; } && (carry || zero) {
    block_attack();
}
```

It is possible to use bit-indexing `$` to test a single-bit of a value and get its boolean result. Which registers are capable of this, as well as the bits that can be selected, depends on the target system.

Example:

```
if b$7 {
    vertical_flip();
} else if b$6 {
    horizontal_flip(); 
}
```

### While Statement

A `while` statement allows a block of code to be executed conditionally 0 or more times. The block is repeated as long as the condition provided still evaluates to `true`.

```
while condition {
    statement;
    statement;
    statement;
}
```

Example:

```
x = count;
while !zero {
    beep();
    x--;
}
```

### Do/While Statement

A `do` ... `while` statement allows a block of code to be executed unconditionally once, followed by conditional execution 0 or more times. The block is entered once, and will repeat more times as long as the condition provided still evaluates to `true`.

```
do {
    statement;
    statement;
    statement;
} while condition;
```

Note that `do` ... `while` statements are slightly more efficient than `while` statements, because they fall-through to the next code once the condition is `false`, and only jump to the top of the loop if the condition is still `true`.

### For Statement

A `for` statement allows iteration over a sequence of values, and executes a block of code for each step through the sequence.

```
for iterator in iterable {
    statement;
    statement;
    statement;
}
```

Currently, `for` loops only support iteration on comple-time integer ranges.

```
for counter in start .. end {
    statement;
    statement;
    statement;
}

for counter in start .. end by step {
    statement;
    statement;
    statement;
}
```

Example:

```
for x in 0 .. 31 {
    beep();
}
```


`start .. end` is an inclusive range. If the loop terminates normally, then after the loop, the counter provided will have the first value that is outside of the sequence.

Example:

```
// x is a u8 register that can be incremented and decremented.

// x will now be 32.
for x in 0 .. 31 {
    // ...
}

// x will be 0 after the loop.
for x in 31 .. 0 by -1 {
    // ...
}

// x will be 0 after the loop.
for x in 0 .. 255 {
    // ...
}
```

### Return Statements

A `return` statement is used to return from the currently executing function.

```
return;
return if condition;
```

If used in a function that has no return type, it cannot have a return value. If used in a function that has a return type, the return value must be of the same type.

Example:

```
func collect_egg() {
    a = egg_count;
    if a >= 5 {
        return;
    }

    egg_count++;
}

func triple(value : u8 in b) : u8 in a {
    return b + b + b;
}
```

A `return` that exists outside of any function body is a low-level `return` instruction. This would pop a location from the stack and jump there.

### Tail-Call Statements

A statement of form `return f();` is a tail-call statement. It gets subsituted for a `goto`, and avoids the overhead of subroutine call that would push a program counter to the stack. However, unlike a `goto`, a tail-call will evaluate arguments to a function, like a normal function call.

```
func call_subroutine(dest : u16 in hl) {
    goto *(dest as *func);
}
```

### Goto Statements

A `goto` is a low-level form of branching which jumps to another part of the program. Most high-level control structures are internally implemented in terms of `goto`.

```
goto destination;
goto destination if condition;
```

The destination of a `goto` can be a label, a function, or a function pointer expression.

Example:

```
x--; goto there if zero;

loop:
    goto loop;

there:
```

Code that uses `goto` should ensure that everything is already set up correctly before branching, because `goto` does not pass any arguments to its destination. If a `goto` that passes arguments to a function is required, use a tail-call statement of form `return f(arg, arg, arg)` instead.

### Break and Continue

Loop statements such as `while`, `do` ... `while`, `for` have two forms of branching statements in their blocks: `break` and `continue`.

```
break;
continue;
break if condition;
continue if condition;
```

A `break` statement will terminate a loop early, and jump ahead to the code that immediately follows a loop's block.

A `continue` statement will skip to the next iteration of the loop. In `for` loops, this will skip to the code that performs an increment and branch. In `while` and `do` ... `while` loops, this will skip to the conditional branch of the loop.

### Functions and Labels

A function is a piece of code that performs a specific set of actions and operations. They can be executed by other parts of the program, and can take arguments and produce a return value.

```
func f(arg : argument_type, arg2 : argument_type2, arg3 : argument_type3) : return_type {
    statement;
    statement;
    statement;
}
```

Example:

```
func poke(dest : u16 in hl, value : u8 in a) {
    *(dest as *u8) = a;
}

func sum(left : u8 in b, right : u8 in c) : u8 in a {
    return left + right;
}
```

Labels are like functions, but will never return, and cannot be passed arguments. As a low-level detail, labels of form `f:` are effectively the same thing as an empty-fallthrough function `#[fallthrough] func f() {}` placed directly before whatever code is executed. A label can be used in any bank that has a known address. (An address is currently required, but eventually, this requirement could relaxed to allow banks which use relative addressing only.)

Example:

```
label:
    goto label;
```

Labels or functions can be the target of call expressions, or `goto` statements.

### Inline Functions

An `inline func` is like `func` but inlined directly wherever it is used. As a consequence, the contents of the function are inserted wherever it is called, and the function has no address.

Example:

```
inline func times_two() {
    a = a << 1;
}

a = 20;
times_two();
times_two();
```

An `inline func` can still `return`, like a normal function. However, it will not use the stack. Instead, it will jump to a hidden return label that exists at the end of the function.

Because an `inline func` has no address, an `inline func` cannot be stored in a function pointer, and cannot be branched to from a `goto` or tail-call statement.

### Inline For Statement

An `inline for` allows unrolling a loop at compile-time. The iterator of the loop is optional but exists only for the scope of the `inline for` block. The value of the iteration at each iteration can be used by expressions within the loop.

```
inline for let iterator in iterable {
    statement;
    statement;
    statement;
}

inline for in iterable {
    statement;
    statement;
    statement;
}
```

Unlike a normal `for` statement, other iterables like arrays can be used as an iterable.

Example:

```
inline for let i in 1 .. 3 {
    hoot(i);
}

inline for in 1 .. 100 {
    nop();
}

inline for let i in [1, 2, 8, 10, 5, 3, 1] {
    y = i;
    draw();
}
```

### Attributes

Attributes are a form of special metadata that can be placed before a statement, directive, or declaration. They can provide the compiler with extra details about a particular piece of code.

Example:

```
#[fallthrough] func lasagna() {
    idx8();
    #[idx8] {
        x = 9;
    }

    idx16();
    #[idx16] {
        xx = 99;
        yy = xx;
    }

    while true {}
}

#[nmi] func vblank() {
    // ...
}

let INCLUDE_THIS_MESSAGE = true;

#[compile_if(INCLUDE_THIS_MESSAGE)] const message = "HI THIS IS A CONDITIONALLY COMPILED MESSAGE";

```

Attributes

- `compile_if(condition)` - indicates that some code should only be compiled if a particular compile-time boolean condition is `true`. If the condition is `false`, the code tagged with the attribute is not emitted.
- `fallthrough` - indicates the function might fall through into the immediately following code, and disables the implicit return at the end of the function. Useful for tagging functions that are guaranteed to never return, or functions that are meant to fall into some other code afterwards.
- `nmi` - indicates that a function handles a non-maskable interrupt request. All `return;` instructions will be translated into `nmireturn;` instead. (eg. `rti` on 6502, `retn` on Z80)
- `irq` - indicates that a function handles a maskable interrupt request. All `return;` instructions will be translated into `irqreturn;` instead.  (eg. `rti` on 6502, `reti` on Z80)

65816 Attributes

- `mem8` - assumes the tagged statements will be using an 8-bit accumulator and main memory-related instructions. (eg. accumulator arithmetic, storing/loading from accumulator, bit-shifting accumulator/memory)
- `mem16` - assumes the tagged statements will be using an 16-bit accumulator and main memory-related instructions.  (eg. accumulator arithmetic, storing/loading from accumulator, bit-shifting accumulator/memory)
- `idx8` - assumes the tagged statements will be using 8-bit index registers and indexing-related instructions. (eg. indexing, storing/loading from index registers, push/pop)
- `idx16` - assumes the tagged statements will be using 16-bit index registers and indexing-related instructions.  (eg. indexing, storing/loading from index registers, push/pop)

### Config Directives

Depending on the output file format selected for the program, there are extra options that can be configured.

```
config {
    option = value,
    option = value,
    option = value
}
```

See Formats section for possible options.

### Types

Built-in Types:

```
// Boolean
bool
// Compie-time integer expression ("arbitrary precision", unknown size, but currently 128-bit)
iexpr
// Unsigned sized integer
u8
u16
u24
u32
u64
// Signed sized integer
i8
i16
i24
i32
i64
```

Array Types:

```
// Array. eg. [u8; 100]
[T; N]
// Implicitly-sized array type. (size determined by initializer)
[T]
```

Designated Storage Types:

```
// Designated storage for parameter-passing and named locals.
// eg. u8 in a
T in expr
```

User-defined types:

```
// Structure. Data members have increasing start addresses.
struct Point {
    x : u8,
    y : u8
}

// Union. Data members have the same start address.
union Union {
    byte : u8,
    word : u16
}

// Enum. A strongly-typed enumeration type.
// Uses the same storage as its underlying type.
// It cannot be used as integer without casting.
enum ItemType : u8 {
    Consumable,
    Key,    
    Weapon,
    Armor,
    Accessory,
}

// Type alias. Gives another name to the same type.
// A typealias is interchangable with the type it aliases.
typealias byte = u8;
```

### Expressions

```
    (a) // parentheses

    -a // signed negation. integer
    +a // signed identity. integer

    // Arithmetic operators.
    // integer + integer -> integer
    a + b // add. integer + integer -> integer
    a +# b // add with carry. integer +# integer -> integer
    a - b // subtract. integer - integer -> integer
    a -# b // subtract with carry. integer -# integer -> integer
    a * b // multiply. integer * integer -> integer
    a / b // divide. integer / integer -> integer
    a % b // modulo. integer % integer -> integer
    a << b // arithmetic left shift. integer << integer -> integer
    a >> b // arithmetic right shift. integer >> integer -> integer
    a <<< b // logical left shift. integer <<< integer -> integer
    a >>> b // logical right shift. integer >>> integer -> integer
    a <<<< b // left rotate (N bits). integer <<<< integer -> integer
    a >>>> b // right rotate (N bits). integer >>>> integer -> integer
    a <<<<# b // left rotate with carry (N+1 bits). integer <<<<# integer -> integer
    a >>>># b // right rotate with carry (N+1 bits). integer >>>># integer -> integer

    // Logical operators.
    !a // logical negation. bool
    a && b // logical and. bool && bool -> bool
    a || b // logical or. bool || bool -> bool

    ~a // bitwise negation. integer | integer -> integer
    a & b // bitwise and. integer | integer -> integer
    a | b // bitwise or. integer | integer -> integer
    a ^ b // bitwise xor. integer ^ integer -> integer
    a $ b // bit indexing. integer $ integer -> bool
    <:a // low byte access (bits 0 .. 7). integer -> u8
    >:a // high byte access (bits 8 .. 15). integer -> u8
    #:a // bank byte access (bits 16 .. 23). integer -> u8

    a++ // post-increment. integer
    a-- // post-decrement. integer
    ++a // pre-increment. integer
    --a // pre-decrement. integer

    *a // indirection. *T -> T
    &a // address of. term must be an addressable l-value. T -> *T
    a[n] // indexing. returns the nth element of a. Possibly the nth item from its start address.
    a.b // member access.

    a as T // cast operator.

    f(a, b, c, d) // call.

    1 // unsized integer expression. iexpr
    1u8 // integer with size suffix. in this case, u8

    "hello" // string literal. [u8; N]
    [expr for let iterator in iterable] // array comprehension. [T; N]
    [expr; size] // array pad literal. [T; N]
    [expr, expr, expr] // array literal. [T; N]
    (a, b, c, d) // tuple literal (reserved but not implemented yet). (T, T2, T3, ..., TN)

    a ~ b // concatenation [T; M] ~ [T; N] -> [T; M + N]
```

### Imports

To include another file in the program, use `import` directives.

```
import "path";
```

The path is a string literal. This path is the name of the .wiz file, without its extension.

Example:

```
import "nes";
import "magic";
```

### Embeds

To include a binary asset, use `embed` expressions.

```
embed "path"
```

The path passed to an `embed` should include the file extension. The contents of the embedded file are loaded into compiler memory and cached, and then inserted verbatim as a `[u8]` expression anywhere they are embedded.

Example:

```
const data = embed "hero.chr";
```

Platforms
---------

The registers, addressing modes and operations all vary depending on the system.

TODO: write about each platform's specifics.

### MOS 6502

The MOS 6502 was a very popular 8-bit CPU used by a number of 8-bit game consoles and computer systems. Some common machines that used the 6502:

- NES
- Atari 2600 (VCS)
- Atari 5200
- Atari 7800
- Atari 8-bit family (400, 800, 1200, 800XL, 1200XL, etc)
- Commodore 64
- Commodore 128
- Commodore VIC-20
- Apple II

Miscellaneous:

- `sizeof(*u8) = sizeof(u16)`
- `sizeof(far *u8) = sizeof(u24)`
- `carry` is the inverse of the borrow flag in subtraction/comparison (`true` when no borrow)

Registers:

- `a : u8` - the accumulator
- `x : u8` - index register x
- `y : u8` - index register y
- `s : u8` - stack pointer register
- `p : u8` - processor flag register
- `zero : bool` - the zero flag, used to indicate if the result of the last operation resulted in the value 0.
- `carry : bool` - the carry flag, used to indicate if the last operation resulted in a carry outside of the range 0 .. 255. For addition, the `carry` flag is `true` when an addition resulted in a carry, and `false` otherwise. For subtraction and comparison, the `carry` flag is `false` when there is a borrow (left-hand side was less than the right-hand side), and `true` otherwise. The `carry` also indicates the bit shifted out from a bit-shift operation.
- `negative : bool` - the negative flag, used to indicate the sign bit (bit 7) of the last operation.
- `overflow : bool` - the overflow flag, used to indicate if the last operation resulted an arithmetic overflow. If an overflow happened, the sign bit (bit 7) of the result is flipped.
- `decimal : bool` - the decimal flag, used to indicate whether decimal mode is active. If `true`, numbers are treated as if they are packed binary-coded decimal (BCD) format. If `false`, numbers are treated as binary numbers. Some 6502 CPUs like the 2A03/2A07 used in the NES do not have a decimal mode, so BCD arithmetic must be manually accomplished through software.
- `nointerrupt : bool` - the interrupt disable flag, used to indicate that maskable interrupt requests (IRQ) are disallowed. This will not disable non-maskable interrupts (NMI).

Intrinsics:

- `cmp(left, right)` - compares two values, by performing a subtraction without storing the result in memory. `zero` indicates whether `left == right`, `carry` indicates whether `left >= right`.
- `bit(mem)` - bitwise test between the accumulator and a value in memory. `zero` indicates whether `(a & mem) == 0`, `overflow` is set to `mem $ 6`, and `negative` is set to `mem $ 7`.
- `irqcall(code)` - manually requests an IRQ, code is a parameter that is put after irqcall opcode, which can be read from the IRQ handler routine.
- `push(value)` - pushes the given value to the stack.
- `pop(value)` - pushes the given value to the stack.
- `nop()` - does nothing, and uses 1 byte and 2 cycles.

Tests and Branches:

- `a`, `x` and `y` can be used with comparison operators (`==`, `!=`, `<`, `>`, `<=`, or `>=`) inside conditional branches. A branch like `if a >= 5 { ... }` becomes `if { cmp(a, 5); } && carry { ... }`.
- `a & mem == 0` or `a & mem != 0` can be used as a conditional branches. `if a & mem == 0 { ... }` becomes `if { bit(mem); } && zero { ... }` and `if a & mem != 0 { ... }` becomes `if { bit(mem) } && !zero { ... }` 
- `mem $ 6` can be used as a conditional branch for testing bit 6 of memory. `if mem $ 6 { ... }` becomes `if { bit(mem); } && overflow { ... }` 
- `mem $ 7` can be used as a conditional branch for testing bit 7 of memory. `if mem $ 7 { ... }` becomes `if { bit(mem); } && negative { ... }` 

Addressing Modes:

- `{0..255}` - immediate
- `*({0..255} as *u8)` - zero-page
- `*(({0..255} + x) as *u8)` - zero-page indexed by x
- `*(({0..255} + y) as *u8)` - zero-page indexed by y
- `*(*(({0..255} + x) as *u16) as *u8)` - zero-page indexed by x indirect
- `*(({0..255} + y) as *u8)` - zero-page indexed by y
- `*((*({0..255} as *u16) + y) as *u8)` - zero-page indirect indexed by y
- `*({0..65535} as *u8)` - absolute indexed by x
- `*(({0..65535} + x) as *u8)` - absolute indexed by x
- `*(({0..65535} + y) as *u8)` - absolute indexed by y

Instructions:

```
a = {0..255}
a = *({0..255} as *u8)
a = *(({0..255} + x) as *u8)
a = *(*(({0..255} + x) as *u16) as *u8)
a = *(*(({0..255} as *u16) + y) as *u8)
a = *({0..65535} as *u8)
a = *(({0..65535} + x) as *u8)
a = *(({0..65535} + y) as *u8)

a += {0..255}
a += *({0..255} as *u8)
a += *(({0..255} + x) as *u8)
a += *(*(({0..255} + x) as *u16) as *u8)
a += *(*(({0..255} as *u16) + y) as *u8)
a += *({0..65535} as *u8)
a += *(({0..65535} + x) as *u8)
a += *(({0..65535} + y) as *u8)

a +#= {0..255}
a +#= *({0..255} as *u8)
a +#= *(({0..255} + x) as *u8)
a +#= *(*(({0..255} + x) as *u16) as *u8)
a +#= *(*(({0..255} as *u16) + y) as *u8)
a +#= *({0..65535} as *u8)
a +#= *(({0..65535} + x) as *u8)
a +#= *(({0..65535} + y) as *u8)

a -= {0..255}
a -= *({0..255} as *u8)
a -= *(({0..255} + x) as *u8)
a -= *(*(({0..255} + x) as *u16) as *u8)
a -= *(*(({0..255} as *u16) + y) as *u8)
a -= *({0..65535} as *u8)
a -= *(({0..65535} + x) as *u8)
a -= *(({0..65535} + y) as *u8)

a -#= {0..255}
a -#= *({0..255} as *u8)
a -#= *(({0..255} + x) as *u8)
a -#= *(*(({0..255} + x) as *u16) as *u8)
a -#= *(*(({0..255} as *u16) + y) as *u8)
a -#= *({0..65535} as *u8)
a -#= *(({0..65535} + x) as *u8)
a -#= *(({0..65535} + y) as *u8)

a |= {0..255}
a |= *({0..255} as *u8)
a |= *(({0..255} + x) as *u8)
a |= *(*(({0..255} + x) as *u16) as *u8)
a |= *(*(({0..255} as *u16) + y) as *u8)
a |= *({0..65535} as *u8)
a |= *(({0..65535} + x) as *u8)
a |= *(({0..65535} + y) as *u8)

a &= {0..255}
a &= *({0..255} as *u8)
a &= *(({0..255} + x) as *u8)
a &= *(*(({0..255} + x) as *u16) as *u8)
a &= *(*(({0..255} as *u16) + y) as *u8)
a &= *({0..65535} as *u8)
a &= *(({0..65535} + x) as *u8)
a &= *(({0..65535} + y) as *u8)

a ^= {0..255}
a ^= *({0..255} as *u8)
a ^= *(({0..255} + x) as *u8)
a ^= *(*(({0..255} + x) as *u16) as *u8)
a ^= *(*(({0..255} as *u16) + y) as *u8)
a ^= *({0..65535} as *u8)
a ^= *(({0..65535} + x) as *u8)
a ^= *(({0..65535} + y) as *u8)

{0..255} = a
*({0..255} as *u8) = a
*(({0..255} + x) as *u8) = a
*(*(({0..255} + x) as *u16) as *u8) = a
*(*(({0..255} as *u16) + y) as *u8) = a
*({0..65535} as *u8) = a
*(({0..65535} + x) as *u8) = a
*(({0..65535} + y) as *u8) = a

cmp(a, {0..255})
cmp(a, *({0..255} as *u8))
cmp(a, *(({0..255} + x) as *u8))
cmp(a, *(*(({0..255} + x) as *u16) as *u8))
cmp(a, *(*(({0..255} as *u16) + y) as *u8))
cmp(a, *({0..65535} as *u8))
cmp(a, *(({0..65535} + x) as *u8))
cmp(a, *(({0..65535} + y) as *u8))

bit(*({0..255} as *u8))
bit(*({0..65535} as *u8))

a = x
x = a
a = y
y = a
x = s
s = x

x = {0..255};
x = *({0..255} as *u8)
x = *(({0..255} + y) as *u8)
x = *({0..65535} as *u8)
x = *(({0..65535} + y) as *u8)

*({0..255} as *u8) = x
*(({0..255} + y) as *u8) = x
*({0..65535} as *u8) = x

cmp(x, {0..255})
cmp(x, *({0..255} as *u8))
cmp(x, *({0..65535} as *u8))

y = {0..255};
y = *({0..255} as *u8)
y = *(({0..255} + y) as *u8)
y = *({0..65535} as *u8)
y = *(({0..65535} + y) as *u8)

*({0..255} as *u8) = y
*(({0..255} + x) as *u8) = y
*({0..65535} as *u8) = y

cmp(y, {0..255})
cmp(y, *({0..255} as *u8))
cmp(y, *({0..65535} as *u8))

push(a)
push(p)

a = pop()
p = pop()

++*({0..255} as *u8)
++*(({0..255} + x) as *u8)
++*({0..65535} as *u8)
++*(({0..65535} + x) as *u8)
++x
++y

--*({0..255} as *u8)
--*(({0..255} + x) as *u8)
--*({0..65535} as *u8)
--*(({0..65535} + x) as *u8)
--x
--y

a = ~a
a = -a

a <<= {0..7}
*({0..255} as *u8) <<= {0..7}
*(({0..255} + x) as *u8) <<= {0..7}
*({0..65535} as *u8) <<= {0..7}
*(({0..65535} + x) as *u8) <<= {0..7}

a <<<= {0..7}
*({0..255} as *u8) <<<= {0..7}
*(({0..255} + x) as *u8) <<<= {0..7}
*({0..65535} as *u8) <<<= {0..7}
*(({0..65535} + x) as *u8) <<<= {0..7}

a >>>= {0..7}
*({0..255} as *u8) >>>= {0..7}
*(({0..255} + x) as *u8) >>>= {0..7}
*({0..65535} as *u8) >>>= {0..7}
*(({0..65535} + x) as *u8) >>>= {0..7}

a <<<<#= {0..7}
*({0..255} as *u8) <<<<#= {0..7}
*(({0..255} + x) as *u8) <<<<#= {0..7}
*({0..65535} as *u8) <<<<#= {0..7}
*(({0..65535} + x) as *u8) <<<<#= {0..7}

a >>>>#= {0..7}
*({0..255} as *u8) >>>>#= {0..7}
*(({0..255} + x) as *u8) >>>>#= {0..7}
*({0..65535} as *u8) >>>>#= {0..7}
*(({0..65535} + x) as *u8) >>>>#= {0..7}

goto {-128..127} if zero
goto {-128..127} if !zero
goto {-128..127} if carry
goto {-128..127} if !carry
goto {-128..127} if negative
goto {-128..127} if !negative
goto {-128..127} if overflow
goto {-128..127} if !overflow

goto {0..65535}
^goto {0..65535} if zero
^goto {0..65535} if !zero
^goto {0..65535} if carry
^goto {0..65535} if !carry
^goto {0..65535} if negative
^goto {0..65535} if !negative
^goto {0..65535} if overflow
^goto {0..65535} if !overflow

goto *({0..65535} as *u16)

return
irqreturn
nmireturn

irqcall({0..255})

nop()

carry = false
carry = true

decimal = false
decimal = true

nointerrupt = false
nointerrupt = true

overflow = false
```

### MOS 65C02

The MOS 65C02 is like the MOS 6502, but also has some extra instructions.

Extra Intrinsics:

- `test_and_reset(mem)` - like `bit(mem)`, but also uses the accumulator to turn off bits in mem, as if `mem &= ~a;` is applied.
- `test_and_set(mem)` - like `bit(mem)`, but also uses the accumulator to turn on bits in mem, as if `mem |= a;` is applied.

Extra Instructions:

```
a = *(*({0..255} as *u16) as *u8)
a += *(*({0..255} as *u16) as *u8)
a +#= *(*({0..255} as *u16) as *u8)
a -= *(*({0..255} as *u16) as *u8)
a -#= *(*({0..255} as *u16) as *u8)
a |= *(*({0..255} as *u16) as *u8)
a &= *(*({0..255} as *u16) as *u8)
a ^= *(*({0..255} as *u16) as *u8)
*(*({0..255} as *u16) as *u8) = a
cmp(a, *(*({0..255} as *u16) as *u8))

bit({0..255})
bit(*(({0..255} + x) as *u8))
bit(*(({0..65535} + x) as *u8))

++a
--a

goto {-128..127}
^goto {0..65535}

goto *(({0..65535} + x) as *u16)

push(x)
push(y)

x = pop()
y = pop()

*({0..255} as *u8) = 0
*(({0..255} + x) as *u8) = 0
*({0..65535} as *u8) = 0
*(({0..65535} + x) as *u8) = 0

test_and_reset(*({0..255} as *u8))
test_and_reset(*({0..65535} as *u8))

test_and_set(*({0..255} as *u8))
test_and_set(*({0..65535} as *u8))
```

### Rockwell 65C02

The Rockwell 65C02 is like the MOS 65C02, but also has some extra instructions.

Extra Instructions:

```
goto {-128..127} if *({0..255} as *u8) $ {0..7}
goto {-128..127} if !(*({0..255} as *u8)) $ {0..7}
^goto {0..65535} if *({0..255} as *u8) $ {0..7}
^goto {0..65535} if !(*({0..255} as *u8)) $ {0..7}

*({0..255} as *u8) $ {0..7} = false
*({0..255} as *u8) $ {0..7} = true
```

### WDC 65C02

The WDC 65C02 is like the Rockwell 65C02 but also has some extra instructions.

Extra Intrinsics:
- `stop_until_reset()` - stops execution until a hardware reset occurs.
- `wait_until_reset()` - sleeps until a hardware interrupt occurs.

Extra Instructions:

```
stop_until_reset()
wait_until_interrupt()
```

### HuC 6280

The HuC6280 is like the Rockwell 65C02, but has some extra registers and instructions. Also, zero page instructions are at 0x2000 .. 0x20FF rather than the usual 0x00 .. 0xFF of other 6502 processors. This can be bank-switched with the `mpr1` register.

Extra Registers:

- `turbo_speed : bool` - CPU turbo mode setting
- `vdc_select : u8` - VDC address select register (fast access)
- `vdc_data_l : u8` - VDC data register low (fast access)
- `vdc_data_h : u8` - VDC data register high (fast access) 
- The VDC registers are also available in a memory mapped I/O registers, these registers are just for special quick-write instructions in the CPU.
- `mpr0 : u8` - mapper register 0, maps the memory bank available at address range `0x0000 .. 0x1FFF`.
- `mpr1 : u8` - mapper register 1, maps the memory bank available at address range `0x2000 .. 0x3FFF`. Affects zero page.
- `mpr2 : u8` - mapper register 2, maps the memory bank available at address range `0x4000 .. 0x5FFF`.
- `mpr3 : u8` - mapper register 3, maps the memory bank available at address range `0x6000 .. 0x7FFF`.
- `mpr4 : u8` - mapper register 4, maps the memory bank available at address range `0x8000 .. 0x9FFF`.
- `mpr5 : u8` - mapper register 5, maps the memory bank available at address range `0xA000 .. 0xBFFF`.
- `mpr6 : u8` - mapper register 6, maps the memory bank available at address range `0xC000 .. 0xDFFF`.
- `mpr7 : u8` - mapper register 7, maps the memory bank available at address range `0xE000 .. 0xFFFF`.

Extra Intrinsics

- `swap(left, right)` - exchanges the values between the left and right operand.
- `transfer_alternate_to_increment(source, dest, len)` - transfers `len` bytes of data from an alternating `source` address (`source`, `source + 1`, `source`, ...) to an incrementing `dest` address (`dest`, `dest + 1`, `dest + 2`, ...). 
- `transfer_increment_to_alternate(source, dest, len)` - transfers `len` bytes of data from an incrementing `source` address (`source`, `source + 1`, `source + 2`, ...)  to an alternating `dest` address (`dest`, `dest + 1`, `dest`, ...). 
- `transfer_decrement_to_decrement(source, dest, len)` - transfers `len` bytes of data from an decrementing `source` address address (`source`, `source - 1`, `source - 2`, ...) to an decrementing `dest` address (`dest`, `dest - 1`, `dest - 2`, ...). 
- `transfer_increment_to_increment(source, dest, len)` - transfers `len` bytes of data from an incrementing `source` address address (`source`, `source + 1`, `source + 2`, ...) to an incrementing `dest` address (`dest`, `dest + 1`, `dest + 2`, ...).
- `transfer_increment_to_fixed(source, dest, len)` - transfers `len` bytes of data from an incrementing `source` address address (`source`, `source + 1`, `source + 2`, ...) to a fixed `dest` address (`dest`, `dest`, `dest`, ...).
- `mpr_set(mask, a)` - sets `mpr0` .. `mpr7` registers to the value in the accumulator, using `mask` to indicate which of registers are being selected to update. For N = 0 .. 7, if `mask $ N` is `true` (`(mask & (1 << N)) != 0`), then `mprN` will be set to `a`.
- `tst(imm, mem)` - like `bit` but the `zero` flag is the result of whether `(imm & bit) == 0`.

Extra Instructions:

```
a = 0
x = 0
y = 0

turbo_speed = false
turbo_speed = true

swap(a, x)
swap(a, y)
swap(x, y)

*(x as *u8) += {0..255}
*(x as *u8) += *({0..255} as *u8)
*(x as *u8) += *(({0..255} + x) as *u8)
*(x as *u8) += *(*(({0..255} + x) as *u16) as *u8)
*(x as *u8) += *(*(({0..255} as *u16) + y) as *u8)
*(x as *u8) += *({0..65535} as *u8)
*(x as *u8) += *(({0..65535} + x) as *u8)
*(x as *u8) += *(({0..65535} + y) as *u8)

*(x as *u8) +#= {0..255}
*(x as *u8) +#= *({0..255} as *u8)
*(x as *u8) +#= *(({0..255} + x) as *u8)
*(x as *u8) +#= *(*(({0..255} + x) as *u16) as *u8)
*(x as *u8) +#= *(*(({0..255} as *u16) + y) as *u8)
*(x as *u8) +#= *({0..65535} as *u8)
*(x as *u8) +#= *(({0..65535} + x) as *u8)
*(x as *u8) +#= *(({0..65535} + y) as *u8)

*(x as *u8) |= {0..255}
*(x as *u8) |= *({0..255} as *u8)
*(x as *u8) |= *(({0..255} + x) as *u8)
*(x as *u8) |= *(*(({0..255} + x) as *u16) as *u8)
*(x as *u8) |= *(*(({0..255} as *u16) + y) as *u8)
*(x as *u8) |= *({0..65535} as *u8)
*(x as *u8) |= *(({0..65535} + x) as *u8)
*(x as *u8) |= *(({0..65535} + y) as *u8)

*(x as *u8) &= {0..255}
*(x as *u8) &= *({0..255} as *u8)
*(x as *u8) &= *(({0..255} + x) as *u8)
*(x as *u8) &= *(*(({0..255} + x) as *u16) as *u8)
*(x as *u8) &= *(*(({0..255} as *u16) + y) as *u8)
*(x as *u8) &= *({0..65535} as *u8)
*(x as *u8) &= *(({0..65535} + x) as *u8)
*(x as *u8) &= *(({0..65535} + y) as *u8)

*(x as *u8) ^= {0..255}
*(x as *u8) ^= *({0..255} as *u8)
*(x as *u8) ^= *(({0..255} + x) as *u8)
*(x as *u8) ^= *(*(({0..255} + x) as *u16) as *u8)
*(x as *u8) ^= *(*(({0..255} as *u16) + y) as *u8)
*(x as *u8) ^= *({0..65535} as *u8)
*(x as *u8) ^= *(({0..65535} + x) as *u8)
*(x as *u8) ^= *(({0..65535} + y) as *u8)

vdc_select = {0..255}
vdc_data_l = {0..255}
vdc_data_h = {0..255}

transfer_alternate_to_increment({0..65535}, {0..65535}, {0..65535})
transfer_increment_to_alternate({0..65535}, {0..65535}, {0..65535})
transfer_decrement_to_decrement({0..65535}, {0..65535}, {0..65535})
transfer_increment_to_increment({0..65535}, {0..65535}, {0..65535})
transfer_increment_to_fixed({0..65535}, {0..65535}, {0..65535})

mpr_set({0..255}, a)
mpr0 = a
mpr1 = a
mpr2 = a
mpr3 = a
mpr4 = a
mpr5 = a
mpr6 = a
mpr7 = a
a = mpr0
a = mpr1
a = mpr2
a = mpr3
a = mpr4
a = mpr5
a = mpr6
a = mpr7

tst({0..255}, *({0..255} as *u8))
tst({0..255}, *(({0..255} + x) as *u8))
tst({0..255}, *(({0..65535} + x) as *u8))
tst({0..255}, *(({0..65535} + x) as *u8))
```

### Zilog Z80

Miscellaneous

- `sizeof(*u8) = sizeof(u16)`
- `sizeof(far *u8) = sizeof(u24)`
- `carry` acts as a borrow flag in subtraction/comparison

Registers

- `a : u8` - accumulator
- `b : u8`, `c : u8`, `d : u8`, `e : u8`, `h : u8`, `l : u8` - general registers
- `ixh : u8`, `ixl : u8`, `iyh : u8`, `iyl : u8` - index register sub-registers (undocumented)
- `af : u16` - accumulator + flags pair
- `bc : u16`, `de : u8`, `hl : u8` - general register pairs
- `ix : u16`, `iy : u8` - index registers
- `zero : bool` - the zero flag, used to indicate if the result of the last operation resulted in the value 0.
- `carry : bool` - the carry flag, used to indicate if the last operation resulted in a carry outside of the range 0 .. 255. For addition, the `carry` flag is `true` when an addition resulted in a carry, and `false` otherwise. For subtraction and comparison, the `carry` flag is `true` when there is a borrow (left-hand side was less than the right-hand side), and `false` otherwise. The `carry` also indicates the bit shifted out from a bit-shift operation.
- `overflow : bool`
- `negative : bool`
- `interrupt : bool` - the interrupt enable flag, used to indicate whether maskable interrupts should be activated.
- `interrupt_mode : u8`
- `interrupt_page : u8`
- `memory_refresh : u8`

Intrinsics

- `push(value)`
- `pop()`
- `nop()`
- `halt()`
- `decimal_adjust()`
- `swap(left, right)`
- `swap_shadow()`
- `load_increment()`
- `load_decrement()`
- `load_increment_repeat()`
- `load_decrement_repeat()`
- `bit`
- `cmp`
- `comare_increment`
- `comare_decrement`
- `comare_increment_repeat`
- `comare_decrement_repeat`
- `rotate_left_digits`
- `rotate_right_digits`
- `io_read`
- `io_read_increment`
- `io_read_decrement`
- `io_read_increment_repeat`
- `io_read_decrement_repeat`
- `io_write`
- `io_write_increment`
- `io_write_decrement`
- `io_write_increment_repeat`
- `io_write_decrement_repeat`
- `decrement_branch_not_zero`

Addressing Modes

- TODO

Instructions

```
TODO
```

Documentation TODO

### Game Boy

Miscellaneous

- `sizeof(*u8) = sizeof(u16)`
- `sizeof(far *u8) = sizeof(u24)`
- `carry` acts as a borrow flag in subtraction/comparison

Registers

- `a : u8` - accumulator
- `b : u8`, `c : u8`, `d : u8`, `e : u8`, `h : u8`, `l : u8` - general registers
- `af : u16` - accumulator + flags pair
- `bc : u16`, `de : u8`, `hl : u8` - general register pairs
- `zero : bool` - the zero flag, used to indicate if the result of the last operation resulted in the value 0.
- `carry : bool` - the carry flag, used to indicate if the last operation resulted in a carry outside of the range 0 .. 255. For addition, the `carry` flag is `true` when an addition resulted in a carry, and `false` otherwise. For subtraction and comparison, the `carry` flag is `true` when there is a borrow (left-hand side was less than the right-hand side), and `false` otherwise. The `carry` also indicates the bit shifted out from a bit-shift operation.
- `interrupt : bool` - the interrupt enable flag, used to indicate whether maskable interrupts should be activated.

Intrinsics

- `push(value)` - pushes a value to the stack.
- `pop()` - pops a value from the stack.
- `nop()` - does nothing, but takes 1 byte and 4 cycles.
- `halt()` - halts execution until the next interrupt occurs. saves CPU and battery.
- `stop()` - goes into a low-power sleep until a joy-pad interrupt occurs. also used after changing the Game Boy Color's speed to apply the change, by setting the joy-pad register to force an immediate wake.
- `decimal_adjust()` - corrects the result in the accumulator after performing arithmetic on packed binary-coded decimal (BCD) values. put a call to this after an addition or subtraction involving packed BCD values.
- `swap_digits(r)` - swaps the low and high nybbles of the register `r`.
- `debug_break()` - emits an `ld b, b` nop opcode can be used to triggers a breakpoint in the Game Boy emulator BGB.
- `bit(r, n)` - test bit `n` of the register `r`, and updates the `zero` flag.
- `cmp(left, right)` - compares two values, by performing a subtraction without storing the result in memory. `zero` indicates whether `left == right`, `carry` indicates whether `left < right`.

Addressing Modes

- TODO

Instructions

```
TODO
```

Documentation TODO

### WDC 65816

Documentation TODO

### SPC 700

Miscellaneous

- TODO

Registers

- `a : u8` - accumulator
- `x : u8` - index register x
- `y : u8` - index register y
- `sp : u8` - stack pointer register
- `psw : u8` - processor status word register
- `ya : u16` - ya register pair
- `negative : bool`
- `overflow : bool`
- `direct_page : bool`
- `interrupt : bool`
- `zero : bool`
- `carry : bool`

Instrinsics

- `push(value)`
- `pop()`
- `irqcall()`
- `nop()`
- `sleep()`
- `stop()`
- `swap_digits(a)`
- `test_and_set(a, mem)`
- `test_and_clear(a, mem)`
- `divmod(ya, x)`
- `decimal_adjust_add()`
- `decimal_adjust_sub()`
- `cmp(left, right)`
- `compare_branch_not_equal(a, operand, destination)`
- `decrement_branch_not_equal(operand, destination)`

Addressing Modes

- TODO

Instructions

```
TODO
```

Documentation TODO

Formats
-------

### Binary

The default format for any program being output. Wiz peforms no special header or footer handling.

Config options:

- `trim` (bool) - if `true`, then the unused portion at the end of final bank in the output will be trimmed from the file. Useful for shortening the size of binaries that are going to be embedded in another program. (Defaults to `false`)

### Nintendo Entertainment System / Famicom

The iNES format is an emulator format for NES ROMs that is commonly supported. It has the `.nes` file extension.

Format Specifics:

- 16-byte iNES header at the start of the output binary
- PRG banks defined by `constdata` or `prgdata` bank. Padded to nearest multiple of 16 KiB.
- CHR banks defined by `chrdata` banks. Padded to nearest multiple of 8 KiB.

Config options:

- `cart_type` (string) - the type of cartridge that the cart is using. It can be one of the following strings:

```
"nrom"
"sxrom"
"mmc1"
"uxrom"
"cnrom"
"txrom"
"mmc3"
"mmc6"
"exrom"
"mmc5"
"axrom"
"pxrom"
"mmc2"
"fxrom"
"mmc4"
"color-dreams"
"cprom"
"24c02"
"ss8806"
"n163"
"vrc4a"
"vrc4c"
"vrc2a"
"vrc2b"
"vrc4e"
"vrc6a"
"vrc4b"
"vrc4d"
"vrc6b"
"action-53"
"unrom512"
"bnrom"
"rambo1"
"gxrom"
"mxrom"
"after-burner"
"fme7"
"sunsoft5b"
"codemasters"
"vrc3"
"vrc1"
"n109"
"vrc7"
"gtrom"
"txsrom"
"tqrom"
"24c01"
"dxrom"
"n118"
"n175"
"n340"
"action52"
"codemasters-quattro"
```

- `cart_type_id` (integer) - the type of cartridge that the cart is using, as an iNES mapper number. Takes precedence over `cart_type_id`, so use this if `cart_type` is not recognized. (If a cart type name is missing, feel free to request it!)
- If neither `cart_type` nor `cart_type_id` are specified, then the cart is NROM (mapper 0). This no special memory mapper, so it is limited to a view of 32K of ROM at addresses 0x8000 .. 0xFFFF in memory.
- `vertical_mirror` (bool) - whether or not this cart uses vertical mirroring. (Defaults to `false`)
- `battery` (bool) - whether or not this cart uses battery backed save RAM. (Defaults to `false`)
- `four_screen` (bool) - whether or not this cart contains RAM for extra nametables, allowing use of all four screens without PPU nametable mirroring. (Defaults to `false`)
- `prg_ram_size` (integer) - the number of bytes to use for PRG RAM. Must be divisble by 8192 bytes, and can be no more than 2088960 bytes (8192 bytes * 255 banks). (Defaults to 0)

### Game Boy

The Game Boy has a standardized header format that is is contained a fixed location in ROM. The file format on disk has a `.gb` extension. Wiz will automatically fill in the necessary fields to get a basic booting Game Boy ROM. This includes the Nintendo logo image, and the checksum, necessary to get past the boot screen of the Game Boy.

Format Specifics:

- 0x100 .. 0x150 in the first bank of ROM will be reserved for the Game Boy header.
- A checksum is automatically calculated and inserted into the header.
- The program ROM is automatically padded to nearest size permitted by the Game Boy header format.

Config options:

- `cart_type` (string) - the type of cartridge that the cart is using. It can be one of the following strings:

```
"rom"
"mbc1"
"mbc1-ram"
"mbc1-ram-battery"
"mbc2"
"mbc2-battery"
"rom-ram"
"rom-ram-battery"
"mmm01"
"mmm01-ram"
"mmm01-ram-battery"
"mbc3-timer-battery"
"mbc3-timer-ram-battery"
"mbc3"
"mbc3-ram"
"mbc3-ram-battery"
"mbc4"
"mbc4-ram"
"mbc4-ram-battery"
"mbc5"
"mbc5-ram"
"mbc5-ram-battery"
"mbc5-rumble"
"mbc5-rumble-ram"
"mbc5-rumble-ram-battery"
"mmm01"
"camera"
"tama5"
"huc3"
"huc1"
```

- `cart_type_id` (integer) - the type of cartridge mapper that the cart is using, as a numeric ID used by the Game Boy header format. Takes precedence over `cart_type_id`, so use this if `cart_type` is not recognized. (If a cart type name is missing, feel free to request it!)
- If neither `cart_type` nor `cart_type_id` are specified, then the ROM has no memory bank controller, and it is limited to a view of 32K of ROM at addresses 0x0000 .. 0x7FFF in memory.
- `title` (string) - The name of the program. The max length is 11 if there is `manufacturer` code, and 15 on legacy carts that do not have one. (Defaults to the output filename with extension removed, truncated to the maximum length requirement of the title)
- `gbc_compatible` (bool) - Whether or not this cart is compatible with Game Boy Color features. If `true`, the cart will boot with Game Boy Color functionality when played on a Game Boy Color compatible system. A GBC-compatible game can still be played on an original DMG-compatible model of Game Boy. (Defaults to `false`)
- `gbc_exclusive` (bool) - Whether or not this cart is exclusive to the Game Boy Color. If `true`, the cart will boot with Game Boy Color functionality when played on a Game Boy Color compatible system. Takes precedence over `gbc_compatible` if it is `true`. A GBC-exclusive game should not be played on an original DMG-compatible model of Game Boy. However, it is still up to the program to detect if it is being played on a Game Boy Color system, and stop if an incompatible system is used. (Defaults to `false`)
- `sgb_compatible` (bool) - Whether or not this cart is compatible with Super Game Boy features. If `true`, the cart is able to use extra functionality on a Super Game Boy, allowing communication with the Super Game Boy program through the Game Boy joypad port. Most emulators will expect a SGB border to be uploaded if this flag is enabled. (Defaults to `false`)
- `ram_size` (integer) - The amount of on-cartridge RAM that this cart has available. (Defaults to 0) Must be a size that is supported by the Game Boy header format. Possible values include:

```
0
2048
8192
32768
65536
131072
```

- `international` (bool) - Whether or not this game is an international release outside of Japan. (Defaults to `false`)
- `licensee` (string) - A two-character string representing the licensee / creator of the game. (Defaults to `""`)
- `old_licensee` (integer) - A legacy licensee code indicating the ID of the licensee / creator of this game. All newer releases have a legacy licensee code of 0x33 and will not use this field. (defaults to 0x33)
- `version` (integer) - The version of the cart being released, which can be used to tell apart later revisions. (Defaults to 0)

### Sega Master System / Game Gear

The Sega Master System and Game Gear are very similar game systems, and both share the same header format. Sega Master System ROMs end in an `.sms` extension. Game Gear ROMs end in a `.gg` extension.

Format specifics:

- Depending on the size of the cart, there is a header at either 0x1FF0 (8 KiB ROMs), 0x3FF0 (16 KiB ROMs), or 0x7FF0 (32 KiB ROMs).
- The program ROM is automatically padded to either 8 KiB, 16 KiB, or 32 KiB if it is not aligned to one of these sizes.
- A checksum is automatically calculated and inserted into the header.

Config Options:

- `product_code` (integer) - a number between 0 and 159999 indicating the product code of this cart. (Defaults to 0)
- `version` (integer) - a version number for this cart release, between 0 and 15. (Defaults to 0)
- `region` - The region that this cart is being released for. (Defaults to `"international"`)

```
"japan"
"export"
"international"
```

### Super Nintendo Entertainment System / Super Famicom

The SNES has a common header format found at fixed location in the ROM. The preferred format for SNES ROMs nowadays is `.sfc`, which contains no redundant copier headers to speak of, only the necessary headers to run the game.

There is additionally `.smc`, a common format popularized by the Super Magicom copier. Wiz will add an extra 512 bytes to work as this SMC format.

When possible prefer `.sfc`, but `.smc` is here for those who want headered ROMs.

Format Specifics:

- Depending on the map mode of the cart, there is a SNES header at either 0x7F00 (lorom) or 0xFF00 (hirom).
- A checksum is automatically calculated and inserted into the header.
- The program ROM is automatically padded to at least 128 KiB, and then to the nearest power of two from that.
- For now, you need to import the ROM before you can use it with higan or older versions of bsnes, but maybe down the road there will be a direct export to game folders.

Config options:

- `map_mode` (string) - The type of memory mapping mode that this cart uses (Defaults to `"lorom"`)

```
"lorom"
"hirom"
"sa1"
"sdd1"
"exhirom"
"spc7110"
```

- `fastrom` (bool) - Whether or not this cart uses fast ROM, which . (Defaults to `false`)
- `expansion_type` (string) - The expansion hardware type that this cart uses. (Defaults to `"none"`)

```
"none"
"dsp"
"super-fx"
"obc1"
"sa1"
"other"
"custom"
```

- `maker_code` (string) - two character code used to designate the licensee / creator of the game. (Defaults to `""`)
- `game_code` (string) - four character code used to designate this game. (Defaults to `""`)
- `region` (string) - the region of the world that this is being released for. This region also indicates NTSC/PAL of the game. (Defaults to `"japanese"` NTSC game)

```
"ntsc"
"pal"
"japanese"
"american"
"european"
"scandinavian"
"french"
"dutch"
"spanish"
"german"
"italian"
"chinese"
"korean"
"canadian"
"brazilian"
"australian"
```

- `ram_size` (integer) - the size in bytes of on-cartridge RAM that this cart uses. If non-zero, the size must be power of two and greater than 4096 bytes. (Defaults to 0)
- `expansion_ram_size` (integer) - the size in bytes of on-cartridge RAM that the expansion uses. If non-zero, the size must be power of two and greater than 4096 bytes. (Defaults to 0)
- `battery` (bool) - whether or not this cart uses battery backed save RAM. (Defaults to `false`)
- `special_version` (integer) - the special version of this cart. (Defaults to 0)
- `cart_subtype` (integer) - the sub-type identifier to use for this cart. (Defaults to 0)
- `title` (string) - The title of the cart, as a 21-character string. (Defaults to `""`)

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

However, the try-in-the-browser code contains some parts (emulator code) that are GPL licensed, and if the emulator is included, all code there becomes subject to the GPL. All other code is MIT.

Areas for Improvement
---------------------

- Better documentation.
- Better code organization.
- Better error reporting.
- More examples.
- More libraries for hardware register defines.
- Automated test suite (unit tests, functional tests, etc).
- Improve instruction pattern matching - make data-driven / externally configurable if it can be done with low runtime cost.
- More platform support.
- More ROM header/format support.
- Improve linker phase - allow overlays on top of existing programs, better error-checking, debug symbol table dumps and source maps for various emulators.
- Better macros.
- Tuples.
- More type-safety / high-level syntax.
- More mechanisms for argument-passing (besides designated storage / registers).
- Opt-in optimization attributes.
- Attributes for assuming relocatable page registers (eg. direct page register on 65816, bank registers)
- Embed multiple languages in a single compile instruction.
- Make modules nicer to use.
- Syntax highlighting and language extensions for code editors.
- Improving build performance.
- Better try-in-browser sandbox.

Other Notes
-----------

- This is alpha software. It should be usable but contains many rough edges. Future revisions may change and break compatibility, but this will be documented whenever possible.
- Better documentation is coming soon. I am very tired and I need to rest a bit.
- This project has existed in other forms before this release. For the old version of Wiz that was written in the D programming language, see the `release/wiz-d-legacy` branch of this repo.
- For the even older esoteric language project that turned into Wiz, see: http://github.com/Bananattack/nel-d
- While this project has been in development for a long time, this release is considered an "early development" build. It may contain issues and instabilities, or parts of the language that are subject to change. Help improving this is appreciated.