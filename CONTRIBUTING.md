# Naming Checks

The project enforces C++ identifier naming with clang-tidy. Install a clang-tidy
build that supports query-based custom checks, then configure and build with:

```sh
cmake -S . -B build -DSIGNAL_SIMULATOR_ENABLE_CLANG_TIDY=ON
cmake --build build
```

The checks enforce `m_` plus PascalCase for members, PascalCase for local
variables, `p` plus PascalCase for pointer variables, and `r` plus PascalCase
for reference variables. Member pointers and references use `m_p` and `m_r`.
Lint findings fail the build.

# Formatting Checks

C++ uses Allman brace placement: opening braces start on their own line. Install
clang-format, then run the formatting gate with:

```sh
cmake --build build --target format-check
```

The target fails if clang-format is unavailable or a listed C++ file does not
match `.clang-format`.
