# Implementation Choices

## JSON Configuration Parser

- **Decision:** Use [nlohmann/json](https://github.com/nlohmann/json) to parse the simulator's JSON configuration in C++.
- **Rationale:** It is a mature, widely used C++ JSON library with a straightforward API. Its header-only distribution keeps integration small for this host-based simulator.
- **Integration:** Use the `nlohmann_json` CMake package and link the `nlohmann_json::nlohmann_json` target. CMake prefers an installed package and otherwise fetches the pinned 3.12.0 release archive with SHA-256 verification.
- **Responsibilities:** Use the library to parse JSON syntax and access typed values. Reject parse errors and report useful diagnostics to standard error before initializing shared memory.
- **Validation boundary:** Parsing alone does not enforce the configuration schema or cross-field constraints. Validate required fields, value ranges, and relationships (such as ADC headroom, channel IDs, and disturbance targets) before runtime initialization. The JSON Schema in [signal_simulator.schema.json](signal_simulator.schema.json) documents editor-facing structure and options; runtime schema validation, if desired, requires a separate JSON Schema validator.

## Build System

- **Decision:** Use CMake to describe and configure the C++ project. Use Ninja as the default build backend on Linux.
- **Distinction:** CMake generates a native build system; Ninja performs the build. CMake can also generate Unix Makefiles, so GNU Make remains available as a backend without changing the project description.
- **Rationale:** CMake's target-based model fits the simulator's expected executable, core components, and tests. It also provides a direct way to declare the `nlohmann_json::nlohmann_json` dependency and includes CTest for test registration and execution.
- **Typical workflow:**

    ```sh
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake --build build
    ctest --test-dir build --output-on-failure
    ```

- **Alternatives:** A hand-written GNU Makefile is reasonable for a tiny, Linux-only build, but its rules and dependency organization would be maintained directly in the Makefile. Consider Bazel if hermetic/reproducible builds, remote caching, or a substantially larger multi-repository project become requirements; its additional workspace and rule setup is not justified by the current scope.
- **References:** [CMake generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html), [CMake buildsystem model](https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html), [CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html), and [Bazel overview](https://bazel.build/about/intro).

## Signal Capture And Visualization

- **Decision:** Use a separate C++ integration-test capture consumer to read complete blocks from shared memory and write a test-only CSV dump. Use a Python tool with NumPy and Matplotlib to inspect the captured signals.
- **Data flow:** The simulator continues to publish only through shared memory. The capture consumer attaches as the SPSC consumer, validates the global and per-slot headers, copies each complete payload, checks sequence/timestamp continuity and dropped-block counts, and then writes decoded sample rows for plotting.
- **Capture contents:** Include block sequence, virtual timestamp, sample index, and one column per configured channel. Record the sampling rate, representation, layout, and ADC parameters with the capture so samples can be interpreted correctly. Preserve ADC output as integer counts; write floating-point values with sufficient precision to round-trip.
- **Verification boundary:** C++ integration tests remain authoritative for byte layout, quantization, slot sequencing, and synchronization. Plots help inspect waveform shape, phase, amplitude, clipping, and disturbance windows, but do not replace numeric assertions.
- **Performance caveat:** CSV writing adds I/O and can slow the consumer. Do not use file-writing captures to claim real-time throughput or jitter performance; run those checks without file output or with a bounded in-memory capture.
- **Scope:** This is test/diagnostic tooling, not a file-based simulator egress mode, so the initial release remains shared-memory-only.
