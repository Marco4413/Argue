# Argue

A small, **header-only**, **exception-less** command line arguments parsing library
for **C++20** which **doesn't auto-log to `std::cout`**.

> I tried to search a good header-only arguments parsing library,
> but every library I have found didn't give me much control over
> its logging. Moreover, people really like C++ exceptions while
> I'm here hating them.

**Old C++20 compilers may also work**,
see [Tested Compilers](#tested-compilers)

## Copying

You're allowed to copy the `argue.hpp` file as long as you follow
its LICENSE. You don't really need to credit me in your source
code because that file contains a copy of the LICENSE.
However, I'd appreciate being mentioned.

## Building

You'll need a C++ compiler which supports some C++20 features.

See [Tested Compilers](#tested-compilers)

The easiest way to build this library in your project is to include
`argue.hpp` where needed and add `argue.cpp` to your build.

However, if you check `argue.cpp` out, the implementation is defined
inside `argue.hpp` locked behind an `#ifdef ARGUE_IMPLEMENTATION`.
So you can implement this library within one of your project's `.cpp`
files with the following snippet of code:

```c++
#define ARGUE_IMPLEMENTATION
#include "argue.hpp"
```

### Tested Compilers

This library is built and tested on `gcc` and `clang` with the flags
found in `cxxflags.txt`.

- `clang` 10.0.0
- `gcc` 10.5.0
- `msvc` vs2022 (with `/W4` `/WX` flags)

This library does not require any C++20 fancy features, mainly methods
that were added to `std::string_view`s.

### Building Examples on Linux

You can use the `build_example.sh` script to build an example:

```sh
$ ./build_example.sh examples/00-helloworld.cpp
$ ./main
```

**MAKE SURE TO TAKE A LOOK AT THE SCRIPT, DO NOT RUN SCRIPTS YOU DON'T TRUST**

It's also very small, so it doesn't hurt.
