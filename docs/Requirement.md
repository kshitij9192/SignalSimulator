# Requirement: SignalSimulator

## Document Control

- Topic: Multi-Channel ADC Signal Simulator
- Status: Ready
- Ready for issue decomposition: Yes
- Owner: Kshitij
- Last updated: 2026-10-11

## Title

Host-Based Configurable Multi-Channel ADC Signal Simulator

## Purpose

Provide embedded developers and test automation environments with a Linux-host-executable simulator capable of generating synthetic ADC-sampled sensor signals, allowing embedded control logic (e.g., protection relays, motor control, temperature controllers) to be tested without physical signal generator hardware.

## Scope

- Generation of sinusoidal signals for a configurable number of channels, including three-phase profiles with configurable inter-channel phase shifts.
- Dynamic waveform disturbance injection via deterministic scheduled event timelines (surges, sags/swells, and frequency distortions).
- Host platform execution on Linux.
- Configurable signal parameters across defined dimensions: value, time, frequency, phase, and quality.
- Output emission to a dedicated shared memory buffer.

## Non-Goals

- Physical hardware signal generation (DAC, physical signal injection hardware, GPIO/PWM output).
- File-based and pipe/FIFO-based egress channels in the initial release (scoped strictly to shared memory).
- Continuous analog signal impairments in the initial release: Gaussian white noise (AWGN/SNR modeling), integer harmonic distortion (THD), and ripple / DC offset drift.
- Closed-loop hardware-in-the-loop (HIL) plant simulation in the initial scope.
- Interactive graphical UI (GUI) in the initial release.
- Windows host execution and Windows-specific IPC in the initial release.

## Functional Requirements

- FR-001: The system shall generate synthetic signal waveforms simultaneously for the configured number of distinct channels.
- FR-002: The system shall support sinusoidal signal generation for all configured channels with configurable amplitude, fundamental frequency (e.g., 50 Hz, 60 Hz, or arbitrary positive frequency), offset, and per-channel phase displacement. A typical three-phase profile uses 0°, 120°, and 240°.
- FR-003: The configuration shall specify samples per cycle at the nominal frequency, and the system shall derive the sample rate from that value and the nominal frequency.
- FR-004: The system shall implement block/chunk-based emission mimicking ADC DMA ping-pong buffers, where the block size (number of samples per channel accumulated before triggering emission) is user-configurable.
- FR-005: The system shall support configurable channel memory layout within each emitted block:
  - Interleaved layout: Samples alternate sequentially by configured channel order for each sample index.
  - Planar layout: Samples are contiguous per channel in configured channel order within the block.
- FR-006: The system shall support configurable sample output representations:
  - Floating-point mode: Emitting IEEE-754 32-bit floating-point (`float`) values representing physical engineering units.
  - Raw ADC integer mode: Emitting quantized discrete integer counts derived from the simulated analog signal using configurable ADC parameters:
    - Reference amplitude $A_{\text{ref}}$ ($A_{\text{ref}} > 0$), defining the ADC hardware full-scale input boundary.
    - Polarity mode: `UNIPOLAR` (input dynamic range $[0, A_{\text{ref}}]$) or `BIPOLAR` (input dynamic range $[-A_{\text{ref}}, +A_{\text{ref}}]$).
    - Resolution / bit depth $B \in \{8, 12, 16, 24\}$ bits.
    - Dynamic Headroom / Nominal Scaling: The configuration shall define nominal waveform amplitude $A_{\text{nominal}}$ relative to $A_{\text{ref}}$ such that $A_{\text{nominal}} < A_{\text{ref}}$ (or via an explicit `headroom_factor` / full-scale margin), reserving dynamic range above nominal peak for surges and swells before reaching the saturation boundary $A_{\text{ref}}$.
    - In-memory integer containers and alignment:
      - 8-bit: stored in 1-byte container (`uint8_t` for unipolar, `int8_t` for bipolar).
      - 12-bit: stored right-justified in 2-byte container (`uint16_t` for unipolar with bits $[15:12]$ zero-padded; `int16_t` for bipolar with bits $[15:12]$ sign-extended from bit 11).
      - 16-bit: stored in 2-byte container (`uint16_t` for unipolar, `int16_t` for bipolar).
      - 24-bit: stored right-justified in 4-byte container (`uint32_t` for unipolar with bits $[31:24]$ zero-padded; `int32_t` for bipolar with bits $[31:24]$ sign-extended from bit 23).
    - Saturation & Clamping: Signal values outside the configured dynamic range shall be clamped/saturated at the minimum and maximum discrete counts without integer overflow or wrap-around.
- FR-007: The system shall structure the shared memory segment as a Single-Producer Single-Consumer (SPSC) lock-free ring buffer comprising a user-configurable number of slots $K$ ($2 \le K \le 6$).
- FR-008: The shared memory segment shall include a fixed Global Control Header at offset 0 containing:
  - Active configuration fields: slot count $K$, block size $N$ (samples per channel), configured channel count, sample representation format, memory layout mode (interleaved vs planar), and configured sampling rate.
  - 64-bit atomic synchronization variables: `write_seq` (monotonically incremented by producer), `read_seq` (monotonically incremented by consumer), and `dropped_blocks` (incremented on overrun).
- FR-009: Each ring buffer slot shall include a Per-Slot Header immediately preceding the sample data payload, containing:
  - Block sequence number (monotonically incremented with each emitted block; readable before and after payload copy for torn-read detection).
  - 64-bit virtual simulation timestamp (in nanoseconds) corresponding to the first sample in the block.
  - Per-slot payload metadata (sample count $N$, payload byte size).
- FR-010: Each ring buffer slot shall store exactly one block of $N$ samples for every configured channel, formatted according to the configured memory layout (interleaved or planar) immediately following the Per-Slot Header.
- FR-011: The system shall provide an OS synchronization signaling primitive to wake the consumer when a new block is available:
  - On Linux, the producer shall signal via a named POSIX semaphore (`sem_post`) or eventfd descriptor.
  - The producer shall write the block payload, update the Per-Slot Header, update the atomic `write_seq` in the Global Control Header with release memory ordering, and subsequently signal the OS synchronization primitive.
  - The consumer may wait on the OS synchronization primitive to achieve zero CPU utilization during inter-block idle periods, or alternatively poll `write_seq` with acquire memory ordering.
- FR-012: The system shall accept an input configuration via a structured JSON file specifying nominal signal parameters, ADC quantization parameters, buffer geometry, IPC identifiers, and an optional deterministic disturbance event timeline.
- FR-013: The simulator CLI shall support passing the configuration file path via `--config <path>` (or `-c <path>`), with command-line validation verifying JSON syntax and parameter ranges prior to initializing shared memory or starting signal synthesis. If validation fails, the process shall emit a descriptive error to standard error and terminate with a non-zero exit code.
- FR-014: The system shall support a deterministic disturbance event timeline defined in the JSON configuration where events execute at specified simulation timestamps:
  - Surge event: Injects an additive transient impulse or surge waveform into one or more specified channels with configurable start time, duration, and non-zero (positive or negative) peak amplitude.
  - Sag / Swell event: Modulates the nominal waveform amplitude by a configurable scaling factor (sag: $0 \le s < 1.0$, swell: $> 1.0$) for a configurable duration on one or more specified channels.
  - Frequency distortion event: Alters the fundamental frequency by applying a frequency step or ramp $(\Delta f)$ for a configurable duration synchronously across all configured channels, maintaining continuous phase across transitions without discontinuous phase jumps.
- FR-015: Disturbance events shall support targeting an individual configured channel or all configured channels simultaneously. During disturbance injection, if the resultant instantaneous signal value exceeds the configured ADC dynamic range, the quantizer shall saturate at the ADC rail limits in accordance with FR-006.

## Non-Functional Requirements

- NFR-001: The simulator shall compile and execute on Linux host platforms.
- NFR-002: Real-time sample delivery throughput shall sustain the configured sampling rate and block emission cadence without dropped blocks under normal host load.
- NFR-003: Timing jitter across emitted sample blocks shall remain bounded and shall not accumulate unbounded phase or frequency drift over continuous runtime.

## Constraints

- Execution is restricted to standard Linux host development/testing environments without requiring a hard real-time kernel (PREEMPT_RT) as a mandatory prerequisite.
- Target initial scope supports a configurable number of channels and sinusoidal waveforms, including three-phase profiles.

## Assumptions

- The consuming application or test harness runs on the same Linux host and attaches to the configured shared memory segment.

## Failure Modes

- Buffer overrun in shared memory if the consumer processes blocks slower than the generation rate.
- Configuration validation failures for invalid frequency, negative amplitude, or unachievable sample rates.

## Acceptance Criteria

- AC-001: Given a sinusoidal configuration with a configured number of channels and specified frequency, amplitude, and phase offsets, the system outputs samples exhibiting the specified phase shifts and amplitudes for every configured channel.
- AC-002: In `float32` mode, output values match theoretical waveform values within IEEE-754 binary32 precision tolerance.
- AC-003: In raw ADC mode, output values are correctly quantized according to the configured bit depth $B \in \{8, 12, 16, 24\}$, reference amplitude $A_{\text{ref}}$, and polarity:
  - For `UNIPOLAR` mode, input dynamic range $[0, A_{\text{ref}}]$ maps linearly to $[0, 2^B - 1]$; values $< 0$ clamp to $0$, and values $> A_{\text{ref}}$ clamp to $2^B - 1$. Unused high bits in containers (bits $[15:12]$ for 12-bit; bits $[31:24]$ for 24-bit) are zero-extended.
  - For `BIPOLAR` mode, input dynamic range $[-A_{\text{ref}}, +A_{\text{ref}}]$ maps linearly to $[-2^{B-1}, 2^{B-1} - 1]$; values $< -A_{\text{ref}}$ clamp to $-2^{B-1}$, and values $> +A_{\text{ref}}$ clamp to $2^{B-1} - 1$. Unused high bits in containers are sign-extended from the ADC MSB ($B-1$).
  - Sample container sizes match the specification: 1 byte for 8-bit, 2 bytes for 12-bit and 16-bit, and 4 bytes for 24-bit.
- AC-004: When configured for block emission of size $N$, the system emits batches containing exactly $N$ samples per channel at the calculated interval $N \times T_s$.
- AC-005: When configured for interleaved layout, samples for each sample index appear in configured channel order; when configured for planar layout, each channel's $N$ samples appear contiguously in configured channel order.
- AC-006: Generated sample blocks are successfully written to and read from the shared memory SPSC ring buffer across all $K$ slots in sequence.
- AC-007: The consumer validates the Global Control Header magic bytes and configuration parameters upon attaching, rejecting mismatched layouts before reading samples.
- AC-008: The consumer verifies per-slot sequence numbers and timestamps to detect torn reads, dropped blocks, and time continuity across successive slots.
- AC-009: If the consumer fails to advance its read pointer before the ring buffer fills, the simulator handles the overrun deterministically (increments `dropped_blocks` metric without memory corruption, deadlock, or crashing).
- AC-010: The producer signals the OS event/semaphore after every committed slot write. A blocking consumer waiting on the OS synchronization primitive wakes up and successfully reads the newly available block without requiring a busy-polling loop.
- AC-011: Given a configured surge event at simulation time $t_{\text{start}}$, the emitted samples for the targeted channel reflect the additive surge with peak amplitude and duration within 1 sample period ($T_s$) of the configured timeline.
- AC-012: Given a configured sag or swell event, the targeted channel amplitude scales by the specified factor during the active window, and returns to nominal amplitude upon event expiration.
- AC-013: Given a configured frequency distortion event, the generated waveforms track the modified frequency across all configured channels without discontinuous phase jumps or waveform tearing at transition boundaries.
- AC-014: If an injected surge or swell stays within $A_{\text{ref}}$, raw ADC output codes proportionally track the increased signal value within the reserved headroom without saturation. If the disturbance signal exceeds $A_{\text{ref}}$ (or falls below $A_{\text{min}}$), output codes clamp to the respective extreme counts ($0$ or $2^B - 1$ for unipolar; $-2^{B-1}$ or $2^{B-1} - 1$ for bipolar) without numerical wrapping.
- AC-015: When launched with an invalid JSON configuration or out-of-range parameters (e.g., negative frequency, unknown polarity, invalid bit depth, or nominal amplitude plus offset exceeding $A_{\text{ref}}$ without headroom), the simulator fails fast before allocating shared memory, emits an error to `stderr`, and exits with a non-zero code.

## Verification Notes

- Waveform correctness to be verified via offline spectral/harmonic analysis (FFT) and phase angle calculation from captured egress data.
- Quantization accuracy in raw mode to be verified by comparing reconstructed engineering values against original analog curve.
- Block cadence and sample count to be verified by capturing egress streams and measuring inter-block arrival timestamps and payload lengths.
- Channel layout conformance (interleaved vs planar) to be verified by comparing demuxed channel vectors against expected mathematical sequences.
- Ring buffer synchronization to be verified using automated test harness validating concurrent reader-writer operations under simulated consumer latency, verifying seqlock torn-read detection and drop counter increments.

## Open Questions

- None. (All functional boundaries, data representations, timing cadences, IPC synchronization primitives, and non-goals are resolved.)

## Risks

- Operating system scheduler latency on the general-purpose Linux CFS kernel may cause wake-up jitter exceeding 130 µs when using naive sleep mechanisms.
- Memory allocation or page faults during runtime loop can introduce multi-millisecond spikes.
- Consumer lag or stall may cause shared memory ring buffer overruns, resulting in dropped sample blocks or data tearing if synchronization is insufficient.

## Glossary

- ADC: Analog-to-Digital Converter
- IPC: Inter-Process Communication
- SPSC: Single-Producer Single-Consumer lock-free ring buffer
- Slot: One discrete buffer partition in the ring buffer capable of holding one full block of $N$ samples for every configured channel
- Phase Shift: Angular displacement between wave peaks across channels (typically nominal 120° for 3-phase)
