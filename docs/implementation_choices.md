# Implementation Choices

## JSON Configuration Parser

- **Decision:** Use [nlohmann/json](https://github.com/nlohmann/json) to parse the simulator's JSON configuration in C++.
- **Rationale:** It is a mature, widely used C++ JSON library with a straightforward API. Its header-only distribution keeps integration small for this host-based simulator.
- **Integration:** Use the `nlohmann_json` CMake package and link the `nlohmann_json::nlohmann_json` target. CMake prefers an installed package and otherwise fetches the pinned 3.12.0 release archive with SHA-256 verification.
- **Responsibilities:** Use the library to parse JSON syntax and access typed values. Reject parse errors and report useful diagnostics to standard error before initializing shared memory.
- **Validation boundary:** Parsing alone does not enforce the configuration schema or cross-field constraints. Validate required fields, value ranges, and relationships (such as ADC headroom and disturbance target channel indices) before runtime initialization. The JSON Schema in [signal_simulator.schema.json](signal_simulator.schema.json) documents editor-facing structure and options; runtime schema validation, if desired, requires a separate JSON Schema validator.

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

## Signal Synthesis, Quantization, And Block Cadence

- **Decision:** Decouple virtual sample time from host OS wall-clock time, synthesize analog waveforms using a continuous phase accumulator, apply disturbances in the analog signal domain prior to quantization, and emit full blocks of $N$ samples per wake-up paced by an absolute monotonic timer (`clock_nanosleep` with `CLOCK_MONOTONIC` and `TIMER_ABSTIME`).
- **Virtual sample time vs. wall-clock time:** Each sample index $n = 0, 1, 2, \dots$ corresponds to a deterministic virtual simulation timestamp $t_n = n \cdot T_s = n / f_s$. Host OS scheduler wake-up jitter affects only the wall-clock instant a block becomes visible in shared memory, never the mathematical sample values or timestamps written into the payload and slot header.
- **Continuous phase accumulation:** Rather than evaluating $\sin(2\pi f t_n)$ directly from elapsed time $t_n$—which introduces discontinuous phase jumps when frequency changes and loses floating-point precision over long runtimes—the generator maintains a wrapped base phase accumulator $\theta_n \in [0, 2\pi)$ updated per sample as:

    $$
    \theta_{n+1} = \left(\theta_n + 2\pi \cdot f_{\text{inst}}(t_n) \cdot T_s\right) \bmod 2\pi
    $$

    Each channel $i$ then evaluates $\sin(\theta_n + \phi_i)$, preserving phase continuity across frequency steps and ramps.
- **Analog disturbance composition before quantization:** Disturbances model physical analog events before the ADC input stage:
  1. Frequency distortion modifies $f_{\text{inst}}(t_n)$ when advancing $\theta_n$.
  2. Sag and swell scale the sinusoidal amplitude $A_{\text{nominal}}$ (leaving `offset` unchanged), and surge adds a transient offset:

      $$
      x_i(t_n) = \text{Offset} + \left(\text{scale}_i(t_n) \cdot A_{\text{nominal}} \cdot \sin(\theta_n + \phi_i)\right) + x_{\text{surge}, i}(t_n)
      $$

  3. The resulting analog value $x_i(t_n)$ is then either written directly as IEEE-754 `float32` or quantized to raw ADC integer counts ($8/12/16/24$-bit, unipolar or bipolar) with saturation/clamping at the configured $A_{\text{ref}}$ rails.
- **Block-rate pacing without cumulative drift:** Waking a Linux thread on every individual sample period (e.g., $156.25\ \mu\text{s}$ at $6.4\text{ kHz}$) incurs high context-switch overhead and scheduler jitter. Instead, the producer wakes once per block interval $T_{\text{block}} = N \cdot T_s$, synthesizes all $N$ samples for all configured channels into the next ring-buffer slot, updates the slot and global sequence counters, and signals the POSIX semaphore. Using absolute target deadlines ($\text{NextWakeTime}_k = t_{\text{start}} + k \cdot T_{\text{block}}$) with `TIMER_ABSTIME` ensures that single-cycle wake-up latency never accumulates into long-term frequency or cadence drift.

