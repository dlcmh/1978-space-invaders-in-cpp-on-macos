# Why do transitive includes matter, and how does this build catch them?

**Short answer:** C++ `#include` is copy-and-paste, so a name can arrive
without anyone writing the line that provides it. That is a **transitive
include**. This project handles it in three layers: a style rule ("include what
you use"), a build check that compiles every portable header on its own, and a
periodic audit for the one case the compiler cannot see.

If you come from Python, Go, Rust, Java, or Swift, none of this exists in the
same way. Those languages resolve imports per name or per module: `import math`
gives you `math` and nothing that `math` happened to import. C++ has no such
isolation, so the responsibility is placed on conventions and tooling instead.

## Why C++ is different

A Python module can do `from math import sqrt`, and `math`'s own imports never
leak into your namespace. Rust's `use` brings in exactly the item named. Java
and Go each have one file per package or directory, and the compiler checks the
whole unit.

C++ has no module boundary in the same sense. The preprocessor pastes the
text of an included file into the including file, at the point of the
`#include`. Whatever that text includes gets pasted too, recursively. The
compiler then sees one long document and has no idea which file any line came
from.

That gives you two failure modes:

| Failure | What it looks like |
| --- | --- |
| **Transitive reliance** | `Game.hpp` uses `std::size_t` but includes only `<array>`; it compiles because `<array>`'s own internals happen to pull in `<cstddef>`. |
| **Unused umbrella includes** | A header includes `<cstdint>` and never uses it. Harmless today, and a stale dependency on the standard library's internals tomorrow. |

Both are invisible in a normal build: the first is green, and the second is
green and says nothing.

## What goes wrong when transitivity drives your build

Transitive includes work until they don't. Common failure paths:

- **SDK or standard-library upgrade.** `<array>` on a new toolchain includes
  less, and a file that quietly relied on it stops compiling somewhere else.
- **A neighbour's cleanup.** Someone removes `<vector>` from a header because
  that header doesn't use it, and the file two levels down that borrowed it
  from the chain breaks — not in the clever commit, but in a later one.
- **Wrong-order includes.** A header compiles only when something else was
  included before it. The standalone header build then goes red for reasons
  that look unrelated.
- **Free-floating tooling.** clangd flags unused includes but not missing
  transitive ones, so "no squiggles" is not the same as "correct includes."

The classic analogue in a C header world is relying on `<stdio.h>` to get
`size_t` through `<stddef.h>`. It happens to work for a while, then stops.

## What this repository does about it

### Layer 1: the rule

Every file includes the standard header that declares each thing it names.
That line is in the style section of [CPP_DESIGN.md](../CPP_DESIGN.md) —
`const` and `constexpr`. In practice: if you type `std::optional`, that file
includes `<optional>`, even if another header already does.

The same rule applies to project headers, which matters more because they
change constantly. If `Scene.hpp` names `std::string_view` in its API, it
includes `<string_view>`, independent of what `PixelFont.hpp` and `Canvas.hpp`
bring in.

### Layer 2: the standalone header build

[HeaderChecks.cmake](../../cmake/HeaderChecks.cmake) —
`space_invaders_add_header_checks` generates, for each portable header, a
throwaway source file containing nothing but:

```cpp
#include <invaders/Game.hpp>
```

It compiles those with the project warning set and `-Werror`, as part of the
normal Debug and Release builds. There is no extra tool and no network step.

It is registered for the Core headers in
[CMakeLists.txt](../../Sources/Core/CMakeLists.txt) —
`spaceinvaders_core_header_checks` and for the Render headers in
[CMakeLists.txt](../../Sources/Render/CMakeLists.txt) —
`spaceinvaders_render_header_checks`. When you add a portable header, add it to
that list.

The effect: a header cannot depend on include order or on another header
supplying a name it uses. If `Game.hpp` uses `std::span` without including
`<span>`, the `Game.cpp` object file fails to compile, because nothing else has
run first.

### Layer 3: the audit for the gap

The standalone compile catches a transitive include only when the header
**actually uses** the missing name. It cannot flag a transitive include that is
currently merely unused — that is what clangd's `unused-includes` hint is for.

For that case the routine is:

- when clangd says `unused-includes`, treat it as a prompt to look at the
  header's whole include block;
- when you remove an include, build immediately. If another file breaks, fix it
  by adding the include where the name is used, not by reinstating the
  umbrella include;
- occasionally compare usage with includes across all portable headers:

```bash
for f in Sources/Core/include/invaders/*.hpp Sources/Render/*.hpp; do
  echo "== $f"
  echo "  uses    : $(grep -o 'std::[a-z0-9_]*' "$f" | sort -u | tr '\n' ' ')"
  echo "  includes: $(grep -o '<[a-z_]*>' "$f" | tr '\n' ' ')"
done
```

That is the check that found `Game.hpp` including `<cstdint>` for nothing and
[AudioEngine.hpp](../../Sources/Platform/AudioEngine.hpp) — `AudioEngine` using
`std::size_t` without `<cstddef>`.

## What about C++20 modules?

Modules are the language's real answer to this problem: an import boundary
declares exactly what is exported, so transitivity is controlled by the module
interface rather than by file layout. The project does not use them yet — the
build would need module-aware compilation rules and the toolchain support is
still uneven for a project this size. That decision is recorded in the
rejected-alternatives table of [CPP_DESIGN.md](../CPP_DESIGN.md) — Modules.

Until then, the three layers above are the substitute, and they are cheap: no
dependency, no daemon, no IDE requirement.

## Coming from a language with real imports

| Your habit | In C++, instead |
| --- | --- |
| `import math` gives exactly `math` | You get `math` **and everything nested inside it**. |
| Unused imports are obvious | An unused `#include` costs nothing at runtime and is only visible to linters. |
| The compiler maps errors to the file you edited | An error may point at a file you did not touch, because the name came from elsewhere. |
| Package managers guarantee versions | Your "dependencies" change with the SDK and the standard library implementation. |
| Refactor tools move symbols safely | Include hygiene you keep is yours alone; nothing in the language protects it. |

The mental shift is that includes are a **textual** mechanism and purity is a
**discipline**: the build can enforce the part that uses names, and the rest is
caught by review and the occasional audit.

## Glossary

| Term | Meaning |
| --- | --- |
| Include | A preprocessor directive that pastes another file in place. |
| Transitive include | A header you got indirectly, through another include. |
| Include hygiene | Keeping each file's includes to exactly what it uses. |
| Umbrella header | A header included to get many names, often a convenience aggregate. |
| Standalone header build | Compiling a header as the only content of a translation unit. |
| Module (C++20) | A sealed compiled unit with explicit exports; the real import boundary. |

Return to the [Q&A index](README.md) or the
[project README](../../README.md).
