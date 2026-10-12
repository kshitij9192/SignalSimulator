# SignalSimulator

SignalSimulator is a C++17 project built with CMake.

## Requirements

- CMake 3.24 or newer
- A C++17-compatible compiler
- Network access during configuration if `nlohmann_json` 3.12.0 is not installed locally; CMake fetches it automatically

## Configure and Build

From the repository root, run:

```sh
cmake -S . -B build
cmake --build build --parallel
```

## Optional Checks

If `clang-format` is installed, normal builds include the formatting check. To
run it directly:

```sh
cmake --build build --target format-check
```

To enable clang-tidy naming checks, install a clang-tidy build that supports
query-based custom checks and configure a separate build directory:

```sh
cmake -S . -B build-lint -DSIGNAL_SIMULATOR_ENABLE_CLANG_TIDY=ON
cmake --build build-lint --parallel
```