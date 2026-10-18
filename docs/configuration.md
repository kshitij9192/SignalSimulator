# SignalSimulator Configuration

The Linux simulator reads a JSON configuration with `--config <path>` (or `-c <path>`). [sample_config.json](sample_config.json) is a complete example. Its `$schema` property points to [signal_simulator.schema.json](signal_simulator.schema.json), enabling validation and completion in editors that support JSON Schema, including VS Code.

The schema documents the config shape, supported choices, and local value ranges. Runtime validation must also check relationships between fields and reject invalid configurations before creating shared memory. JSON does not support comments; use this guide for explanation instead of adding comments to a config file.

## Top-Level Settings

| Property | Required | Purpose |
| --- | --- | --- |
| `signal` | Yes | Nominal sine-wave frequency, amplitude, offset, and configured channel phases. |
| `sampling` | Yes | Samples per cycle at the nominal frequency. |
| `output` | Yes | Sample representation, channel memory layout, and ADC options. |
| `buffer` | Yes | Ring-buffer slot count and samples per channel in each block. |
| `ipc` | Yes | Linux shared-memory and notification identifiers. |
| `disturbance_timeline` | No | Optional scheduled waveform events. |

Unknown properties are rejected by the schema. Enum values are case-sensitive and use the lowercase spellings shown in the sample.

## Signal And Sampling

`signal.frequency_hz` is the positive nominal frequency in hertz. `nominal_amplitude` is the peak amplitude; `offset` is a constant offset. `channels` must contain at least one entry, and each channel is identified by its 0-based index in the array. Each `phase_deg` is that channel's phase displacement in degrees; the sample uses the common three-phase values 0, 120, and 240 degrees. Event `target_channels` values must be valid 0-based indices into `signal.channels`.

The nominal signal value for channel $i$ is:

$$
x_i(t) = \text{offset} + \text{nominal\_amplitude} \sin\left(2\pi f t + \phi_i\right),
\qquad \phi_i = \text{phase\_deg}_i \frac{\pi}{180}.
$$

Set `sampling.samples_per_cycle` to the integer resolution at the nominal frequency. The parser derives the sample rate as `frequency_hz * samples_per_cycle`.

The derived sample rate remains fixed during frequency-distortion events. Consequently, the number of samples per cycle changes while the event changes the signal frequency. With the sample values, 50 Hz and 128 samples per cycle produce 6.4 kHz. `block_size_samples_per_channel` of 240 then gives a nominal block interval of 37.5 ms.

## Output And ADC

`output.representation` accepts:

- `float32`: emit IEEE-754 32-bit engineering-unit samples. Do not include `output.adc` for this mode.
- `adc_integer`: quantize signal samples using the required ADC settings.

`output.layout` is `interleaved` (channel samples alternate at each sample index in configured channel order) or `planar` (all samples for one channel are followed by all samples for the next, in configured order). Each slot contains the configured number of channels times `block_size_samples_per_channel` samples.

For `adc_integer`, `reference_amplitude` must be positive, `polarity` is `unipolar` or `bipolar`, and `resolution_bits` is 8, 12, 16, or 24. Unipolar input spans `[0, Aref]`; bipolar input spans `[-Aref, +Aref]`. Out-of-range values saturate at the corresponding ADC rail. Containers are 1 byte for 8-bit, 2 bytes for 12- and 16-bit, and 4 bytes for 24-bit samples; 12- and 24-bit values are right-justified.

Nominal amplitude and offset must fit the selected ADC range to preserve headroom. For bipolar mode, the nominal peak envelope is `abs(offset) + nominal_amplitude` and must not exceed `reference_amplitude`. For unipolar mode, the full nominal envelope must fit `[0, reference_amplitude]`. Disturbances may exceed the rails and are saturated during quantization.

## Buffer And IPC

`buffer.slot_count` must be between 2 and 6. A slot holds exactly one block and its metadata; the shared-memory header carries the active geometry and sequence counters.

The initial release uses Linux IPC only. `shared_memory_name` is a POSIX shared-memory name, and `notification` configures the POSIX named semaphore used to wake the consumer. Names begin with one slash and contain no additional slash. The example semaphore is initialized to zero. Consumers may wait on the notification or poll the shared-memory sequence counters, as supported by the implementation.

## Disturbance Timeline

All event times and durations are integer nanoseconds relative to simulation start. Events are deterministic; list them in ascending `start_time_ns` order for readability. Targeted events must specify one or more 0-based configured channel indices. Frequency distortion affects all configured channels synchronously.

| Type | Required event-specific settings | Behavior |
| --- | --- | --- |
| `surge` | `shape`, `peak_amplitude` | Adds a half-sine transient to each target channel over the event duration; `peak_amplitude` is its non-zero (positive or negative) peak. |
| `sag` | `scaling_factor` | Multiplies target-channel amplitude by a factor in `[0, 1)`. |
| `swell` | `scaling_factor` | Multiplies target-channel amplitude by a factor greater than 1. |
| `frequency_distortion` | `mode`, `delta_frequency_hz` | Applies a frequency offset to all channels. `step` applies it throughout the event; `ramp` changes the offset linearly from zero to `delta_frequency_hz` over the event duration. Phase remains continuous through transitions. |

Sag and swell scale the sine amplitude, not the constant offset. Frequency changes must not make instantaneous frequency zero or negative. Until event-overlap composition is defined by the implementation, avoid overlapping events that affect the same channel or frequency.

## Validation

The schema checks JSON structure, required fields, enums, and local ranges. Runtime validation must additionally check conditions that depend on multiple values, including event targets referring to valid configured channel indices, nominal waveform fit within ADC rails, positive frequency throughout a frequency event, achievable sample rates, and Linux IPC constraints. Invalid configuration must be rejected before shared-memory initialization.