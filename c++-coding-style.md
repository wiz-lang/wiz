# Wiz C++ Coding Style Guide

## Formatting

- Four spaces for indentation, no tabs allowed.
- Put the opening brace `{` on the same line as the associated control statement or declaration.
- Put spaces between control structure keywords and their parenthesized expression. eg. `if (expr)`
- Almost always prefer to use a brace-delimited block as the body of a control structure, rather than a single statement.
- Spaces before and after binary operators, except for `.` or `->` or `::`
- No space between an indexable term and the brackets `[` ... `]` for an indexing expression.
- No space between a callable term and the parenthesis `(` ... `)` for a function call
- No space between a unary operator and its term.
- No superfluous padding spaces on the inside of parenthesized/bracketed expressions.
- Add parentheses to expressions when the precedence is not immediately clear (bitwise operations, more complicated mixed-logical operations), but leave them off when it is clear (eg. comparisons, basic math)
- For binary operator expressions that span multiple lines, put the operator on the next line.
- Put consecutive closing parens for nested expressions together on the same line preferrably, and on the same line as the last expression in parentheses.
- Constructors can be weird to indent, see existing examples of how constructors and their member initializer lists should be indented.
- When in doubt, look at existing code for examples and try to match the formatting.

## C++ Usage

- C++ is not great, but C is even worse, in my opinion. I wanted a language that's easy enough to compile cross-platform (including to the Web), has mature compilers, and good static type safety and explicit memory management (no GC requirement). Rust is really nice but not quite there. D was a fun language to write earlier versions of Wiz, but C++14 does most of what I wanted from D but without requiring GC, and with better portability. Haskell is a great language that I love, but it fails in a lot of ways for writing software tools, so I stuck with C++.
- Don't use `#pragma once` directives. Use the annoying long-winded way with `#ifndef MODULE_H` ... `#define MODULE_H` ... `#endif` include guards instead. 
- Don't use C-style casts. Always use `static_cast` or `reinterpret_cast` instead.
- Don't cast away `const` with `const_cast` - rewrite the code to respect the `const` in place, or make the field mutable.
- If a local variable has an initializer or easily deduced type, use `auto`
- If a local variable is bound once and never changed after binding, use `const auto`
- Prefer named locals over needing to comment complex expressions.
- Range-based for is nice, use `auto`/`auto&` or `const auto`/`const auto&` for the iterator variable.
- Prefer `const` versions of `auto` whenever possible.
- If possible, prefer references `&` to pointers `*`, and make them `const` when possible.
- Prefer `Optional<T>` over pointers or `bool` + out-params for nullable value types.
- Prefer `Variant<T, T2, T3, ...>` over subclasses for simple structs that hold data but don't have behaviour.
- If a function does not need to mutate a reference parameter, take it by `const`
- Do not use raw `new` or `malloc` -- prefer `std::unique_ptr` to manage memory, and `std:move` to explicitly transfer ownership.
- If needing a `std::unique_ptr` to an incomplete type, use `FwdUniquePtr`, and implement a `FwdDeleter` for the type in its .cpp file.
- If passing a pointer to some data in a non-owning fashion, use a raw C-style pointer.
- If a function does not need to mutate a pointer, take it by `const`
- If a member function of a class does not modify `this`, it should be marked `const`
- Don't use `std::shared_ptr`
- Don't use `dynamic_cast` and `typeid` expressions or other RAII stuff.
- Prefer `StringView` to `const char*` or `const std::string&` since it handles both.
- Use `std::string` sparingly. Prefer to pass around `StringView` until you need to copy or mutate a string.
- Use `StringPool` to keep a long-lived `StringView` of a character string.
- If possible, pass dynamically-allocated data structures around by reference (const if possible) rather than copy.
- Don't let the unspecified order of argument evaluation for function calls bite you by accidentally relying on specific order to side-effects in the call, eg. `std::move`
- For C functions, use `<cstdio>`, `<cstring>`, etc instead of `<stdio.h>`, `<stddef.h>`, etc, and prefix C identifiers with `std::` unless they're macros.
- Avoid using C functions when there is something better/safer in the C++ libraries.
- Don't use file-scoped `static` functions, use unnamed namespaces instead.
- The codebase targets C++14. C++17-specifics should currently be avoided except for working around valid C++14 code that got deprecated in C++17.
- C++14 doesn't support nice `std::is_same_v<A, B>` etc and there's no support for `inline` variables until C++17, so use `std::is_same<A, B>::value` etc instead.
- C++14 doesn't support `std::invoke_result_t`, but C++17 doesn't support `std::result_of_t`, so be prepared to work around that.
- Avoid implementation inheritance. Pure-`virtual` interface classes are ok.
- Use `override` whenever providing an implementation of a `virtual` method.
- A `class` with only public members should be a `struct`
- Avoid cluttering headers with `#include` directives that aren't needed so that compile-times stay fast(er). If possible, use forward-declared / incomplete types in the .h, and move the `#include` into .cpp.
- Avoid `#define` macros except for very basic conditional compilation stuff, no code-insertion or constants.
- Prefer `enum class` to `enum`.
- Don't mixed signed and unsigned types without casting.
- Avoid implicit casts (except to `bool` which are okay sometimes)

## AST

- The AST was designed to be immutable to simplify design and avoid programmer error, and uses unique pointers to ensure that memory ownership is clearly retained. All nodes should be declared `FwdUniquePtr<const T>`. To have a non-owning reference to a node, use a `const T*` variable to hold it, and use `get()` on a unique pointer.
- There are `clone()` functions on the node types to provide an identical (recursive) copy when needed. It can be expensive depending on where it is done, so it's recommended to apply to the smallest section that needs copying.