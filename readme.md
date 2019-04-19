wiz
===

**by Andrew G. Crowell**

Wiz is a high-level assembly language for writing homebrew software for retro console platforms.

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
- `prgdata` is ROM used for program code and constants.
- `constdata` is ROM used for constants and program code.
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

### Expression Statement

An expression statement is some statement that evaluates an expresssion that contains side-effects. It is terminated by a `;`. Assingment statements and call statements are both forms of expression statement.

### Assignment Statement

An assignment statement is kind of expression statement that can be used to store a value somewhere. Depending on where the value is stored, it can be retrieved again later.

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

After constant expression folding, any remaining run-time operations must have instructions available for them.

For an assignment containing a binary expression like `a = b + c;`, the instruction selection system will first look for an exact instruction of form `a = b + c;`. Failing that, it will attempt decomposing the assignment `a = b + c` into the form `a = b; a = a + c;`.


Assignments containing run-time operations cannot require an implicit temporary expression, or it is an error. All registers.

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

Assignments can be chained together if they're compatible.

Example:

```
y = a = x;

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

A common-to-find flag is the `zero` flag. If an instruction affects the zero flag, it might be used to indicate whether the result of the last operation was equal to zero. This can avoid the need to do some .

Example:

```
x = 10;
do {
    x--;
} while !zero;
```

A `carry` flag is a common flag to be affected. The interpretation of a `carry` flag is dependent on the system. There are operations like add-with-carry `+#`, subtract-with-carry `-#`, rotate-left-with-carry `<<<<#`, and rotate-right-with-carry `>>>>#` which depend upon the last state of the `carry`. These operations can be useful for extending arithmetic operations to work on larger numeric values.

Example:

```
// add 0x0134 to bc.
c = a = c + 0x34;
b = a = b +# 0x01;
```

Some CPUs allow the `carry` to be set or cleared by assigning to it.


### Call Statement

A call statement is a kind of expression statement that invokes a function. If the function being called produces a return value, the value is discarded. If the function call returns, the code following it will execute. Intrinsic instructions can also be called in the same manner.

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

`start .. end` is an inclusive range. If the loop terminates normally, then after the loop, the counter provided will have the first value that is outside of the sequence.

Example:

```
for x in 0 .. 31 {
    beep();
}
```

### Return Statements

A `return` statement is used to return from the currently executing function. If used in a function that has no return type, it cannot have a return value. If used in a function that has a return type, the return value must be of the same type.

A `return` that exists outside of any function body is a low-level `return` instruction.

### Tail-Call Statements

A statement of form `return f();` is a tail-call statement. It gets subsituted for a `goto`, and avoids the overhead of subroutine call that would push a program counter to the stack. However, unlike a `goto`, a tail-call will evaluate arguments to a function, like a normal function call.

```
func call_subroutine(dest : u16 in hl) {
    goto *(dest as *func);
}
```

### Goto Statements

A `goto` is a low-level form of branching which . All higher level branching constructs are internally implement

```
goto destination;
goto destination if condition;
```

The destination of a `goto` can be a label, a function, or a function pointer expression. A `goto` always passes no arguments to its destination, so it should be used carefully. If a `goto` that passes arguments to a function is required, use a tail-call statement instead.

Example:

```
loop:
    goto loop;
```

### Break and Continue

Loop statements such as `while`, `do` ... `while`, `for` have two forms of branching statements in their blocks: `break` and `continue`.

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
```

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

    a ~ b // concetenation [T; M] ~ [T; N] -> [T; M + N]
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

Platform Specifics
------------------

The registers, addressing modes and operations all vary depending on the system.

TODO: write about each platform's specifics.

### MOS 6502

- Status: Supported.
- Documentation: TODO

### MOS 65C02

- Status: Supported.
- Documentation: TODO

### Rockwell 65C02

- Status: Supported.
- Documentation: TODO

### WDC 65C02

- Status: Supported.
- Documentation: TODO

### HuC 6280

- Status: Supported.
- Documentation: TODO

### WDC 65816

- Status: Supported. Missing some small improvements to make mixing register modes a bit easier, and catching runtime errors relating to addressing modes.
- Documentation: TODO

### SPC 700

- Status: Supported.
- Documentation: TODO

### Zilog Z80

- Status: Supported.
- Documentation: TODO

### Game Boy

- Status: Supported.
- Documentation: TODO


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

Contact
-------

- Please report issues in the GitHub issue tracker.
- Project Website: http://wiz-lang.org/
- Project Discord: https://discord.gg/BKnTg7N
- Project GitHub: https://github.com/wiz-lang/wiz
- My Twitter: https://twitter.com/eggboycolor
- My Email: eggboycolor AT gmail DOT com
- My Github: http://github.com/Bananattack