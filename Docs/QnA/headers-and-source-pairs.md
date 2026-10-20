# Why do headers exist, and why are `.hpp`/`.cpp` or `.h`/`.mm` paired?

**Short answer:** C++ compiles each source file on its own, not as one big
program. A header is the shared contract that every source file copies in; the
`.cpp` or `.mm` supplies the implementation behind that contract. `Bunker.hpp`
and `Bunker.cpp` are the C++ example. `AppDelegate.h` and `AppDelegate.mm` are
the Objective-C++ example.

If you already know what a header is and want the preprocessor mechanics, read
[What is a `.hpp` file?](hpp-files.md) first. This entry covers why the split
exists, what each half is responsible for, and why the extensions differ.

## The compiler sees one translation unit at a time

When the compiler builds `Game.cpp`, it does not see `Bunker.cpp` or
`AppDelegate.mm`. It sees `Game.cpp` plus everything that `Game.cpp` includes,
after the preprocessor has pasted those files together. That single expanded
file is a **translation unit**. Each translation unit is compiled separately,
and the linker joins the resulting object files afterwards.

That model creates a problem. `Game.cpp` calls `Bunker::carve`, but the compiler
has never seen the body of `carve`. It still has to know that the function
exists, what arguments it takes, and what it returns. A **declaration** provides
exactly that:

```cpp
auto carve(Vec2 center, float radius) noexcept -> bool;
```

The compiler checks the call against the declaration and emits a reference to
the symbol. The **definition** — the version with the `{ ... }` body — lives in
`Bunker.cpp` and is compiled once into an object file. The linker connects the
reference to that object file. No header is present at runtime; it disappears
after preprocessing.

## Declaration versus definition

| Term | What it does | Example |
| --- | --- | --- |
| Declaration | Introduces a name and its type or signature. Says "this exists somewhere". | `auto makeBunkers() -> std::array<Bunker, 4>;` |
| Definition | Supplies the body or storage. Says "this is it". | The `makeBunkers` body in `Bunker.cpp` |
| Translation unit | One `.cpp`/`.mm` after all its includes are expanded. | `Bunker.cpp` with `Bunker.hpp` pasted in |
| Linker | Matches references from one object file to definitions in another. | Resolves `Game.cpp`'s call to `Bunker::carve` |

A function may be declared many times, but a non-inline definition may appear
only once in the whole program. That is the **one-definition rule**, abbreviated
ODR. Class definitions are the exception: the same class definition may appear
in many translation units as long as every copy is identical. That is why a
class is normally written once in a header and included everywhere it is needed.

## Why not put everything in one file?

You could paste every declaration into every `.cpp` by hand, but then a signature
change has to be made in many places, and the copies eventually disagree. A
header is a single authoritative copy.

You could also put every function body in the header. That works for tiny
header-only utilities, but it has costs:

- every includer re-parses the body, so compile times grow;
- every includer can see implementation details that should be private;
- changing one private helper recompiles every file that includes the header.

Keeping implementations in `.cpp` files means code that only uses the interface
does not recompile when a private body changes. The header is the boundary
between "what callers may rely on" and "how it happens to work today".

The trade-off is that the header is re-read by every includer. That is why this
project keeps headers small and stable: `Bunker.hpp` exposes a compact class,
while the shape and carving algorithms live in `Bunker.cpp`.

## What `#pragma once` and `#import` do

A header can be reached through more than one include path in a single
translation unit. Without protection, the class would be defined twice and the
compiler would reject it. [Bunker.hpp](../../Sources/Core/include/invaders/Bunker.hpp) — `Bunker`
starts with `#pragma once`, which tells the compiler to include the file at most
once per translation unit. It is not ISO C++ but is supported by Clang and every
compiler this project targets. The classic portable alternative is an
`#ifndef`/`#define`/`#endif` guard.

Objective-C headers can use `#import`, an Objective-C extension that also
includes a file only once. [`AppDelegate.h`](../../Sources/App/AppDelegate.h) —
`SIAppDelegate` uses both `#pragma once` and `#import <AppKit/AppKit.h>`; the
extra guard is harmless and makes the intent obvious to both languages.

## Pairing 1: `Bunker.hpp` and `Bunker.cpp`

[`Bunker.hpp`](../../Sources/Core/include/invaders/Bunker.hpp) — `Bunker` is the
public contract:

- the class definition, including its constants, constructors, and private data;
- **inline getters** such as `origin`, `cells`, and `remainingCells`, whose
  bodies are written inside the class;
- **declarations only** for the expensive operations: `isSolid`, `cellRect`,
  `hitTest`, `hitPoint`, and `carve`;
- the declaration of the free function `makeBunkers`.

[`Bunker.cpp`](../../Sources/Core/Bunker.cpp) — `Bunker::carve` is the
implementation side:

- it includes its own header first, so the compiler compares each definition
  against the declaration that callers see;
- it defines `Bunker`'s member functions and `makeBunkers`;
- its private helper `isShapeCell` lives in an anonymous namespace, so it is
  invisible outside this translation unit;
- its `<algorithm>` and `<cmath>` includes are implementation details; they do
  not leak into every file that includes `Bunker.hpp`.

The getters are inline because their bodies are trivial and callers need them
available to compile. The larger operations are out of line because their bodies
are not part of the interface. If you edit only `Bunker.cpp`, the compiler
rebuilds that file and the linker relinks the program; unrelated files do not
need to be recompiled.

There is also a hard language reason the private members appear in the header:
[`Game.hpp`](../../Sources/Core/include/invaders/Game.hpp) — `Game` stores
`std::array<Bunker, 4>` **by value**, so the compiler must know `sizeof(Bunker)`
and therefore the complete layout. A forward declaration such as
`class Bunker;` would be enough for a pointer or reference, but not for a member
stored by value.

## Pairing 2: `AppDelegate.h` and `AppDelegate.mm`

This is the same declaration/definition split in Objective-C++, with different
file extensions.

[`AppDelegate.h`](../../Sources/App/AppDelegate.h) — `SIAppDelegate` declares the
class:

```objc
@interface SIAppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end
```

It says that a class named `SIAppDelegate` exists, inherits from `NSObject`, and
conforms to two protocols. It contains no method bodies. That is enough for
[`main.mm`](../../Sources/App/main.mm) — `main` to write:

```objc
SIAppDelegate* delegate = [[SIAppDelegate alloc] init];
```

Without the interface declaration, `main` would not know the class exists.

[`AppDelegate.mm`](../../Sources/App/AppDelegate.mm) — `applicationDidFinishLaunching`
contains the `@implementation`. It holds the private `_window` and `_gameView`
instance variables in a class extension, builds the menu and window, and
implements the delegate callbacks. Those details stay out of the header, so
`main.mm` cannot accidentally depend on them.

Why `.h` rather than `.hpp`, and `.mm` rather than `.m`?

- `.h` is this repository's convention for an Objective-C or bridging header. It
  contains `@interface` and imports Apple headers, which are not C++.
- `.hpp` is the convention for a pure C++ header such as `Bunker.hpp`.
- `.mm` is **Objective-C++**: Objective-C syntax plus C++ syntax in one file.
  `AppDelegate.mm` uses `<cmath>` and `std::floor` to size the window, so a plain
  Objective-C `.m` file would not compile.
- The app target is built as Objective-C++ because it links the C++ core and
  renderer, so `.mm` is the honest extension for every implementation file at
  that boundary.

[`MetalRenderer.h`](../../Sources/Render/MetalRenderer.h) — `SIRenderer` shows a
mixed case: C++ forward declarations are guarded by `#ifdef __cplusplus`, while
the Objective-C `@interface` is visible to both languages. It is a bridging
header imported from `.mm` files, not a pure C++ header.

## What about headers with no `.cpp` partner?

Not every header needs an implementation file.

- [`Rng.hpp`](../../Sources/Core/include/invaders/Rng.hpp) — `SplitMix64` defines
  all of its small methods inside the class. Member functions defined inside a
  class are implicitly `inline`, so each translation unit can compile its own
  copy without violating the one-definition rule.
- [`Types.hpp`](../../Sources/Core/include/invaders/Types.hpp) — `Vec2` holds
  only type definitions and `inline constexpr` constants. It needs no object
  file; the header is the whole component.
- [`MetalShaders.hpp`](../../Sources/Render/MetalShaders.hpp) —
  `kMetalShaderSource` is one `inline constexpr` string holding the MSL program.
  It is data, not behaviour, so there is nothing to put in a `.cpp`.

These are deliberately small. A header-only component with a large
implementation would make every includer pay for it at compile time.

## How this repository keeps the split honest

The style rule is "include what you use":

- if a file names `std::optional`, it includes `<optional>` itself rather than
  relying on another header that happens to include it;
- if a header is self-contained, any translation unit can include it first and
  still compile;
- a header's own includes are part of its contract, not an implementation
  detail.

The build now enforces this. [`HeaderChecks.cmake`](../../cmake/HeaderChecks.cmake)
— `space_invaders_add_header_checks` compiles each portable header as its own
translation unit. If `Bunker.hpp` started using `std::vector` without including
`<vector>`, the normal build would fail even though some other file might have
provided `<vector>` first. The check catches a missing direct include, not an
unused one; clangd's `unused-includes` hint is a separate editor-only signal.

The format, build, simulation, and documentation gates are described in
[CONTRIBUTING.md](../../CONTRIBUTING.md) and
[Docs/CPP_DESIGN.md](../CPP_DESIGN.md). The short version is: headers are the
compile-time contract, `.cpp`/`.mm` files are the definitions, and the build
checks both sides of that contract.

## Newbie glossary

| Term | Meaning |
| --- | --- |
| Header | A file included into other files; usually declarations and types. |
| Source file | A `.cpp` or `.mm` compiled into an object file. |
| Translation unit | One source file after includes are expanded. |
| Declaration | Introduces a name, type, or signature; no body or storage. |
| Definition | Supplies the body or storage for a declaration. |
| Inline | Allows a function or variable definition to appear in multiple translation units; also a hint to the optimiser. |
| ODR | One-definition rule: a non-inline definition appears exactly once in the program. |
| Linker | Joins object files and resolves references to definitions. |
| Objective-C++ | A `.mm` file that can contain both Objective-C and C++. |

Return to the [Q&A index](README.md) or the
[project README](../../README.md).
