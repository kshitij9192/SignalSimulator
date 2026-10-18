# SignalSimulator

SignalSimulator is a C++17 project built with CMake.

## Requirements

- CMake 3.24 or newer
- A C++17-compatible compiler
- Network access during configuration if `nlohmann_json` 3.12.0 or `Catch2` 3.16.1 (when `BUILD_TESTING=ON`) are not installed locally; CMake fetches them automatically

## Build Configurations

### 1. Default Build (Simulator + Tests)

By default (`BUILD_TESTING=ON` and `SIGNAL_SIMULATOR_ENABLE_CLANG_TIDY=OFF`), CMake configures the simulator library, main executable, and Catch2 test suite:

```sh
cmake -S . -B build
cmake --build build --parallel
```

### 2. Debug and Release Builds

Specify `CMAKE_BUILD_TYPE` to configure debug symbols or release optimizations:

```sh
# Debug build
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --parallel

# Release build
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
```

### 3. Simulator-Only Build (Without Tests)

Disable `BUILD_TESTING` to skip `Catch2` and test target compilation:

```sh
cmake -S . -B build-notest -DBUILD_TESTING=OFF
cmake --build build-notest --parallel
```

### 4. Clang-Tidy Naming & Lint Build

To enable `clang-tidy` checks on the production targets (`signal_simulator_config` and `signal_simulator`) without affecting third-party dependencies or test targets, install a `clang-tidy` build that supports query-based custom checks (`--experimental-custom-checks`) and run:

```sh
cmake -S . -B build-lint -DSIGNAL_SIMULATOR_ENABLE_CLANG_TIDY=ON
cmake --build build-lint --parallel
```

## Building Individual Targets and Running Tests

### Run Unit / Behavioral Tests

Build the test executable and run tests via CTest (or execute the Catch2 binary directly):

```sh
cmake --build build --target signal_simulator_tests --parallel
ctest --test-dir build --output-on-failure
```

### Run Formatting Checks

If `clang-format` is installed, building `signal_simulator` automatically runs `format-check`. To run the formatting check standalone:

```sh
cmake --build build --target format-check
```

### Build Specific Production Targets

```sh
# Configuration parser static library only
cmake --build build --target signal_simulator_config --parallel

# Main simulator executable
cmake --build build --target signal_simulator --parallel
```